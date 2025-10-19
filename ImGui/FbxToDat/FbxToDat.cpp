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

// =============================================
// 기본 구조체
// =============================================
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

// =============================================
// 애니메이션 구조체
// =============================================
struct BINANIM {
    char szName[MAX_PATH];
    double dbDuration;
    double dbTickPerSecond;
    int iNumChannels;
};

#pragma pack(push, 4)
struct BINCHANNEL {
    char szName[MAX_PATH];
    int iNumKeyFrames;
};

struct KEYFRAME {
    float fTime;
    float vScale[3];
    float vRotation[4];
    float vPosition[3];
};
#pragma pack(pop)

// =============================================
// 유틸 함수
// =============================================
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

// =============================================
// 루트 Bone 찾기
// =============================================
const aiNode* FindRootBoneNode(const aiScene* scene) {
    if (!scene || scene->mNumMeshes == 0)
        return scene->mRootNode;

    const aiBone* firstBone = scene->mMeshes[0]->mBones[0];
    const aiNode* node = scene->mRootNode->FindNode(firstBone->mName);
    const aiNode* current = node;
    while (current && current->mParent && scene->mRootNode->FindNode(current->mParent->mName))
        current = current->mParent;
    return current ? current : scene->mRootNode;
}

// =============================================
// Bone 계층 저장 (스케일 중복 제거 / DirectX 행벡터 기준)
// =============================================
void WriteBoneRecursive(FILE* fp, const aiNode* node, const aiMatrix4x4& parentTransform)
{
    if (!fp || !node) return;

    // ? DirectX(Row-major) 기준 → Local * Parent 순서
    aiMatrix4x4 globalTransform = node->mTransformation * parentTransform;

    BINBONE bone{};
    strcpy_s(bone.szName, node->mName.C_Str());

    // ? DirectX 호환 Transpose 저장
    bone._11 = globalTransform.a1; bone._12 = globalTransform.b1; bone._13 = globalTransform.c1; bone._14 = globalTransform.d1;
    bone._21 = globalTransform.a2; bone._22 = globalTransform.b2; bone._23 = globalTransform.c2; bone._24 = globalTransform.d2;
    bone._31 = globalTransform.a3; bone._32 = globalTransform.b3; bone._33 = globalTransform.c3; bone._34 = globalTransform.d3;
    bone._41 = globalTransform.a4; bone._42 = globalTransform.b4; bone._43 = globalTransform.c4; bone._44 = globalTransform.d4;

    bone.iNumChildren = node->mNumChildren;
    write_n(fp, &bone, sizeof(BINBONE));

    cout << "[BoneWrite] " << bone.szName
        << " Pos(" << bone._41 << ", " << bone._42 << ", " << bone._43
        << ") Children=" << bone.iNumChildren << endl;

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
        WriteBoneRecursive(fp, node->mChildren[i], globalTransform);
}

// =============================================
// Mesh Bone Offset 저장
// =============================================
void WriteMeshBoneWeights(FILE* fp, const aiScene* scene) {
    unsigned int meshCount = scene->mNumMeshes;
    write_n(fp, &meshCount, sizeof(meshCount));

    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* mesh = scene->mMeshes[m];
        unsigned int numBones = mesh->mNumBones;
        write_n(fp, &numBones, sizeof(numBones));

        for (unsigned int b = 0; b < numBones; ++b) {
            const aiBone* bone = mesh->mBones[b];
            char name[MAX_PATH]{}; strcpy_s(name, bone->mName.C_Str());
            write_n(fp, name, sizeof(name));

            FLOAT4X4 offset{};
            const aiMatrix4x4& mtx = bone->mOffsetMatrix;
            offset.m[0] = mtx.a1; offset.m[1] = mtx.a2; offset.m[2] = mtx.a3; offset.m[3] = mtx.a4;
            offset.m[4] = mtx.b1; offset.m[5] = mtx.b2; offset.m[6] = mtx.b3; offset.m[7] = mtx.b4;
            offset.m[8] = mtx.c1; offset.m[9] = mtx.c2; offset.m[10] = mtx.c3; offset.m[11] = mtx.c4;
            offset.m[12] = mtx.d1; offset.m[13] = mtx.d2; offset.m[14] = mtx.d3; offset.m[15] = mtx.d4;
            write_n(fp, &offset, sizeof(FLOAT4X4));
        }
    }
}

