#include "Model.h"// CModel.cpp
#include "Bone.h"
#include "Texture.h"
#include "Shader.h"
#include "Mesh.h"
#include "Animation.h"
#include "Channel.h"
#include "Mesh_Instance.h"

#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


namespace fs = std::filesystem;

CModel::CModel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CComponent(pDevice, pContext) {
}

CModel::CModel(const CModel& rhs)
    : CComponent(rhs)
    , m_Materials(rhs.m_Materials)
    , m_iNumMeshes(rhs.m_iNumMeshes)
    , m_iNumMaterials(rhs.m_iNumMaterials)
    , m_iNumBones(rhs.m_iNumBones)
    , m_PivotMatrix(rhs.m_PivotMatrix)
    , m_eModelType(rhs.m_eModelType)
    , m_iCurrentAnimIndex(rhs.m_iCurrentAnimIndex)
    , m_iNumAnimations(rhs.m_iNumAnimations)
    , m_iPreAnimIndex(rhs.m_iPreAnimIndex)
{
    for (auto& pMeshContainer : rhs.m_Meshes)
        m_Meshes.push_back((CMesh*)pMeshContainer->Clone());

    for (auto& Material : m_Materials)
        for (_uint i = 0; i < AI_TEXTURE_TYPE_MAX; ++i)
            Safe_AddRef(Material.pMaterials[i]);

    memcpy(m_szFilePath, rhs.m_szFilePath, sizeof(_tchar) * MAX_PATH);
}

CBone* CModel::Get_BonePtr(const char* pBoneName) const
{
    auto it = std::find_if(m_Bones.begin(), m_Bones.end(),
        [&](CBone* pNode) { return !strcmp(pNode->Get_Name(), pBoneName); });
    return it == m_Bones.end() ? nullptr : *it;
}

vector<ANIMEVENT> CModel::Get_Events(void)
{
    return m_Animations[m_iCurrentAnimIndex]->Get_Events();
}

void CModel::Get_MoveTransformationMatrix(const char* pBoneName, _vector* pTranslation, _float* pRotation)
{
    for (auto& pBone : m_Bones)
    {
        if (!strcmp(pBoneName, pBone->Get_Name()))
        {
            _vector vecMoveTranslation = m_bInterupted ? XMVectorSet(0.f, 0.f, 0.f, 1.f) : pBone->Get_Translation();
            _float Rotation = pBone->Get_RotationRadian();

            memcpy(pTranslation, &vecMoveTranslation, sizeof(_vector));
            memcpy(pRotation, &Rotation, sizeof(_float));
            break;
        }
    }
}

void CModel::Set_CurrentAnimIndex(_uint iAnimIndex)
{
    if (!m_bInterupted)
    {
        m_iPreAnimIndex = m_iCurrentAnimIndex;
        m_iCurrentAnimIndex = iAnimIndex;
        m_bInterupted = true;
    }
    else
    {
        m_Animations[m_iPreAnimIndex]->Reset();
        m_iPreAnimIndex = m_iCurrentAnimIndex;
        m_iCurrentAnimIndex = iAnimIndex;
    }
}

_bool CModel::Is_Keyframe(char* pChannelName, _uint iKeyframe) { return m_Animations[m_iCurrentAnimIndex]->Is_Keyframe(pChannelName, iKeyframe); }
_bool CModel::Under_Keyframe(char* pChannelName, _uint iKeyframe) { return m_Animations[m_iCurrentAnimIndex]->Under_Keyframe(pChannelName, iKeyframe); }
_bool CModel::Over_Keyframe(char* pChannelName, _uint iKeyframe) { return m_Animations[m_iCurrentAnimIndex]->Over_Keyframe(pChannelName, iKeyframe); }
_bool CModel::Between_Keyframe(char* pChannelName, _uint l, _uint u) { return m_Animations[m_iCurrentAnimIndex]->Between_Keyframe(pChannelName, l, u); }
void  CModel::Set_TimeReset() { m_Animations[m_iCurrentAnimIndex]->Set_TimeReset(); }
void  CModel::Reset(void) { m_Animations[m_iCurrentAnimIndex]->Reset(); }
void  CModel::Reset_Anim(_uint idx) { m_Animations[idx]->Reset(); }

