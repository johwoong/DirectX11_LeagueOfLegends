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

static void write_n(FILE* fp, const void* p, size_t sz) {
    if (!fp) throw runtime_error("invalid FILE*");
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

static void put_path_fixed_ansi(FILE* fp, const fs::path& p) {
    char out[MAX_PATH]{};

    if (!p.empty()) {
        wstring ws = p.wstring();
        replace(ws.begin(), ws.end(), L'/', L'\\');
        WideCharToMultiByte(CP_ACP, 0, ws.c_str(), -1, out, MAX_PATH, nullptr, nullptr);
    }

    write_n(fp, out, sizeof(out));
}

static fs::path GetAssimpTexture(aiMaterial* mat, aiTextureType type, const fs::path& texDir)
{
    if (!mat) return fs::path();

    aiString rel;
    if (mat->GetTextureCount(type) > 0 && mat->GetTexture(type, 0, &rel) == AI_SUCCESS)
    {
        fs::path p(rel.C_Str());

        // 보통은 파일명만 들어오므로 texDir 기준으로 조합
        if (p.has_filename()) {
            fs::path candidate = texDir / p.filename();
            if (fs::exists(candidate))
                return candidate;
        }

        // 절대경로였던 경우
        if (fs::exists(p))
            return p;
    }

    return fs::path(); // 없으면 빈 경로(== 빈 문자열로 저장)
}

static void WriteNonAnimModel_EngineFormat(FILE* fp, const aiScene* scene, const fs::path& texDir)
{
    // =========================================================
    // 1) Pivot (HalfSphere.dat와 동일: 스케일 0.01만)
    // =========================================================
    FLOAT4X4 pivot{};
    {
        // identity
        pivot.m[0] = 1.f; pivot.m[5] = 1.f; pivot.m[10] = 1.f; pivot.m[15] = 1.f;

        // scale 0.01
        pivot.m[0] *= 0.01f;
        pivot.m[5] *= 0.01f;
        pivot.m[10] *= 0.01f;
    }
    write_n(fp, &pivot, sizeof(pivot));

    // =========================================================
    // 2) Meshes
    // =========================================================
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
        for (unsigned v = 0; v < numVerts; ++v)
        {
            verts[v].vPosition = { mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z };

            verts[v].vNormal = mesh->HasNormals() ?
                float3{ mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z } :
                float3{ 0,1,0 };

            verts[v].vTexUV = mesh->HasTextureCoords(0) ?
                float2{ (float)mesh->mTextureCoords[0][v].x, (float)mesh->mTextureCoords[0][v].y } :
                float2{ 0,0 };

            verts[v].vTangent = mesh->HasTangentsAndBitangents() ?
                float3{ mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z } :
                float3{ 1,0,0 };
        }
        write_n(fp, verts.data(), sizeof(VTXMODEL) * verts.size());

        unsigned numFaces = mesh->mNumFaces;
        write_n(fp, &numFaces, sizeof(numFaces));

        vector<FACEINDICES32> faces(numFaces);
        for (unsigned f = 0; f < numFaces; ++f)
        {
            faces[f] = {
                mesh->mFaces[f].mIndices[0],
                mesh->mFaces[f].mIndices[1],
                mesh->mFaces[f].mIndices[2]
            };
        }
        write_n(fp, faces.data(), sizeof(FACEINDICES32) * faces.size());
    }

    // =========================================================
    // 3) Materials  ★ 여기가 너 코드에서 틀렸던 부분
    //    HalfSphere.dat: (uint32 numMaterials) 후에
    //    material마다 AI_TEXTURE_TYPE_MAX개의 path[MAX_PATH] 저장
    // =========================================================
    unsigned numMaterials = scene->mNumMaterials;
    write_n(fp, &numMaterials, sizeof(numMaterials));

    for (unsigned m = 0; m < numMaterials; ++m)
    {
        aiMaterial* mat = scene->mMaterials[m];

        for (int t = 0; t < AI_TEXTURE_TYPE_MAX; ++t)
        {
            fs::path texPath = GetAssimpTexture(mat, (aiTextureType)t, texDir);

            // 존재 안하면 빈 문자열로 저장(엔진에서 nullptr 처리)
            if (!texPath.empty() && !fs::exists(texPath))
                texPath.clear();

            put_path_fixed_ansi(fp, texPath);
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

        for (const auto& dir : fs::recursive_directory_iterator(root))
        {
            if (!dir.is_directory()) continue;

            fs::path championPath = dir.path();
            fs::path championName = championPath.filename();

            fs::path fbxPath;
            for (const auto& f : fs::directory_iterator(championPath))
            {
                auto ext = f.path().extension().wstring();
                if (ext == L".fbx" || ext == L".FBX")
                {
                    fbxPath = f.path();
                    break;
                }
            }

            if (fbxPath.empty()) {
                wcout << L"[SKIP] " << championName << L" - FBX not found\n";
                continue;
            }

            fs::path relative = fs::relative(championPath, root);
            fs::path outDir = outputRoot / relative;
            fs::create_directories(outDir);

            fs::path outFile = outDir / (championName.wstring() + L".dat");
            if (fs::exists(outFile)) {
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
            if (!fp) {
                wcerr << L"[ERROR] cannot open output file: " << outFile << endl;
                continue;
            }

            // texDir = FBX 폴더
            WriteNonAnimModel_EngineFormat(fp, scene, fbxPath.parent_path());

            fclose(fp);

            wcout << L"[OK] " << relative << L" → " << outFile << endl;
        }

        wcout << L"\n=== 모든 NonAnim 모델 변환 완료 ===\n";
    }
    catch (const exception& e)
    {
        cerr << "[ERROR] " << e.what() << endl;
    }
    return 0;
}