// =============================================
// Model.dat (Pivot 스케일만 적용, 본 스케일 X)
// =============================================
void WriteModelData(FILE* fp, const aiScene* scene) {
    // ? PivotMatrix: 전체 모델 단위 축소 (0.01배)
    FLOAT4X4 pivot{};
    for (int i = 0; i < 16; ++i)
        pivot.m[i] = (i % 5 == 0) ? 0.01f : 0.f;
    pivot.m[15] = 1.f;
    write_n(fp, &pivot, sizeof(pivot));

    unsigned int numMeshes = scene->mNumMeshes;
    write_n(fp, &numMeshes, sizeof(numMeshes));

    for (unsigned int mi = 0; mi < numMeshes; ++mi) {
        const aiMesh* mesh = scene->mMeshes[mi];
        string meshName = mesh->mName.length ? mesh->mName.C_Str() : ("Mesh_" + to_string(mi));
        put_str_fixed(fp, meshName);

        unsigned int matIndex = mesh->mMaterialIndex;
        write_n(fp, &matIndex, sizeof(matIndex));

        unsigned int numVerts = mesh->mNumVertices;
        write_n(fp, &numVerts, sizeof(numVerts));

        vector<VTXANIMMODEL> verts(numVerts);
        for (unsigned int v = 0; v < numVerts; ++v) {
            verts[v].vPosition = { mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z };
            verts[v].vNormal = mesh->HasNormals() ? float3{ mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z } : float3{ 0,1,0 };
            verts[v].vTexUV = mesh->HasTextureCoords(0) ? float2{ (float)mesh->mTextureCoords[0][v].x, 1.f - (float)mesh->mTextureCoords[0][v].y } : float2{ 0,0 };
            verts[v].vTangent = mesh->HasTangentsAndBitangents() ? float3{ mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z } : float3{ 1,0,0 };
        }
        write_n(fp, verts.data(), sizeof(VTXANIMMODEL) * verts.size());

        unsigned int numBones = mesh->mNumBones;
        write_n(fp, &numBones, sizeof(numBones));

        unsigned int numFaces = mesh->mNumFaces;
        write_n(fp, &numFaces, sizeof(numFaces));

        vector<FACEINDICES32> faces(numFaces);
        for (unsigned int f = 0; f < numFaces; ++f)
            faces[f] = { mesh->mFaces[f].mIndices[0], mesh->mFaces[f].mIndices[1], mesh->mFaces[f].mIndices[2] };
        write_n(fp, faces.data(), sizeof(FACEINDICES32) * faces.size());
    }

    // ? 머티리얼 경로 저장
    unsigned int numMaterials = scene->mNumMaterials;
    write_n(fp, &numMaterials, sizeof(numMaterials));

    for (unsigned int mi = 0; mi < numMaterials; ++mi) {
        aiMaterial* mat = scene->mMaterials[mi];
        auto writeTex = [&](aiTextureType type) {
            aiString path;
            char fullPath[MAX_PATH]{};

            if (mat->GetTexture(type, 0, &path) == AI_SUCCESS) {
                fs::path texPath = fs::u8path(path.C_Str());
                if (!texPath.is_absolute())
                    texPath = fs::path("C:\\DirectX11_LeagueOfLegends\\Bin\\Resources\\Meshes\\Anim\\Champion\\Ezreal") / texPath.filename();
                strncpy_s(fullPath, texPath.string().c_str(), MAX_PATH - 1);
            }
            write_n(fp, fullPath, sizeof(fullPath));
            };
        writeTex(aiTextureType_DIFFUSE);
        writeTex(aiTextureType_NORMALS);
    }
}