HRESULT CModel::Initialize_Prototype(TYPE eModelType, const char* pModelFilePath, _fmatrix PivotMatrix)
{
    m_eModelType = eModelType;

    _tchar szModelFilePath[MAX_PATH] = TEXT("");
    MultiByteToWideChar(CP_ACP, 0, pModelFilePath, (int)strlen(pModelFilePath), szModelFilePath, MAX_PATH);
    memcpy(m_szFilePath, szModelFilePath, sizeof(_tchar) * MAX_PATH);

    _ulong dwByte = 0;
    HANDLE hFile = CreateFile(szModelFilePath, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (0 == hFile)
    {
        _tchar szErrorMsg[MAX_PATH] = TEXT("Failed to Open File : ");
        lstrcat(szErrorMsg, szModelFilePath);
        MessageBox(nullptr, szErrorMsg, L"System Message", MB_OK);
        return E_FAIL;
    }

    ReadFile(hFile, &m_PivotMatrix, sizeof(_float4x4), &dwByte, nullptr);
    if (FAILED(Create_MeshContainer(hFile, &dwByte))) { CloseHandle(hFile); return E_FAIL; }
    if (FAILED(Create_Materials(hFile, &dwByte))) { CloseHandle(hFile); return E_FAIL; }
    CloseHandle(hFile);
    return S_OK;
}

/* ===== FBX 프로토타입 초기화 ===== */
HRESULT CModel::Initialize_Prototype_FromFBX(const char* fbxPath, _fmatrix PivotMatrix)
{
    m_eModelType = TYPE_NONANIM;
    XMStoreFloat4x4(&m_PivotMatrix, PivotMatrix);
    MultiByteToWideChar(CP_ACP, 0, fbxPath, -1, m_szFilePath, MAX_PATH);

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        fbxPath,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_ValidateDataStructure |
        aiProcess_OptimizeMeshes |
        aiProcess_PreTransformVertices
    );
    if (!scene || !scene->mRootNode) return E_FAIL;

    // Meshes
    m_iNumMeshes = scene->mNumMeshes;
    m_Meshes.reserve(m_iNumMeshes);
    for (uint32_t i = 0; i < scene->mNumMeshes; ++i)
    {
        const aiMesh* aimesh = scene->mMeshes[i];
        CMesh* mesh = CMesh::CreateFromAiMesh(m_pDevice, m_pContext, TYPE_NONANIM, aimesh, this, XMLoadFloat4x4(&m_PivotMatrix));
        if (!mesh) return E_FAIL;
        m_Meshes.push_back(mesh);
    }

    // Materials
    m_iNumMaterials = scene->mNumMaterials;
    m_Materials.resize(m_iNumMaterials);

    fs::path fbxFs = fs::u8path(fbxPath);
    fs::path fbxDir = fbxFs.has_parent_path() ? fbxFs.parent_path() : fs::path();

    for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
    {
        aiMaterial* aimat = scene->mMaterials[i];
        MODELMATERIAL mat{};
        auto loadOne = [&](aiTextureType type)
            {
                aiString tex;
                if (AI_SUCCESS == aimat->GetTexture(type, 0, &tex))
                {
                    fs::path rel = fs::u8path(tex.C_Str());     // 예: "Map.fbm/xxx.png"
                    fs::path full = rel.is_absolute() ? rel : (fbxDir / rel);
                    full = full.lexically_normal();

                    _tchar w[MAX_PATH] = TEXT("");
                    std::string s = full.string();
                    MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, w, MAX_PATH);

                    mat.pMaterials[type] = CTexture::Create(m_pDevice, m_pContext, w);
                }
            };

        loadOne(aiTextureType_DIFFUSE);
        loadOne(aiTextureType_NORMALS);
        // 필요한 타입 있으면 추가로 loadOne 호출

        m_Materials[i] = mat;
    }

    return S_OK;
}

CModel* CModel::CreateFromFBX(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const char* fbxPath, _fmatrix PivotMatrix)
{
    CModel* pInstance = new CModel(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype_FromFBX(fbxPath, PivotMatrix)))
    {
        MSG_BOX("Failed to Created : CModel (FBX)");
        Safe_Release(pInstance);
    }
    return pInstance;
}

