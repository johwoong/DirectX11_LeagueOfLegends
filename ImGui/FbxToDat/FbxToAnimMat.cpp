
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

// ============================= 공통 유틸 =============================
static void write_n(FILE* fp, const void* p, size_t sz) {
    if (fwrite(p, 1, sz, fp) != sz) throw runtime_error("file write error");
}
static void put_str_empty_fixed(FILE* fp)
{
    char buf[MAX_PATH]{};
    // 전부 0 = 빈 문자열
    write_n(fp, buf, sizeof(buf));
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
    // 이름 힌트 우선 (base/albedo/diffuse)
    vector<string> prefer = { "base","albedo","diff","diffuse","_cm","color" };
    vector<string> exts = { ".png",".dds",".jpg",".jpeg",".tga" };
    fs::path fallback;
    for (auto& e : fs::directory_iterator(folder)) {
        if (!e.is_regular_file()) continue;
        string name = e.path().filename().string();
        string ext = e.path().extension().string();
        transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (find(exts.begin(), exts.end(), ext) == exts.end()) continue;
        string lower = name; transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        for (auto& key : prefer) if (lower.find(key) != string::npos) return e.path();
        if (fallback.empty()) fallback = e.path();
    }
    return fallback; // 없으면 빈 경로
}


static void WriteModelData(FILE* fp, const aiScene* scene, const fs::path& texDir)
{
    FLOAT4X4 pivot{};

    // Y축 180도 회전 매트릭스 (직접 작성)
    float rotY[16] =
    {
        1, 0,  0, 0,
         0, 1,  0, 0,
         0, 0, 1, 0,
         0, 0,  0, 1
    };

    // pivot에 복사
    for (int i = 0; i < 16; ++i)
        pivot.m[i] = rotY[i];

    // 스케일 적용 (0.01)
    pivot.m[0] *= 0.01f;   // X scale
    pivot.m[5] *= 0.01f;   // Y scale
    pivot.m[10] *= 0.01f;   // Z scale

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

        vector<VTXANIMMODEL> verts(numVerts);
        for (unsigned v = 0; v < numVerts; ++v)
        {
            verts[v].vPosition = { mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z };
            verts[v].vNormal = mesh->HasNormals() ? float3{ mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z } : float3{ 0,1,0 };
            verts[v].vTexUV = mesh->HasTextureCoords(0) ? float2{ (float)mesh->mTextureCoords[0][v].x, (float)mesh->mTextureCoords[0][v].y } : float2{ 0,0 };
            verts[v].vTangent = mesh->HasTangentsAndBitangents() ? float3{ mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z } : float3{ 1,0,0 };

            for (int i = 0; i < 4; ++i)
            {
                verts[v].iBlendIndices[i] = 0;
                verts[v].fBlendWeights[i] = 0.f;
            }
        }

        // ?? 본 가중치
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
        for (unsigned f = 0; f < numFaces; ++f)
            faces[f] = { mesh->mFaces[f].mIndices[0], mesh->mFaces[f].mIndices[1], mesh->mFaces[f].mIndices[2] };
        write_n(fp, faces.data(), sizeof(FACEINDICES32) * faces.size());
    }

    // ================== 머티리얼 (NonAnim처럼 이름 기반 매핑) ==================
    unsigned numMats = scene->mNumMaterials;
    write_n(fp, &numMats, sizeof(numMats));

    for (unsigned mi = 0; mi < numMats; ++mi)
    {
        // AI_TEXTURE_TYPE_MAX 만큼 빈 경로 기록 (포맷 유지)
        for (int t = 0; t < AI_TEXTURE_TYPE_MAX; ++t)
            put_str_empty_fixed(fp);
    }
}

