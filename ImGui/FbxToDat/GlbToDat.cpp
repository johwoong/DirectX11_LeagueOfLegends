#define NOMINMAX
#include <Windows.h>
#include <string>
#include <filesystem>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#pragma pack(push, 1)
struct float2 { float x, y; };
struct float3 { float x, y, z; };
struct FLOAT4X4 { float m[16]; };
struct BINBONE {
    char szName[MAX_PATH];
    float _11, _12, _13, _14;
    float _21, _22, _23, _24;
    float _31, _32, _33, _34;
    float _41, _42, _43, _44;
    unsigned int iNumChildren;
};
struct VTXANIMMODEL {
    float3 vPosition;
    float3 vNormal;
    float2 vTexUV;
    float3 vTangent;
    unsigned int iBlendIndices[4];
    float fBlendWeights[4];
};
struct FACEINDICES32 { unsigned int _0, _1, _2; };
#pragma pack(pop)

using namespace std;
namespace fs = std::filesystem;

static void write_n(FILE* fp, const void* p, size_t sz) {
    if (fwrite(p, 1, sz, fp) != sz)
        throw runtime_error("file write error");
}
static void put_str_fixed(FILE* fp, const string& s) {
    char buf[MAX_PATH]{};
    string fixed = s;
    replace(fixed.begin(), fixed.end(), '/', '\\');
    strncpy_s(buf, fixed.c_str(), MAX_PATH - 1);
    fwrite(buf, sizeof(char), MAX_PATH, fp);
}

void WriteBoneRecursive(FILE* fp, const aiNode* node) {
    BINBONE bone{};
    strcpy_s(bone.szName, node->mName.C_Str());
    const aiMatrix4x4& m = node->mTransformation;
    bone._11 = m.a1; bone._12 = m.a2; bone._13 = m.a3; bone._14 = m.a4;
    bone._21 = m.b1; bone._22 = m.b2; bone._23 = m.b3; bone._24 = m.b4;
    bone._31 = m.c1; bone._32 = m.c2; bone._33 = m.c3; bone._34 = m.c4;
    bone._41 = m.d1; bone._42 = m.d2; bone._43 = m.d3; bone._44 = m.d4;
    bone.iNumChildren = node->mNumChildren;
    write_n(fp, &bone, sizeof(BINBONE));

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
        WriteBoneRecursive(fp, node->mChildren[i]);
}

