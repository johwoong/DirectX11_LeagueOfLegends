// FbxToDat.cpp (완성본)
#include <windows.h>
#include <string>
#include <filesystem>
#include <vector>
#include <cstdio>
#include <iostream>
#include <stdexcept>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#pragma pack(push, 1)
struct float2 { float x, y; };
struct float3 { float x, y, z; };
struct FLOAT4X4 { float m[16]; };
struct VTXMODEL {
    float3 vPosition;
    float3 vNormal;
    float2 vTexUV;
    float3 vTangent;
};
struct FACEINDICES32 { uint32_t _0, _1, _2; };
#pragma pack(pop)

static std::string to_utf8(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s(n ? n - 1 : 0, '\0');
    if (n) WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, s.data(), n, nullptr, nullptr);
    return s;
}

static void write_n(FILE* fp, const void* p, size_t sz) {
    size_t w = fwrite(p, 1, sz, fp);
    if (w != sz) throw std::runtime_error("file write error");
}

static void put_str_fixed(FILE* fp, const std::string& s) {
    char buf[MAX_PATH]{};
    strncpy_s(buf, s.c_str(), MAX_PATH - 1);
    write_n(fp, buf, sizeof(buf));
}

int wmain(int argc, wchar_t** wargv) {
    try {
        if (argc < 3) {
            std::wcout << L"Usage: FbxToDat.exe <input.fbx> <output.dat> [scale]\n";
            return 1;
        }
        std::wstring win = wargv[1];
        std::wstring wout = wargv[2];
        float scale = (argc >= 4) ? static_cast<float>(_wtof(wargv[3])) : 1.0f;

        std::string in_utf8 = to_utf8(win);

        // 입력 FBX 존재 확인(유니코드 경로)
        if (!std::filesystem::exists(win)) {
            std::wcerr << L"[ERROR] input FBX not found: " << win << L"\n";
            return 11;
        }

        // 출력 폴더 생성
        std::filesystem::create_directories(std::filesystem::path(wout).parent_path());

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            in_utf8.c_str(),
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_GenNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_OptimizeMeshes |
            aiProcess_ValidateDataStructure
        );
        if (!scene || !scene->mRootNode) {
            std::cerr << "[ERROR] Assimp load failed: " << importer.GetErrorString() << "\n";
            return 2;
        }

        FILE* fp = nullptr;
        if (_wfopen_s(&fp, wout.c_str(), L"wb") || !fp) {
            std::wcerr << L"[ERROR] Cannot open output: " << wout << L"\n";
            return 3;
        }

        size_t totalBytes = 0;
        auto addBytes = [&](size_t n) { totalBytes += n; };

        // 1) PivotMatrix
        {
            FLOAT4X4 pivot{};
            for (int i = 0; i < 16; ++i) pivot.m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
            pivot.m[0] = pivot.m[5] = pivot.m[10] = scale;  // uniform scale
            write_n(fp, &pivot, sizeof(pivot)); addBytes(sizeof(pivot));
        }

        // 2) NumMeshes
        {
            uint32_t numMeshes = scene->mNumMeshes;
            write_n(fp, &numMeshes, sizeof(numMeshes)); addBytes(sizeof(numMeshes));
        }

        // 3) Meshes
        for (uint32_t mi = 0; mi < scene->mNumMeshes; ++mi) {
            const aiMesh* m = scene->mMeshes[mi];

            // 3-1) name
            std::string meshName = (m->mName.length > 0) ? std::string(m->mName.C_Str()) : "Mesh";
            put_str_fixed(fp, meshName); addBytes(MAX_PATH);

            // 3-2) material idx
            uint32_t matIndex = m->mMaterialIndex;
            write_n(fp, &matIndex, sizeof(matIndex)); addBytes(sizeof(matIndex));

            // 3-3) vertices
            uint32_t numVertices = m->mNumVertices;
            write_n(fp, &numVertices, sizeof(numVertices)); addBytes(sizeof(numVertices));

            std::vector<VTXMODEL> verts(numVertices);
            for (uint32_t i = 0; i < numVertices; ++i) {
                // pos
                if (m->HasPositions()) {
                    verts[i].vPosition = { m->mVertices[i].x, m->mVertices[i].y, m->mVertices[i].z };
                }
                else { verts[i].vPosition = { 0,0,0 }; }
                // normal
                if (m->HasNormals()) {
                    verts[i].vNormal = { m->mNormals[i].x, m->mNormals[i].y, m->mNormals[i].z };
                }
                else { verts[i].vNormal = { 0,1,0 }; }
                // uv0
                if (m->HasTextureCoords(0)) {
                    verts[i].vTexUV = {
                        (float)m->mTextureCoords[0][i].x,
                        (float)m->mTextureCoords[0][i].y
                    };
                }
                else { verts[i].vTexUV = { 0,0 }; }
                // tangent
                if (m->HasTangentsAndBitangents()) {
                    verts[i].vTangent = { m->mTangents[i].x, m->mTangents[i].y, m->mTangents[i].z };
                }
                else { verts[i].vTangent = { 1,0,0 }; }
            }
            write_n(fp, verts.data(), sizeof(VTXMODEL) * verts.size());
            addBytes(sizeof(VTXMODEL) * verts.size());

            // 3-4) indices
            uint32_t numFaces = m->mNumFaces;   // == #triangles (triangulate 했으므로)
            write_n(fp, &numFaces, sizeof(numFaces)); addBytes(sizeof(numFaces));

            std::vector<FACEINDICES32> faces(numFaces);
            for (uint32_t f = 0; f < numFaces; ++f) {
                const aiFace& face = m->mFaces[f];
                faces[f] = { face.mIndices[0], face.mIndices[1], face.mIndices[2] };
            }
            write_n(fp, faces.data(), sizeof(FACEINDICES32) * faces.size());
            addBytes(sizeof(FACEINDICES32) * faces.size());
        }

        // 4) Materials (텍스처 경로 MAX_PATH 고정 길이)
        {
            uint32_t numMaterials = scene->mNumMaterials;
            write_n(fp, &numMaterials, sizeof(numMaterials)); addBytes(sizeof(numMaterials));

            std::filesystem::path fbxDir = std::filesystem::path(win).parent_path();
            for (uint32_t mi = 0; mi < numMaterials; ++mi) {
                aiMaterial* aimat = scene->mMaterials[mi];

                for (int t = 0; t < AI_TEXTURE_TYPE_MAX; ++t) {
                    aiTextureType type = (aiTextureType)t;
                    aiString tex;
                    std::string full;

                    if (type == aiTextureType_DIFFUSE || type == aiTextureType_NORMALS) {
                        if (AI_SUCCESS == aimat->GetTexture(type, 0, &tex)) {
                            std::filesystem::path rel = std::filesystem::u8path(tex.C_Str());
                            std::filesystem::path abs = rel.is_absolute() ? rel : (fbxDir / rel);
                            abs = abs.lexically_normal();
                            full = abs.u8string(); // UTF-8로 고정 길이 저장
                        }
                    }
                    put_str_fixed(fp, full); addBytes(MAX_PATH);
                }
            }
        }

        fflush(fp);
        fclose(fp);
        std::wcout << L"[OK] wrote dat: " << wout << L"  (" << totalBytes << L" bytes)\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "[EXCEPTION] " << e.what() << "\n";
        return 10;
    }
}