// =============================================
// Animation.dat
// =============================================
void WriteAnimations(FILE* fp, const aiScene* scene) {
    unsigned int animCount = scene->mNumAnimations;
    fwrite(&animCount, sizeof(unsigned int), 1, fp);

    for (unsigned int i = 0; i < animCount; ++i) {
        const aiAnimation* anim = scene->mAnimations[i];
        BINANIM animHeader{};
        strcpy_s(animHeader.szName, anim->mName.length ? anim->mName.C_Str() : ("Anim_" + to_string(i)).c_str());
        animHeader.dbDuration = static_cast<double>(anim->mDuration);
        animHeader.dbTickPerSecond = (anim->mTicksPerSecond != 0.0) ? static_cast<double>(anim->mTicksPerSecond) : 30.0;
        animHeader.iNumChannels = static_cast<int>(anim->mNumChannels);
        fwrite(&animHeader, sizeof(BINANIM), 1, fp);

        for (int c = 0; c < animHeader.iNumChannels; ++c) {
            const aiNodeAnim* channel = anim->mChannels[c];
            BINCHANNEL ch{};
            strcpy_s(ch.szName, channel->mNodeName.C_Str());
            ch.iNumKeyFrames = static_cast<int>(max({
                channel->mNumPositionKeys,
                channel->mNumRotationKeys,
                channel->mNumScalingKeys }));
            fwrite(&ch, sizeof(BINCHANNEL), 1, fp);

            for (int k = 0; k < ch.iNumKeyFrames; ++k) {
                KEYFRAME frame{};
                frame.fTime = (k < (int)channel->mNumPositionKeys)
                    ? static_cast<float>(channel->mPositionKeys[k].mTime)
                    : 0.f;
                if (k < (int)channel->mNumScalingKeys) {
                    frame.vScale[0] = channel->mScalingKeys[k].mValue.x;
                    frame.vScale[1] = channel->mScalingKeys[k].mValue.y;
                    frame.vScale[2] = channel->mScalingKeys[k].mValue.z;
                }
                else frame.vScale[0] = frame.vScale[1] = frame.vScale[2] = 1.f;
                if (k < (int)channel->mNumRotationKeys) {
                    frame.vRotation[0] = channel->mRotationKeys[k].mValue.x;
                    frame.vRotation[1] = channel->mRotationKeys[k].mValue.y;
                    frame.vRotation[2] = channel->mRotationKeys[k].mValue.z;
                    frame.vRotation[3] = channel->mRotationKeys[k].mValue.w;
                }
                else frame.vRotation[3] = 1.f;
                if (k < (int)channel->mNumPositionKeys) {
                    frame.vPosition[0] = channel->mPositionKeys[k].mValue.x;
                    frame.vPosition[1] = channel->mPositionKeys[k].mValue.y;
                    frame.vPosition[2] = channel->mPositionKeys[k].mValue.z;
                }
                fwrite(&frame, sizeof(KEYFRAME), 1, fp);
            }
        }
    }
}

// =============================================
// Main
// =============================================
int wmain() {
    try {
        fs::path inputFBX = L"C:\\DirectX11_LeagueOfLegends\\Bin\\Resources\\Meshes\\Anim\\Champion\\Ezreal\\IzrealBase.fbx";
        fs::path outModel = L"C:\\DirectX11_LeagueOfLegends\\Bin\\Bin_Data\\Anim\\Champion\\Izreal\\Izreal.dat";
        fs::path outAnim = L"C:\\DirectX11_LeagueOfLegends\\Bin\\Bin_Data\\Anim\\Champion\\Izreal\\Izreal_Animation.dat";

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            inputFBX.string(),
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_GenNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_ConvertToLeftHanded
        );
        if (!scene) throw runtime_error(importer.GetErrorString());

        FILE* fp = nullptr;
        _wfopen_s(&fp, outModel.c_str(), L"wb");
        WriteModelData(fp, scene);
        fclose(fp);

        FILE* fa = nullptr;
        _wfopen_s(&fa, outAnim.c_str(), L"wb");
        const aiNode* root = FindRootBoneNode(scene);
        aiMatrix4x4 identity;
        WriteBoneRecursive(fa, root, identity);
        WriteMeshBoneWeights(fa, scene);
        WriteAnimations(fa, scene);
        fclose(fa);

        cout << "[OK] 변환 완료 ? sizeof(KEYFRAME)=" << sizeof(KEYFRAME) << endl;
        return 0;
    }
    catch (exception& e) {
        cerr << "[ERROR] " << e.what() << endl;
        return 1;
    }
}
