int wmain() {
    try {
        std::ios::sync_with_stdio(false);
        std::wcout.imbue(std::locale(""));

        fs::path inputFBX = L"C:\\DirectX11_LeagueOfLegends\\Bin\\Resources\\Meshes\\Anim\\Nexus\\Nexus.fbx";
        fs::path outModel = L"C:\\DirectX11_LeagueOfLegends\\Bin\\Bin_Data\\Anim\\Nexus\\Nexus.dat";
        fs::path outAnim = L"C:\\DirectX11_LeagueOfLegends\\Bin\\Bin_Data\\Anim\\Nexus\\Nexus_Animation.dat";

        fs::create_directories(outModel.parent_path());
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            inputFBX.string(),
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_GenNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_ConvertToLeftHanded |
            aiProcess_FlipUVs
        );
        if (!scene) throw runtime_error(importer.GetErrorString());

        wcout << L"[FBX LOAD] Meshes=" << scene->mNumMeshes
            << L", Anims=" << scene->mNumAnimations
            << L", Materials=" << scene->mNumMaterials << endl;

        // ───────────────────────────────
        // MODEL.DAT 작성
        // ───────────────────────────────
        FILE* fp = nullptr;
        if (_wfopen_s(&fp, outModel.c_str(), L"wb") || !fp)
            throw runtime_error("Model.dat 생성 실패");

        FLOAT4X4 pivot{};
        for (int i = 0; i < 16; ++i)
            pivot.m[i] = (i % 5 == 0) ? 1.f : 0.f;
        write_n(fp, &pivot, sizeof(pivot));

        unsigned int numMeshes = scene->mNumMeshes;
        write_n(fp, &numMeshes, sizeof(numMeshes));

        for (unsigned int mi = 0; mi < numMeshes; ++mi) {
            const aiMesh* mesh = scene->mMeshes[mi];
            string name = mesh->mName.length ? mesh->mName.C_Str() : ("Mesh_" + to_string(mi));
            wcout << L"[MESH] " << name.c_str() << endl;
            put_str_fixed(fp, name);

            unsigned int matIdx = mesh->mMaterialIndex;
            write_n(fp, &matIdx, sizeof(matIdx));

            unsigned int vCount = mesh->mNumVertices;
            write_n(fp, &vCount, sizeof(vCount));

            vector<VTXANIMMODEL> verts(vCount);
            for (unsigned int v = 0; v < vCount; ++v) {
                verts[v].vPosition = { mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z };
                verts[v].vNormal = mesh->HasNormals() ?
                    float3{ mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z } : float3{ 0,1,0 };
                verts[v].vTexUV = mesh->HasTextureCoords(0) ?
                    float2{ (float)mesh->mTextureCoords[0][v].x, (float)mesh->mTextureCoords[0][v].y } : float2{ 0,0 };
                verts[v].vTangent = mesh->HasTangentsAndBitangents() ?
                    float3{ mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z } : float3{ 1,0,0 };
                for (int j = 0; j < 4; ++j) { verts[v].iBlendIndices[j] = 0; verts[v].fBlendWeights[j] = 0.f; }
            }
            write_n(fp, verts.data(), sizeof(VTXANIMMODEL) * verts.size());

            unsigned int boneCount = mesh->mNumBones;
            write_n(fp, &boneCount, sizeof(boneCount));

            unsigned int faceCount = mesh->mNumFaces;
            write_n(fp, &faceCount, sizeof(faceCount));

            vector<FACEINDICES32> faces(faceCount);
            for (unsigned int f = 0; f < faceCount; ++f)
                faces[f] = { mesh->mFaces[f].mIndices[0], mesh->mFaces[f].mIndices[1], mesh->mFaces[f].mIndices[2] };
            write_n(fp, faces.data(), sizeof(FACEINDICES32) * faces.size());
        }

        // 머티리얼
        unsigned int matCount = scene->mNumMaterials;
        write_n(fp, &matCount, sizeof(matCount));

        for (unsigned int i = 0; i < matCount; ++i) {
            aiMaterial* mat = scene->mMaterials[i];
            aiString tex;
            string texPath;

            if (AI_SUCCESS == mat->GetTexture(aiTextureType_DIFFUSE, 0, &tex)) {
                fs::path texFull = inputFBX.parent_path() / tex.C_Str();
                texFull = texFull.lexically_normal();
                texPath = texFull.string();
            }

            if (texPath.empty() || !fs::exists(texPath))
                texPath = "C:\\DirectX11_LeagueOfLegends\\Bin\\Resources\\Meshes\\Anim\\Nexus\\nexus_tx_cm_red.png";

            put_str_fixed(fp, texPath);
            put_str_fixed(fp, texPath);
        }

        // ✅ 애니메이션 개수도 MODEL.DAT 끝에 추가 저장
        unsigned int animCount = scene->mNumAnimations;
        write_n(fp, &animCount, sizeof(animCount));

        fclose(fp);
        wcout << L"[OK] Model.dat 저장 완료 (" << animCount << L" animations info 포함)" << endl;

        // ───────────────────────────────
        // ANIMATION.DAT 작성
        // ───────────────────────────────
        FILE* fa = nullptr;
        if (_wfopen_s(&fa, outAnim.c_str(), L"wb") || !fa)
            throw runtime_error("Animation.dat 생성 실패");

        WriteBoneRecursive(fa, scene->mRootNode);
        write_n(fa, &animCount, sizeof(animCount));

        for (unsigned int i = 0; i < animCount; ++i) {
            const aiAnimation* anim = scene->mAnimations[i];
            string animName = anim->mName.length ? anim->mName.C_Str() : ("Anim_" + to_string(i));
            put_str_fixed(fa, animName);

            double dur = anim->mDuration;
            double tps = anim->mTicksPerSecond != 0.0 ? anim->mTicksPerSecond : 30.0;
            write_n(fa, &dur, sizeof(dur));
            write_n(fa, &tps, sizeof(tps));
        }

        fclose(fa);
        wcout << L"[OK] Animation.dat 생성 완료" << endl;

        system("pause");
    }
    catch (const std::exception& e) {
        cerr << "[ERROR] " << e.what() << endl;
        system("pause");
    }
}