// ============================= Animation.dat =============================
static void WriteBonesRecursive(FILE* fp, const aiNode* node) {
    BINBONE bone{}; strncpy_s(bone.szName, node->mName.C_Str(), MAX_PATH - 1);
    ToF4x4(bone.Transformation, node->mTransformation);
    bone.iNumChildren = node->mNumChildren;
    write_n(fp, &bone, sizeof(bone));
    for (unsigned i = 0; i < node->mNumChildren; ++i) WriteBonesRecursive(fp, node->mChildren[i]);
}
static void WriteMeshBoneOffsets(FILE* fp, const aiScene* scene) {
    for (unsigned m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* mesh = scene->mMeshes[m];
        for (unsigned b = 0; b < mesh->mNumBones; ++b) {
            const aiBone* bone = mesh->mBones[b];
            char name[MAX_PATH]{}; strncpy_s(name, bone->mName.C_Str(), MAX_PATH - 1); write_n(fp, name, sizeof(name));
            FLOAT4X4 offset{}; ToF4x4(offset, bone->mOffsetMatrix); write_n(fp, &offset, sizeof(offset));
        }
    }
}
static void WriteAnimations(FILE* fp, const aiScene* scene) {
    unsigned animCount = scene->mNumAnimations; write_n(fp, &animCount, sizeof(animCount));
    for (unsigned a = 0; a < animCount; ++a) {
        const aiAnimation* anim = scene->mAnimations[a];
        BINANIM ba{}; strncpy_s(ba.szName, anim->mName.C_Str(), MAX_PATH - 1);
        ba.dbDuration = anim->mDuration; ba.dbTickPerSecond = anim->mTicksPerSecond != 0.0 ? anim->mTicksPerSecond : 30.0;
        ba.iNumChannels = (int)anim->mNumChannels; write_n(fp, &ba, sizeof(ba));
        for (unsigned c = 0; c < anim->mNumChannels; ++c) {
            const aiNodeAnim* ch = anim->mChannels[c];
            BINCHANNEL bc{}; strncpy_s(bc.szName, ch->mNodeName.C_Str(), MAX_PATH - 1);
            bc.iNumKeyFrames = max({ (int)ch->mNumPositionKeys, (int)ch->mNumRotationKeys, (int)ch->mNumScalingKeys });
            write_n(fp, &bc, sizeof(bc));
            for (int i = 0; i < bc.iNumKeyFrames; ++i) {
                KEYFRAME kf{}; kf.fTime = (i < (int)ch->mNumPositionKeys) ? (float)ch->mPositionKeys[i].mTime : 0.f;
                if (i < (int)ch->mNumScalingKeys) { kf.vScale[0] = (float)ch->mScalingKeys[i].mValue.x; kf.vScale[1] = (float)ch->mScalingKeys[i].mValue.y; kf.vScale[2] = (float)ch->mScalingKeys[i].mValue.z; }
                else { kf.vScale[0] = kf.vScale[1] = kf.vScale[2] = 1.f; }
                if (i < (int)ch->mNumRotationKeys) { kf.vRotation[0] = (float)ch->mRotationKeys[i].mValue.x; kf.vRotation[1] = (float)ch->mRotationKeys[i].mValue.y; kf.vRotation[2] = (float)ch->mRotationKeys[i].mValue.z; kf.vRotation[3] = (float)ch->mRotationKeys[i].mValue.w; }
                else { kf.vRotation[0] = kf.vRotation[1] = kf.vRotation[2] = 0.f; kf.vRotation[3] = 1.f; }
                if (i < (int)ch->mNumPositionKeys) { kf.vPosition[0] = (float)ch->mPositionKeys[i].mValue.x; kf.vPosition[1] = (float)ch->mPositionKeys[i].mValue.y; kf.vPosition[2] = (float)ch->mPositionKeys[i].mValue.z; }
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

// ============================= 메인: Champion 하위 전부 자동 변환 =============================
int wmain()
{
    try {
        fs::path srcRoot = L"C:\\DirectX11_LeagueOfLegends\\Bin\\Resources\\Meshes\\Anim\\Champion";
        fs::path dstRoot = L"C:\\DirectX11_LeagueOfLegends\\Bin\\Bin_Data\\Anim\\Champion";
        if (!fs::exists(srcRoot)) throw runtime_error("Champion directory not found!");

        for (const auto& entry : fs::recursive_directory_iterator(srcRoot)) {
            if (!entry.is_directory()) continue;

            // 이 폴더에서 FBX 하나 찾기
            fs::path fbx;
            for (const auto& f : fs::directory_iterator(entry.path())) {
                auto ext = f.path().extension().wstring();
                if (ext == L".fbx" || ext == L".FBX") { fbx = f.path(); break; }
            }
            if (fbx.empty()) continue;

            // 출력 경로는 원본과 동일한 서브 폴더 구조 유지
            fs::path relative = fs::relative(entry.path(), srcRoot);
            fs::path outDir = dstRoot / relative;
            fs::create_directories(outDir);

            fs::path baseName = entry.path().filename(); // 폴더명 기준
            fs::path outModel = outDir / (baseName.wstring() + L".dat");
            fs::path outAnim = outDir / (baseName.wstring() + L"_Animation.dat");

            // 이미 있으면 스킵
            if (fs::exists(outModel) && fs::exists(outAnim)) {
                wcout << L"[SKIP] " << relative << L" (이미 존재)\n";
                continue;
            }

            // FBX 로드
            Assimp::Importer imp;
            const aiScene* scene = imp.ReadFile(
                fbx.string(),
                aiProcess_Triangulate |
                aiProcess_JoinIdenticalVertices |
                aiProcess_GenNormals |
                aiProcess_CalcTangentSpace |
                aiProcess_ConvertToLeftHanded
            );
            if (!scene) { cerr << "[ERROR] " << imp.GetErrorString() << " (" << fbx.string() << ")\n"; continue; }

            // 모델 dat
            if (!fs::exists(outModel)) {
                FILE* fm = nullptr; _wfopen_s(&fm, outModel.c_str(), L"wb");
                if (!fm) { cerr << "[ERROR] cannot open model for write\n"; continue; }
                WriteModelData(fm, scene, fbx.parent_path());
                fclose(fm);
            }

            // 애니메이션 dat
            if (!fs::exists(outAnim)) {
                FILE* fa = nullptr; _wfopen_s(&fa, outAnim.c_str(), L"wb");
                if (!fa) { cerr << "[ERROR] cannot open anim for write\n"; continue; }
                WriteAnimationData(fa, scene);
                fclose(fa);
            }

            wcout << L"[OK] " << relative << L" → " << outDir << L" (생성 완료)\n";
        }

        wcout << L"\n=== Champion 하위 전체 자동 변환 완료 (기존 파일은 스킵) ===\n";
    }
    catch (const exception& e) {
        cerr << "[ERROR] " << e.what() << endl;
    }
    return 0;
}





