#include <windows.h>
#include <string>
#include <filesystem>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#pragma pack(push, 1)
struct float2 { float x, y; };
struct float3 { float x, y, z; };
struct FLOAT4X4 { float m[16]; };

// 애니메이션 메시용 정점 구조체
struct VTXANIMMODEL {
    float3 vPosition;
    float3 vNormal;
    float2 vTexUV;
    float3 vTangent;
    uint32_t iBlendIndices[4];
    float fBlendWeights[4];
};

struct FACEINDICES32 { uint32_t _0, _1, _2; };
#pragma pack(pop)

using namespace std;
namespace fs = std::filesystem;

static void write_n(FILE* fp, const void* p, size_t sz) {
    if (fwrite(p, 1, sz, fp) != sz) throw runtime_error("file write error");
}
static void put_str_fixed(FILE* fp, const string& s) {
    char buf[MAX_PATH]{};
    string fixed = s;
    replace(fixed.begin(), fixed.end(), '/', '\\');
    strncpy_s(buf, fixed.c_str(), MAX_PATH - 1);
    fwrite(buf, sizeof(char), MAX_PATH, fp);
}

int wmain() {
    try {
        fs::path inputFBX = L"C:\\DirectX11_LeagueOfLegends\\Bin\\Resources\\Meshes\\Anim\\Nexus\\Nexus.fbx";
        fs::path outModel = L"C:\\DirectX11_LeagueOfLegends\\Bin\\Bin_Data\\Anim\\Nexus\\Nexus.dat";
        fs::path outAnim = L"C:\\DirectX11_LeagueOfLegends\\Bin\\Bin_Data\\Anim\\Nexus\\Nexus_Animation.dat";

        if (!fs::exists(inputFBX)) {
            wcerr << L"[ERROR] FBX 파일이 존재하지 않습니다: " << inputFBX << endl;
            return 11;
        }
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
        if (!scene || !scene->mRootNode) {
            cerr << "[ERROR] Assimp failed: " << importer.GetErrorString() << endl;
            return 2;
        }

        FILE* fp = nullptr;
        if (_wfopen_s(&fp, outModel.c_str(), L"wb") || !fp)
            throw runtime_error("dat 파일 생성 실패");

        // Pivot (항등행렬)
        FLOAT4X4 pivot{};
        for (int i = 0; i < 16; ++i) pivot.m[i] = (i % 5 == 0) ? 1.f : 0.f;
        write_n(fp, &pivot, sizeof(pivot));

        uint32_t numMeshes = scene->mNumMeshes;
        write_n(fp, &numMeshes, sizeof(numMeshes));

        for (uint32_t mi = 0; mi < numMeshes; ++mi) {
            const aiMesh* mesh = scene->mMeshes[mi];
            string name = mesh->mName.length ? mesh->mName.C_Str() : ("Mesh_" + to_string(mi));
            put_str_fixed(fp, name);

            uint32_t matIndex = mesh->mMaterialIndex;
            write_n(fp, &matIndex, sizeof(matIndex));

            uint32_t numVerts = mesh->mNumVertices;
            write_n(fp, &numVerts, sizeof(numVerts));

            vector<VTXANIMMODEL> verts(numVerts);
            for (uint32_t i = 0; i < numVerts; ++i) {
                verts[i].vPosition = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
                verts[i].vNormal = mesh->HasNormals() ? float3{ mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z } : float3{ 0,1,0 };
                verts[i].vTexUV = mesh->HasTextureCoords(0) ? float2{ (float)mesh->mTextureCoords[0][i].x, (float)mesh->mTextureCoords[0][i].y } : float2{ 0,0 };
                verts[i].vTangent = mesh->HasTangentsAndBitangents() ? float3{ mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z } : float3{ 1,0,0 };
                for (int j = 0; j < 4; ++j) { verts[i].iBlendIndices[j] = 0; verts[i].fBlendWeights[j] = 0.f; }
            }

            // 본 가중치 매핑
            if (mesh->HasBones()) {
                for (uint32_t b = 0; b < mesh->mNumBones; ++b) {
                    const aiBone* bone = mesh->mBones[b];
                    for (uint32_t w = 0; w < bone->mNumWeights; ++w) {
                        uint32_t vIndex = bone->mWeights[w].mVertexId;
                        float weight = bone->mWeights[w].mWeight;
                        for (int j = 0; j < 4; ++j) {
                            if (verts[vIndex].fBlendWeights[j] == 0.f) {
                                verts[vIndex].iBlendIndices[j] = b;
                                verts[vIndex].fBlendWeights[j] = weight;
                                break;
                            }
                        }
                    }
                }
            }

            write_n(fp, verts.data(), sizeof(VTXANIMMODEL) * verts.size());

            // 본 개수 기록
            uint32_t numBones = mesh->mNumBones;
            write_n(fp, &numBones, sizeof(numBones));

            // 인덱스 기록
            uint32_t numFaces = mesh->mNumFaces;
            write_n(fp, &numFaces, sizeof(numFaces));

            vector<FACEINDICES32> faces(numFaces);
            for (uint32_t f = 0; f < numFaces; ++f)
                faces[f] = { mesh->mFaces[f].mIndices[0], mesh->mFaces[f].mIndices[1], mesh->mFaces[f].mIndices[2] };
            write_n(fp, faces.data(), sizeof(FACEINDICES32) * faces.size());

            printf("[Mesh %u] %s | verts=%u | bones=%u | faces=%u\n", mi, name.c_str(), numVerts, mesh->mNumBones, numFaces);
        }

        // 머티리얼
        uint32_t numMats = scene->mNumMaterials;
        write_n(fp, &numMats, sizeof(numMats));

        fs::path blueTex = L"C:\\DirectX11_LeagueOfLegends\\Bin\\Resources\\Meshes\\Anim\\Nexus\\nexus_tx_cm_blue.png";
        string texPath = blueTex.u8string();
        for (uint32_t i = 0; i < numMats; ++i) {
            put_str_fixed(fp, texPath); // diffuse
            put_str_fixed(fp, texPath); // normal
        }

        fclose(fp);
        wcout << L"[OK] 모델 dat 생성 완료: " << outModel << endl;

        // 애니메이션 데이터
        if (scene->mNumAnimations > 0) {
            FILE* fa = nullptr;
            if (_wfopen_s(&fa, outAnim.c_str(), L"wb") || !fa)
                throw runtime_error("animation.dat 파일 생성 실패");

            uint32_t animCount = scene->mNumAnimations;
            write_n(fa, &animCount, sizeof(animCount));

            for (uint32_t i = 0; i < animCount; ++i) {
                const aiAnimation* anim = scene->mAnimations[i];
                string animName = anim->mName.length ? anim->mName.C_Str() : ("Anim_" + to_string(i));
                put_str_fixed(fa, animName);

                double duration = anim->mDuration;
                double ticks = anim->mTicksPerSecond;
                write_n(fa, &duration, sizeof(duration));
                write_n(fa, &ticks, sizeof(ticks));
            }

            fclose(fa);
            wcout << L"[OK] 애니메이션 dat 생성 완료: " << outAnim << endl;
        }
        else {
            wcout << L"[INFO] 애니메이션 없음 (Animation.dat 생략)" << endl;
        }

        wcout << L"[DONE] 변환 완료!" << endl;
        return 0;
    }
    catch (const std::exception& e) {
        cerr << "[EXCEPTION] " << e.what() << endl;
        return 10;
    }
}
