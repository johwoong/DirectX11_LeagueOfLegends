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

struct VTXANIMMODEL {
    float3 vPosition;
    float3 vNormal;
    float2 vTexUV;
    float3 vTangent;
    unsigned int iBlendIndices[4];
    float fBlendWeights[4];
};

struct BINBONE { char szName[MAX_PATH]; FLOAT4X4 Transformation; unsigned int iNumChildren; };
struct BINANIM { char szName[MAX_PATH]; double dbDuration; double dbTickPerSecond; int iNumChannels; };
struct BINCHANNEL { char szName[MAX_PATH]; int iNumKeyFrames; };
struct KEYFRAME { float fTime; float vScale[3]; float vRotation[4]; float vPosition[3]; };


// ============================= UTIL =============================
static void write_n(FILE* fp, const void* p, size_t sz) {
    if (fwrite(p, 1, sz, fp) != sz) throw runtime_error("file write error");
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
static fs::path FindAnyTexture(const fs::path& folder) {
    if (!fs::exists(folder) || !fs::is_directory(folder)) return {};
    vector<string> exts = { ".png",".dds",".jpg",".jpeg",".tga" };
    for (auto& e : fs::directory_iterator(folder)) {
        if (!e.is_regular_file()) continue;
        string ext = e.path().extension().string();
        transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (find(exts.begin(), exts.end(), ext) != exts.end())
            return e.path();
    }
    return {};
}


// ============================= Model.dat =============================
static void WriteModelData(FILE* fp, const aiScene* scene, const fs::path& texDir)
{
    FLOAT4X4 pivot{};

    // =========================================================
    //   ★★★ 좌우반전만 추가 (X축 -1 스케일) ★★★
    // =========================================================
    float flipX[16] =
    {
        -1, 0, 0, 0,
         0, 1, 0, 0,
         0, 0, 1, 0,
         0, 0, 0, 1
    };

    memcpy(pivot.m, flipX, sizeof(flipX));
    // =========================================================

    write_n(fp, &pivot, sizeof(pivot));

    // mesh export
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

        vector<VTXANIMMODEL> verts(numVerts);
        for (unsigned v = 0; v < numVerts; ++v)
        {
            verts[v].vPosition = { mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z };
            verts[v].vNormal = mesh->HasNormals() ? float3{ mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z } : float3{ 0,1,0 };
            verts[v].vTexUV = mesh->HasTextureCoords(0) ? float2{ (float)mesh->mTextureCoords[0][v].x, (float)mesh->mTextureCoords[0][v].y } : float2{ 0,0 };
            verts[v].vTangent = mesh->HasTangentsAndBitangents() ? float3{ mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z } : float3{ 1,0,0 };

            for (int i = 0; i < 4; ++i) {
                verts[v].iBlendIndices[i] = 0;
                verts[v].fBlendWeights[i] = 0.f;
            }
        }

        // Bone Weights
        for (unsigned b = 0; b < mesh->mNumBones; ++b)
        {
            const aiBone* bone = mesh->mBones[b];
            for (unsigned w = 0; w < bone->mNumWeights; ++w)
            {
                unsigned vId = bone->mWeights[w].mVertexId;
                float weight = bone->mWeights[w].mWeight;

                for (int i = 0; i < 4; ++i)
                {
                    if (verts[vId].fBlendWeights[i] == 0.f)
                    {
                        verts[vId].iBlendIndices[i] = b;
                        verts[vId].fBlendWeights[i] = weight;
                        break;
                    }
                }
            }
        }

        write_n(fp, verts.data(), sizeof(VTXANIMMODEL) * verts.size());

        unsigned numBones = mesh->mNumBones;
        write_n(fp, &numBones, sizeof(numBones));

        unsigned numFaces = mesh->mNumFaces;
        write_n(fp, &numFaces, sizeof(numFaces));

        vector<FACEINDICES32> faces(numFaces);
        for (unsigned f = 0; f < numFaces; ++f) {
            faces[f] = { mesh->mFaces[f].mIndices[0], mesh->mFaces[f].mIndices[1], mesh->mFaces[f].mIndices[2] };
        }

        write_n(fp, faces.data(), sizeof(FACEINDICES32) * faces.size());
    }


    // material export
    unsigned numMats = scene->mNumMaterials;
    write_n(fp, &numMats, sizeof(numMats));

    fs::path autoTex = FindAnyTexture(texDir);
    string autoTexStr = autoTex.empty()
        ? (texDir / L"default_diffuse.dds").string()
        : autoTex.string();

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
            else {
                strncpy_s(path, autoTexStr.c_str(), MAX_PATH - 1);
            }

            write_n(fp, path, sizeof(path));
        }
    }
}


// ============================= Animation.dat =============================
static void WriteBonesRecursive(FILE* fp, const aiNode* node) {
    BINBONE bone{};
    strncpy_s(bone.szName, node->mName.C_Str(), MAX_PATH - 1);
    ToF4x4(bone.Transformation, node->mTransformation);
    bone.iNumChildren = node->mNumChildren;
    write_n(fp, &bone, sizeof(bone));

    for (unsigned i = 0; i < node->mNumChildren; ++i)
        WriteBonesRecursive(fp, node->mChildren[i]);
}

static void WriteMeshBoneOffsets(FILE* fp, const aiScene* scene) {
    for (unsigned m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* mesh = scene->mMeshes[m];
        for (unsigned b = 0; b < mesh->mNumBones; ++b) {
            const aiBone* bone = mesh->mBones[b];
            char name[MAX_PATH]{}; strncpy_s(name, bone->mName.C_Str(), MAX_PATH - 1);
            write_n(fp, name, sizeof(name));

            FLOAT4X4 offset{};
            ToF4x4(offset, bone->mOffsetMatrix);
            write_n(fp, &offset, sizeof(offset));
        }
    }
}

