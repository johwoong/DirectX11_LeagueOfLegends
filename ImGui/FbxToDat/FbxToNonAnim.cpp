#define NOMINMAX
#include <Windows.h>
#include <string>
#include <filesystem>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cstdio>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace std;
namespace fs = std::filesystem;

// ============================= 엔진 구조 =============================
struct float2 { float x, y; };
struct float3 { float x, y, z; };
struct FLOAT4X4 { float m[16]; };
struct FACEINDICES32 { unsigned int _0, _1, _2; };

struct VTXMODEL {
    float3 vPosition;
    float3 vNormal;
    float2 vTexUV;
    float3 vTangent;
};

// ============================= 파일 유틸 =============================
static void write_n(FILE* fp, const void* p, size_t sz) {
    if (fwrite(p, 1, sz, fp) != sz)
        throw runtime_error("file write error");
}
static void put_str_fixed(FILE* fp, const string& s) {
    char buf[MAX_PATH]{};
    string fixed = s;
    replace(fixed.begin(), fixed.end(), '/', '\\');
    strncpy_s(buf, fixed.c_str(), MAX_PATH - 1);
    write_n(fp, buf, sizeof(buf));
}
static void ToF4x4(FLOAT4X4& out, const aiMatrix4x4& m) {
    out.m[0] = m.a1; out.m[1] = m.a2; out.m[2] = m.a3; out.m[3] = m.a4;
    out.m[4] = m.b1; out.m[5] = m.b2; out.m[6] = m.b3; out.m[7] = m.b4;
    out.m[8] = m.c1; out.m[9] = m.c2; out.m[10] = m.c3; out.m[11] = m.c4;
    out.m[12] = m.d1; out.m[13] = m.d2; out.m[14] = m.d3; out.m[15] = m.d4;
}

// ============================= 텍스처 자동 탐색 =============================
static fs::path FindTextureFile(const fs::path& texDir)
{
    for (const auto& entry : fs::directory_iterator(texDir))
    {
        if (!entry.is_regular_file()) continue;

        string ext = entry.path().extension().string();
        transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".png" || ext == ".dds")
            return entry.path();
    }
    return fs::path(); // 없으면 빈 경로
}

// ============================= Model.dat =============================
static void WriteNonAnimModel(FILE* fp, const aiScene* scene, const fs::path& texDir)
{
    // Pivot 행렬 (스케일 0.05배)
    FLOAT4X4 pivot{};
    pivot.m[0] = pivot.m[5] = pivot.m[10] = 0.05f;
    pivot.m[15] = 1.f;
    write_n(fp, &pivot, sizeof(pivot));

    unsigned numMeshes = scene->mNumMeshes;
    write_n(fp, &numMeshes, sizeof(numMeshes));

    for (unsigned mi = 0; mi < numMeshes; ++mi)
    {
        const aiMesh* mesh = scene->mMeshes[mi];
        string meshName = mesh->mName.length ? mesh->mName.C_Str() : ("Mesh_" + to_string(mi));
        put_str_fixed(fp, meshName);

        unsigned matIndex = mesh->mMaterialIndex;
        write_n(fp, &matIndex, sizeof(matIndex));

        unsigned numVerts = mesh->mNumVertices;
        write_n(fp, &numVerts, sizeof(numVerts));

        vector<VTXMODEL> verts(numVerts);


        const float scale = 0.01f;  // DirectX용 적정 축소비

        for (unsigned v = 0; v < numVerts; ++v)
        {
            verts[v].vPosition = { mesh->mVertices[v].x * scale,
                                   mesh->mVertices[v].y * scale,
                                   mesh->mVertices[v].z * scale };
            verts[v].vNormal = mesh->HasNormals() ? float3{ mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z } : float3{ 0,1,0 };
            verts[v].vTexUV = mesh->HasTextureCoords(0)
                ? float2{ (float)mesh->mTextureCoords[0][v].x, (float)mesh->mTextureCoords[0][v].y }
            : float2{ 0,0 };
            verts[v].vTangent = mesh->HasTangentsAndBitangents()
                ? float3{ mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z }
            : float3{ 1,0,0 };
        }

        write_n(fp, verts.data(), sizeof(VTXMODEL) * verts.size());

        unsigned numFaces = mesh->mNumFaces;
        write_n(fp, &numFaces, sizeof(numFaces));

        vector<FACEINDICES32> faces(numFaces);
        for (unsigned f = 0; f < numFaces; ++f)
            faces[f] = { mesh->mFaces[f].mIndices[0], mesh->mFaces[f].mIndices[1], mesh->mFaces[f].mIndices[2] };

        write_n(fp, faces.data(), sizeof(FACEINDICES32) * faces.size());
    }

    // 머티리얼
    unsigned numMats = scene->mNumMaterials;
    write_n(fp, &numMats, sizeof(numMats));

    fs::path autoTex = FindTextureFile(texDir);
    string autoTexStr = autoTex.empty() ? "default_diffuse.png" : autoTex.string();

    for (unsigned mi = 0; mi < numMats; ++mi)
    {
        aiMaterial* mat = scene->mMaterials[mi];
        for (int t = 0; t < AI_TEXTURE_TYPE_MAX; ++t)
        {
            aiString tex;
            char path[MAX_PATH]{};

            if (mat->GetTexture((aiTextureType)t, 0, &tex) == AI_SUCCESS)
            {
                fs::path texFile = texDir / fs::path(tex.C_Str()).filename();
                strncpy_s(path, texFile.string().c_str(), MAX_PATH - 1);
            }
            else
            {
                // FBX가 텍스처를 지정하지 않은 경우 자동 탐색된 텍스처 사용
                strncpy_s(path, autoTexStr.c_str(), MAX_PATH - 1);
            }

            write_n(fp, path, sizeof(path));
        }
    }
}

