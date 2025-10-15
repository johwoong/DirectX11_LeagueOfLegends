#include <windows.h>
#include <string>
#include <filesystem>
#include <vector>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <fstream>

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

using namespace std;
namespace fs = std::filesystem;

static string to_utf8(const wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    string s(n ? n - 1 : 0, '\0');
    if (n) WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, s.data(), n, nullptr, nullptr);
    return s;
}

static void write_n(FILE* fp, const void* p, size_t sz) {
    if (fwrite(p, 1, sz, fp) != sz)
        throw runtime_error("file write error");
}

static void put_str_fixed(FILE* fp, const string& s) {
    char buf[MAX_PATH]{};
    if (!s.empty()) {
        string fixed = s;
        replace(fixed.begin(), fixed.end(), '/', '\\');
        strncpy_s(buf, fixed.c_str(), MAX_PATH - 1);
    }
    fwrite(buf, sizeof(char), MAX_PATH, fp);
}

static string extract_embedded_texture(const aiTexture* tex, const fs::path& outputDir, int index) {
    fs::create_directories(outputDir);

    string filename = "Embedded_" + to_string(index) + ".png";
    fs::path fullpath = outputDir / filename;

    if (tex->mHeight == 0) {
        // 압축된 포맷(JPG/PNG 등)
        ofstream fout(fullpath, ios::binary);
        fout.write((const char*)tex->pcData, tex->mWidth);
        fout.close();
    }
    else {
        // RAW BGRA32 포맷
        ofstream fout(fullpath, ios::binary);
        fout.write((const char*)tex->pcData, tex->mWidth * tex->mHeight * 4);
        fout.close();
    }

    return fullpath.u8string();
}

int wmain(int argc, wchar_t** wargv) {
    try {
        if (argc < 3) {
            wcout << L"Usage: FbxToDat.exe <input.glb> <output.dat>\n";
            return 1;
        }

        wstring win = wargv[1];
        wstring wout = wargv[2];
        string in_utf8 = to_utf8(win);

        if (!fs::exists(win)) {
            wcerr << L"[ERROR] input model not found: " << win << L"\n";
            return 11;
        }

        fs::create_directories(fs::path(wout).parent_path());

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            in_utf8.c_str(),
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_GenNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_OptimizeMeshes |
            aiProcess_ValidateDataStructure |
            aiProcess_ConvertToLeftHanded |
            aiProcess_FlipUVs
        );

        if (!scene || !scene->mRootNode) {
            cerr << "[ERROR] Assimp failed: " << importer.GetErrorString() << "\n";
            return 2;
        }

        FILE* fp = nullptr;
        if (_wfopen_s(&fp, wout.c_str(), L"wb") || !fp) {
            wcerr << L"[ERROR] cannot open output: " << wout << L"\n";
            return 3;
        }

        /* Embedded 텍스처 추출 */
        fs::path extractDir = L"C:\\DirectX11_LeagueOfLegends\\Bin\\Resources\\Meshes\\NonAnim\\ExtractedTextures";
        vector<string> extractedTexPaths;
        if (scene->mNumTextures > 0) {
            for (unsigned int i = 0; i < scene->mNumTextures; ++i) {
                const aiTexture* tex = scene->mTextures[i];
                string saved = extract_embedded_texture(tex, extractDir, i);
                extractedTexPaths.push_back(saved);
                cout << "[Extracted] " << saved << endl;
            }
        }

        /* Pivot */
        FLOAT4X4 pivot{};
        for (int i = 0; i < 16; ++i)
            pivot.m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        write_n(fp, &pivot, sizeof(pivot));

        /* Meshes */
        uint32_t numMeshes = scene->mNumMeshes;
        write_n(fp, &numMeshes, sizeof(numMeshes));

        for (uint32_t mi = 0; mi < numMeshes; ++mi) {
            const aiMesh* mesh = scene->mMeshes[mi];

            string meshName = mesh->mName.length ? mesh->mName.C_Str() : "Mesh";
            put_str_fixed(fp, meshName);

            uint32_t matIndex = mesh->mMaterialIndex;
            write_n(fp, &matIndex, sizeof(matIndex));

            uint32_t numVertices = mesh->mNumVertices;
            write_n(fp, &numVertices, sizeof(numVertices));

            vector<VTXMODEL> verts(numVertices);
            for (uint32_t i = 0; i < numVertices; ++i) {
                verts[i].vPosition = mesh->HasPositions()
                    ? float3{ mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z }
                : float3{ 0,0,0 };

                verts[i].vNormal = mesh->HasNormals()
                    ? float3{ mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z }
                : float3{ 0,1,0 };

                verts[i].vTexUV = mesh->HasTextureCoords(0)
                    ? float2{ (float)mesh->mTextureCoords[0][i].x, (float)mesh->mTextureCoords[0][i].y }
                : float2{ 0,0 };

                verts[i].vTangent = mesh->HasTangentsAndBitangents()
                    ? float3{ mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z }
                : float3{ 1,0,0 };
            }
            write_n(fp, verts.data(), sizeof(VTXMODEL) * verts.size());

            uint32_t numFaces = mesh->mNumFaces;
            write_n(fp, &numFaces, sizeof(numFaces));

            vector<FACEINDICES32> faces(numFaces);
            for (uint32_t f = 0; f < numFaces; ++f) {
                const aiFace& face = mesh->mFaces[f];
                faces[f] = { face.mIndices[0], face.mIndices[1], face.mIndices[2] };
            }
            write_n(fp, faces.data(), sizeof(FACEINDICES32) * faces.size());
        }

        /* Materials */
        uint32_t numMaterials = scene->mNumMaterials;
        write_n(fp, &numMaterials, sizeof(numMaterials));

        fs::path baseDir = fs::path(win).parent_path();

        for (uint32_t i = 0; i < numMaterials; ++i) {
            aiMaterial* mat = scene->mMaterials[i];

            for (uint32_t t = 0; t < AI_TEXTURE_TYPE_MAX; ++t) {
                aiString tex;
                string texPath;
                if (t == aiTextureType_DIFFUSE || t == aiTextureType_NORMALS) {
                    if (AI_SUCCESS == mat->GetTexture((aiTextureType)t, 0, &tex)) {
                        if (tex.C_Str()[0] == '*') {
                            int embeddedIdx = atoi(tex.C_Str() + 1);
                            if (embeddedIdx < extractedTexPaths.size())
                                texPath = extractedTexPaths[embeddedIdx];
                        }
                        else {
                            fs::path rel = fs::u8path(tex.C_Str());
                            fs::path abs = rel.is_absolute() ? rel : (baseDir / rel);
                            abs = abs.lexically_normal();
                            texPath = abs.u8string();
                        }
                    }
                }
                put_str_fixed(fp, texPath);
            }
        }

        fclose(fp);
        wcout << L"[OK] wrote dat: " << wout << endl;
        return 0;
    }
    catch (const std::exception& e) {
        cerr << "[EXCEPTION] " << e.what() << endl;
        return 10;
    }
}