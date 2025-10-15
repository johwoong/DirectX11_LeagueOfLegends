#define NOMINMAX
#include <windows.h>
#include <string>
#include <filesystem>
#include <vector>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
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

static void write_n(FILE* fp, const void* p, size_t sz) {
    if (fwrite(p, 1, sz, fp) != sz) throw runtime_error("file write error");
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

// ==============================
// ?? 실행 시작점
// ==============================
int wmain() {
    try {
        // 1?? FBX 파일 경로 자동 지정
        fs::path inputFBX = L"C:\\DirectX11_LeagueOfLegends\\Bin\\Resources\\Meshes\\Anim\\Nexus\\Nexus.fbx";
        fs::path outModel = L"C:\\DirectX11_LeagueOfLegends\\Bin\\Bin_Data\\Anim\\Nexus\\Nexus.dat";
        fs::path outAnim = L"C:\\DirectX11_LeagueOfLegends\\Bin\\Bin_Data\\Anim\\Nexus\\Nexus_Animation.dat";

        // 2?? 파일 존재 확인
        if (!fs::exists(inputFBX)) {
            wcerr << L"[ERROR] FBX 파일을 찾을 수 없습니다:\n" << inputFBX << endl;
            return 11;
        }

        fs::create_directories(outModel.parent_path());

        // 3?? FBX 로드
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

        // 4?? Model.dat 생성
        FILE* fp = nullptr;
        if (_wfopen_s(&fp, outModel.c_str(), L"wb") || !fp)
            throw runtime_error("model.dat 파일 생성 실패");

        FLOAT4X4 pivot{};
        for (int i = 0; i < 16; ++i) pivot.m[i] = (i % 5 == 0) ? 1.f : 0.f;
        write_n(fp, &pivot, sizeof(pivot));

        uint32_t numMeshes = scene->mNumMeshes;
        write_n(fp, &numMeshes, sizeof(numMeshes));

        for (uint32_t mi = 0; mi < numMeshes; ++mi) {
            const aiMesh* mesh = scene->mMeshes[mi];
            string name = mesh->mName.length ? mesh->mName.C_Str() : "Mesh";
            put_str_fixed(fp, name);

            uint32_t matIndex = mesh->mMaterialIndex;
            write_n(fp, &matIndex, sizeof(matIndex));

            uint32_t numVerts = mesh->mNumVertices;
            write_n(fp, &numVerts, sizeof(numVerts));

            vector<VTXMODEL> verts(numVerts);
            for (uint32_t i = 0; i < numVerts; ++i) {
                verts[i].vPosition = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
                verts[i].vNormal = mesh->HasNormals() ? float3{ mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z } : float3{ 0,1,0 };
                verts[i].vTexUV = mesh->HasTextureCoords(0) ? float2{ (float)mesh->mTextureCoords[0][i].x, (float)mesh->mTextureCoords[0][i].y } : float2{ 0,0 };
                verts[i].vTangent = mesh->HasTangentsAndBitangents() ? float3{ mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z } : float3{ 1,0,0 };
            }
            write_n(fp, verts.data(), sizeof(VTXMODEL) * verts.size());

            uint32_t numFaces = mesh->mNumFaces;
            write_n(fp, &numFaces, sizeof(numFaces));
            vector<FACEINDICES32> faces(numFaces);
            for (uint32_t f = 0; f < numFaces; ++f)
                faces[f] = { mesh->mFaces[f].mIndices[0], mesh->mFaces[f].mIndices[1], mesh->mFaces[f].mIndices[2] };
            write_n(fp, faces.data(), sizeof(FACEINDICES32) * faces.size());
        }

        uint32_t numMats = scene->mNumMaterials;
        write_n(fp, &numMats, sizeof(numMats));

        // 블루 텍스처 강제 지정
        fs::path blueTex = L"C:\\DirectX11_LeagueOfLegends\\Bin\\Resources\\Meshes\\Anim\\Nexus\\nexus_tx_cm_blue.png";
        string texPath = blueTex.u8string();
        for (uint32_t i = 0; i < numMats; ++i) {
            put_str_fixed(fp, texPath); // diffuse
            put_str_fixed(fp, texPath); // normal
        }

        fclose(fp);
        wcout << L"[OK] 모델 dat 생성 완료: " << outModel << endl;

        // 5?? 애니메이션이 있으면 Animation.dat 추가 생성
        if (scene->mNumAnimations > 0) {
            FILE* fa = nullptr;
            if (_wfopen_s(&fa, outAnim.c_str(), L"wb") || !fa)
                throw runtime_error("animation.dat 파일 생성 실패");

            uint32_t animCount = scene->mNumAnimations;
            write_n(fa, &animCount, sizeof(animCount));

            fclose(fa);
            wcout << L"[OK] 애니메이션 dat 생성 완료: " << outAnim << endl;
        }
        else {
            wcout << L"[INFO] 애니메이션이 없으므로 Animation.dat 생략" << endl;
        }

        wcout << L"[DONE] 변환 완료!" << endl;
        return 0;
    }
    catch (const std::exception& e) {
        cerr << "[EXCEPTION] " << e.what() << endl;
        return 10;
    }
}