int wmain()
{
    try
    {
        fs::path root = L"C:\\DirectX11_LeagueOfLegends\\Bin\\Resources\\Meshes\\Anim\\Champion";
        fs::path outputRoot = L"C:\\DirectX11_LeagueOfLegends\\Bin\\Bin_Data\\NonAnim\\Champion";

        if (!fs::exists(root))
            throw runtime_error("Champion directory not found!");

        // ? Champion 내부의 모든 하위 폴더를 재귀적으로 순회
        for (const auto& dir : fs::recursive_directory_iterator(root))
        {
            if (!dir.is_directory()) continue;

            fs::path championPath = dir.path();
            fs::path championName = championPath.filename(); // 예: Ezreal, Buildings 등

            fs::path fbxPath;
            for (const auto& f : fs::directory_iterator(championPath))
            {
                if (f.path().extension() == ".fbx" || f.path().extension() == ".FBX")
                {
                    fbxPath = f.path();
                    break;
                }
            }

            if (fbxPath.empty()) {
                wcout << L"[SKIP] " << championName << L" - FBX not found\n";
                continue;
            }

            // ? 출력 경로를 입력 경로와 동일한 구조로 맞춤
            fs::path relative = fs::relative(championPath, root);
            fs::path outDir = outputRoot / relative;
            fs::create_directories(outDir);

            fs::path outFile = outDir / (championName.wstring() + L".dat");

            // ? 이미 .dat 파일이 있으면 스킵
            if (fs::exists(outFile))
            {
                wcout << L"[SKIP] " << championName << L" - already exists (" << outFile << L")\n";
                continue;
            }

            Assimp::Importer imp;
            const aiScene* scene = imp.ReadFile(
                fbxPath.string(),
                aiProcess_Triangulate |
                aiProcess_JoinIdenticalVertices |
                aiProcess_GenNormals |
                aiProcess_CalcTangentSpace |
                aiProcess_ConvertToLeftHanded
            );
            if (!scene) {
                cerr << "[ERROR] " << imp.GetErrorString() << endl;
                continue;
            }

            FILE* fp = nullptr;
            _wfopen_s(&fp, outFile.c_str(), L"wb");
            WriteNonAnimModel(fp, scene, fbxPath.parent_path());
            fclose(fp);

            wcout << L"[OK] " << relative << L" → " << outFile << endl;
        }

        wcout << L"\n=== 모든 NonAnim 모델 변환 완료 (Champion 하위 전체 탐색 + 동일 구조 생성) ===\n";
    }
    catch (const exception& e)
    {
        cerr << "[ERROR] " << e.what() << endl;
    }
}