HRESULT CModel::Instance_Initialize_Prototype(TYPE eModelType, const char* pModelFilePath, const char* pDataFilePath, _fmatrix PivotMatrix)
{
    m_eModelType = eModelType;

    _tchar szModelFilePath[MAX_PATH] = TEXT("");
    MultiByteToWideChar(CP_ACP, 0, pModelFilePath, (int)strlen(pModelFilePath), szModelFilePath, MAX_PATH);
    memcpy(m_szFilePath, szModelFilePath, sizeof(_tchar) * MAX_PATH);

    _ulong dwByte = 0;
    HANDLE hFile = CreateFile(szModelFilePath, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (0 == hFile)
    {
        _tchar szErrorMsg[MAX_PATH] = TEXT("Failed to Open File : ");
        lstrcat(szErrorMsg, szModelFilePath);
        MessageBox(nullptr, szErrorMsg, L"System Message", MB_OK);
        return E_FAIL;
    }

    ReadFile(hFile, &m_PivotMatrix, sizeof(_float4x4), &dwByte, nullptr);
    ReadFile(hFile, &m_iNumMeshes, sizeof(_uint), &dwByte, nullptr);
    m_InstanceMeshes.reserve(m_iNumMeshes);

    // 인스턴스 메시 로딩은 주석 유지
    if (FAILED(Create_Materials(hFile, &dwByte))) { CloseHandle(hFile); return E_FAIL; }
    CloseHandle(hFile);
    return S_OK;
}

HRESULT CModel::Initialize(void* pArg)
{
    if (TYPE_ANIM == m_eModelType)
    {
        char szFilePath[MAX_PATH];
        WideCharToMultiByte(CP_ACP, 0, m_szFilePath, -1, szFilePath, MAX_PATH, nullptr, nullptr);

        char szDir[MAX_PATH] = "";
        char szName[MAX_PATH] = "";
        char szAnimName[MAX_PATH] = "_Animation";
        char szExt[MAX_PATH] = "";
        char szAnimationFileName[MAX_PATH] = "";
        char szAddData[MAX_PATH] = "_Add";
        char szAnimationAddFileName[MAX_PATH] = "";

        _splitpath_s(szFilePath, nullptr, 0, szDir, MAX_PATH, szName, MAX_PATH, szExt, MAX_PATH);

        strcpy_s(szAnimationFileName, szDir);
        strcat_s(szAnimationFileName, szName);
        strcat_s(szAnimationFileName, szAnimName);

        strcpy_s(szAnimationAddFileName, szAnimationFileName);
        strcat_s(szAnimationFileName, szExt);

        strcat_s(szAnimationAddFileName, szAddData);
        strcat_s(szAnimationAddFileName, szExt);

        _tchar szTFullPath[MAX_PATH] = TEXT("");
        _tchar szTAddFullPath[MAX_PATH] = TEXT("");

        MultiByteToWideChar(CP_ACP, 0, szAnimationFileName, (int)strlen(szAnimationFileName), szTFullPath, MAX_PATH);
        MultiByteToWideChar(CP_ACP, 0, szAnimationAddFileName, (int)strlen(szAnimationAddFileName), szTAddFullPath, MAX_PATH);

        _ulong dwByte = 0;
        HANDLE hFile = CreateFile(szTFullPath, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (0 == hFile)
        {
            _tchar szErrorMsg[MAX_PATH] = TEXT("Failed to Open File : ");
            lstrcat(szErrorMsg, szTFullPath);
            MessageBox(nullptr, szErrorMsg, L"System Message", MB_OK);
            return E_FAIL;
        }

        if (FAILED(Create_HierarchyNodes(hFile, &dwByte))) { CloseHandle(hFile); return E_FAIL; }
        for (auto& pMeshContainer : m_Meshes)
            pMeshContainer->SetUp_Bones(hFile, &dwByte, this);
        if (FAILED(Create_Animations(hFile, &dwByte, szTAddFullPath))) { CloseHandle(hFile); return E_FAIL; }

        CloseHandle(hFile);
    }
    return S_OK;
}

HRESULT CModel::SetUp_Material(CShader* pShader, const char* pConstantName, _uint iMeshIndex, aiTextureType eType, _uint /*TextureNum*/)
{
    if (iMeshIndex >= m_iNumMeshes) return E_FAIL;

    if (TYPE_NONANIM_INSTANCE != m_eModelType)
    {
        auto* tex = m_Materials[m_Meshes[iMeshIndex]->Get_MaterialIndex()].pMaterials[eType];
        if (!tex) return S_OK;
        return pShader->Set_ShaderResourceView(pConstantName, tex->Get_SRV());
    }
    return S_OK;
}

_bool CModel::Play_Animation(_float fTimeDelta, _bool isLoop, const char* pBoneName, _float fInterpolatonTime)
{
    _bool isAnimLoop = false;

    if (m_bInterupted)
    {
        _bool isLinearEnd = false;
        m_fLinearCurrentTime += fTimeDelta;

        auto PreChannels = m_Animations[m_iPreAnimIndex]->Get_Channels();
        auto CurChannels = m_Animations[m_iCurrentAnimIndex]->Get_Channels();

        for (_int i = 0; i < (_int)PreChannels.size(); ++i)
        {
            _float fPreLinearTime = PreChannels[i]->Get_LinearKeyFrame().fTime;
            _float fCurLinearTime = CurChannels[i]->Get_LinearKeyFrame().fTime;

            PreChannels[i]->Invalidate_TransformationMatrix(fPreLinearTime + fTimeDelta, pBoneName);
            CurChannels[i]->Invalidate_TransformationMatrix(fCurLinearTime + fTimeDelta, pBoneName);

            _vector vScale, vRotation, vPosition;

            if (m_fLinearCurrentTime >= fInterpolatonTime)
            {
                KEYFRAME k = CurChannels[i]->Get_LinearKeyFrame();
                vScale = XMLoadFloat3(&k.vScale);
                vRotation = XMLoadFloat4(&k.vRotation);
                vPosition = XMVectorSetW(XMLoadFloat3(&k.vPosition), 1.f);
                isLinearEnd = true;
            }
            else
            {
                KEYFRAME a = PreChannels[i]->Get_LinearKeyFrame();
                KEYFRAME b = CurChannels[i]->Get_LinearKeyFrame();
                _float t = m_fLinearCurrentTime / fInterpolatonTime;

                vScale = XMVectorLerp(XMLoadFloat3(&a.vScale), XMLoadFloat3(&b.vScale), t);
                vRotation = XMQuaternionSlerp(XMLoadFloat4(&a.vRotation), XMLoadFloat4(&b.vRotation), t);
                vPosition = XMVectorSetW(XMVectorLerp(XMLoadFloat3(&a.vPosition), XMLoadFloat3(&b.vPosition), t), 1.f);
            }

            _matrix M = XMMatrixAffineTransformation(vScale, XMVectorSet(0.f, 0.f, 0.f, 1.f), vRotation, vPosition);
            PreChannels[i]->Get_BoneNode()->Set_TransformationMatrix(M);
        }

        if (isLinearEnd)
        {
            m_Animations[m_iPreAnimIndex]->Reset();
            m_Animations[m_iCurrentAnimIndex]->Set_CurrentTime(m_fLinearCurrentTime);
            m_fLinearCurrentTime = 0.f;
            m_bInterupted = false;
        }
    }
    else
    {
        isAnimLoop = m_Animations[m_iCurrentAnimIndex]->Invalidate_TransformationMatrix(fTimeDelta, isLoop, pBoneName);
    }

    for (auto& pBoneNode : m_Bones)
        pBoneNode->Invaliddate_CombiendTransformationmatrix(pBoneName);

    return isAnimLoop;
}

HRESULT CModel::Render(CShader* pShader, _uint iMeshIndex, _uint iPassIndex)
{

    if (TYPE_ANIM == m_eModelType)
    {
        _float4x4 Bones[400];
        m_Meshes[iMeshIndex]->Get_BoneMatricese(Bones, XMLoadFloat4x4(&m_PivotMatrix));
        pShader->Set_MatrixArray("g_BoneMatrices", Bones, 400);
    }

    pShader->Begin(iPassIndex);
    if (TYPE_NONANIM_INSTANCE != m_eModelType)
        m_Meshes[iMeshIndex]->Render();
    return S_OK;
}

HRESULT CModel::RenderShadow(CShader* pShader, _uint iMeshIndex, _uint /*iLevelIndex*/, _uint iPassIndex)
{
    if (TYPE_ANIM == m_eModelType)
    {
        _float4x4 Bones[400];
        m_Meshes[iMeshIndex]->Get_BoneMatricese(Bones, XMLoadFloat4x4(&m_PivotMatrix));
        pShader->Set_MatrixArray("g_BoneMatrices", Bones, 400);
    }

    pShader->Begin(iPassIndex);
    if (TYPE_NONANIM_INSTANCE != m_eModelType)
        m_Meshes[iMeshIndex]->Render();
    return S_OK;
}

HRESULT CModel::Set_AnimationReset() { m_Animations[m_iCurrentAnimIndex]->Set_TimeReset(); return S_OK; }
_bool   CModel::Picking(CTransform*, _float3*) { return false; }

void CModel::Update(_float /*fRadius*/)
{
    // 인스턴스 메시 사용 시 필요
}

HRESULT CModel::Create_MeshContainer(HANDLE hFile, _ulong* pdwByte)
{
    if (!hFile) return E_FAIL;

    ReadFile(hFile, &m_iNumMeshes, sizeof(_uint), pdwByte, nullptr);
    m_Meshes.reserve(m_iNumMeshes);

    for (_uint i = 0; i < m_iNumMeshes; ++i)
    {
        CMesh* pMesh = CMesh::Create(m_pDevice, m_pContext, m_eModelType, hFile, pdwByte, this, XMLoadFloat4x4(&m_PivotMatrix));
        if (!pMesh) return E_FAIL;
        m_Meshes.push_back(pMesh);
    }
    return S_OK;
}

HRESULT CModel::Create_Materials(HANDLE hFile, _ulong* pdwByte)
{
    if (!hFile) return E_FAIL;

    ReadFile(hFile, &m_iNumMaterials, sizeof(_uint), pdwByte, nullptr);

    for (_uint i = 0; i < m_iNumMaterials; ++i)
    {
        MODELMATERIAL mat{};
        for (_uint j = 0; j < AI_TEXTURE_TYPE_MAX; ++j)
        {
            char szFullPath[MAX_PATH] = "";
            ReadFile(hFile, szFullPath, sizeof(char) * MAX_PATH, pdwByte, nullptr);
            if (szFullPath[0] == '\0') continue;

            _tchar wsz[MAX_PATH] = TEXT("");
            MultiByteToWideChar(CP_ACP, 0, szFullPath, (int)strlen(szFullPath), wsz, MAX_PATH);

            mat.pMaterials[j] = CTexture::Create(m_pDevice, m_pContext, wsz);
            if (!mat.pMaterials[j]) return E_FAIL;
        }
        m_Materials.push_back(mat);
    }
    return S_OK;
}

HRESULT CModel::Create_HierarchyNodes(HANDLE hFile, _ulong* pdwByte, CBone* pParent)
{
    BINBONE BinBone{};
    ReadFile(hFile, &BinBone, sizeof(BINBONE), pdwByte, nullptr);

    CBone* pNode = CBone::Create(BinBone, pParent);
    if (!pNode) return E_FAIL;

    m_Bones.push_back(pNode);
    for (_uint i = 0; i < BinBone.iNumChildren; ++i)
        Create_HierarchyNodes(hFile, pdwByte, pNode);

    return S_OK;
}

HRESULT CModel::Create_Animations(HANDLE hFile, _ulong* pdwByte, const _tchar* pAddDataFilePath)
{
    if (!hFile) return E_FAIL;

    ReadFile(hFile, &m_iNumAnimations, sizeof(_uint), pdwByte, nullptr);

    _ulong dwAddByte = 0;
    HANDLE hAddFile = CreateFile(pAddDataFilePath, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    for (_uint i = 0; i < m_iNumAnimations; ++i)
    {
        CAnimation* pAnim = CAnimation::Create(hFile, pdwByte, this, hAddFile, &dwAddByte);
        if (!pAnim) { CloseHandle(hAddFile); return E_FAIL; }
        m_Animations.push_back(pAnim);
    }
    CloseHandle(hAddFile);
    return S_OK;
}

CModel* CModel::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, TYPE eModelType, const char* pModelFilePath, _fmatrix PivotMatrix)
{
    CModel* pInstance = new CModel(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype(eModelType, pModelFilePath, PivotMatrix)))
    {
        MSG_BOX("Failed to Created : CModel");
        Safe_Release(pInstance);
    }
    return pInstance;
}

CModel* CModel::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, TYPE eModelType, const char* pModelFilePath, const char* pDataFilePath, _fmatrix PivotMatrix)
{
    CModel* pInstance = new CModel(pDevice, pContext);
    if (FAILED(pInstance->Instance_Initialize_Prototype(eModelType, pModelFilePath, pDataFilePath, PivotMatrix)))
    {
        MSG_BOX("Failed to Created : CModel");
        Safe_Release(pInstance);
    }
    return pInstance;
}

CComponent* CModel::Clone(void* pArg)
{
    CModel* pInstance = new CModel(*this);
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CModel");
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CModel::Free()
{
    __super::Free();

    for (auto& pAnimation : m_Animations)      Safe_Release(pAnimation);
    m_Animations.clear();

    for (auto& pBoneNode : m_Bones)            Safe_Release(pBoneNode);
    m_Bones.clear();

    for (auto& pInstanceMesh : m_InstanceMeshes) Safe_Release(pInstanceMesh);
    m_InstanceMeshes.clear();

    for (auto& pMesh : m_Meshes)               Safe_Release(pMesh);
    m_Meshes.clear();

    for (auto& mat : m_Materials)
        for (_uint i = 0; i < AI_TEXTURE_TYPE_MAX; ++i)
            Safe_Release(mat.pMaterials[i]);
    m_Materials.clear();
}