static void WriteAnimations(FILE* fp, const aiScene* scene) {
    unsigned animCount = scene->mNumAnimations;
    write_n(fp, &animCount, sizeof(animCount));

    for (unsigned a = 0; a < animCount; ++a)
    {
        const aiAnimation* anim = scene->mAnimations[a];

        BINANIM ba{};
        strncpy_s(ba.szName, anim->mName.C_Str(), MAX_PATH - 1);
        ba.dbDuration = anim->mDuration;
        ba.dbTickPerSecond = anim->mTicksPerSecond != 0 ? anim->mTicksPerSecond : 30.0;
        ba.iNumChannels = anim->mNumChannels;
        write_n(fp, &ba, sizeof(ba));

        for (unsigned c = 0; c < anim->mNumChannels; ++c)
        {
            const aiNodeAnim* ch = anim->mChannels[c];

            BINCHANNEL bc{};
            strncpy_s(bc.szName, ch->mNodeName.C_Str(), MAX_PATH - 1);
            bc.iNumKeyFrames = max({
                (int)ch->mNumPositionKeys,
                (int)ch->mNumRotationKeys,
                (int)ch->mNumScalingKeys
                });
            write_n(fp, &bc, sizeof(bc));

            for (int i = 0; i < bc.iNumKeyFrames; ++i)
            {
                KEYFRAME kf{};
                kf.fTime = (i < (int)ch->mNumPositionKeys) ? (float)ch->mPositionKeys[i].mTime : 0.f;

                if (i < (int)ch->mNumScalingKeys) {
                    kf.vScale[0] = (float)ch->mScalingKeys[i].mValue.x;
                    kf.vScale[1] = (float)ch->mScalingKeys[i].mValue.y;
                    kf.vScale[2] = (float)ch->mScalingKeys[i].mValue.z;
                }
                else {
                    kf.vScale[0] = kf.vScale[1] = kf.vScale[2] = 1.f;
                }

                if (i < (int)ch->mNumRotationKeys) {
                    kf.vRotation[0] = (float)ch->mRotationKeys[i].mValue.x;
                    kf.vRotation[1] = (float)ch->mRotationKeys[i].mValue.y;
                    kf.vRotation[2] = (float)ch->mRotationKeys[i].mValue.z;
                    kf.vRotation[3] = (float)ch->mRotationKeys[i].mValue.w;
                }
                else {
                    kf.vRotation[0] = kf.vRotation[1] = kf.vRotation[2] = 0.f;
                    kf.vRotation[3] = 1.f;
                }

                if (i < (int)ch->mNumPositionKeys) {
                    kf.vPosition[0] = (float)ch->mPositionKeys[i].mValue.x;
                    kf.vPosition[1] = (float)ch->mPositionKeys[i].mValue.y;
                    kf.vPosition[2] = (float)ch->mPositionKeys[i].mValue.z;
                }

                write_n(fp, &kf, sizeof(kf));
            }
        }
    }
}

static void WriteAnimationData(FILE* fp, const aiScene* scene) {
    WriteBonesRecursive(fp, scene->mRootNode);
    WriteMeshBoneOffsets(fp, scene);
    WriteAnimations(fp, scene);
}


// ============================= Main =============================
int wmain()
{
    try {
        fs::path srcRoot = L"C:\\DirectX11_LeagueOfLegends\\Bin\\Resources\\Meshes\\Anim\\Champion";
        fs::path dstRoot = L"C:\\DirectX11_LeagueOfLegends\\Bin\\Bin_Data\\Anim\\Champion";

        if (!fs::exists(srcRoot))
            throw runtime_error("Champion folder not found");

        for (auto& entry : fs::recursive_directory_iterator(srcRoot))
        {
            if (!entry.is_directory()) continue;

            fs::path fbx;
            for (auto& f : fs::directory_iterator(entry.path()))
            {
                auto ext = f.path().extension().wstring();
                if (ext == L".fbx" || ext == L".FBX") {
                    fbx = f.path();
                    break;
                }
            }
            if (fbx.empty()) continue;

            fs::path relative = fs::relative(entry.path(), srcRoot);
            fs::path outDir = dstRoot / relative;
            fs::create_directories(outDir);

            fs::path baseName = entry.path().filename();
            fs::path outModel = outDir / (baseName.wstring() + L".dat");
            fs::path outAnim = outDir / (baseName.wstring() + L"_Animation.dat");

            // 이미 있으면 생성 안함
            if (fs::exists(outModel) && fs::exists(outAnim)) {
                wcout << L"[SKIP] " << relative << endl;
                continue;
            }

            // FBX Load
            Assimp::Importer imp;
            const aiScene* scene = imp.ReadFile(
                fbx.string(),
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

            if (!fs::exists(outModel))
            {
                FILE* fm = nullptr;
                _wfopen_s(&fm, outModel.c_str(), L"wb");
                WriteModelData(fm, scene, fbx.parent_path());
                fclose(fm);
            }

            if (!fs::exists(outAnim))
            {
                FILE* fa = nullptr;
                _wfopen_s(&fa, outAnim.c_str(), L"wb");
                WriteAnimationData(fa, scene);
                fclose(fa);
            }

            wcout << L"[OK] " << relative << endl;
        }

    }
    catch (const exception& e) {
        cerr << "[ERROR] " << e.what() << endl;
    }

    return 0;
}
