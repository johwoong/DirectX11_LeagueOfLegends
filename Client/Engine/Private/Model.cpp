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

    for (auto& pMeshContainer_Instance : rhs.m_InstanceMeshes)
        m_InstanceMeshes.push_back((CMesh_Instance*)pMeshContainer_Instance->Clone());

    for (auto& Material : m_Materials)
        for (_uint i = 0; i < AI_TEXTURE_TYPE_MAX; ++i)
            Safe_AddRef(Material.pMaterials[i]);

    memcpy(m_szFilePath, rhs.m_szFilePath, sizeof(_tchar) * MAX_PATH);
}

CBone* CModel::Get_BonePtr(const char* pBoneName) const
{
    auto	iter = find_if(m_Bones.begin(), m_Bones.end(), [&](CBone* pNode)
        {
            return !strcmp(pNode->Get_Name(), pBoneName);
        });


    if (iter == m_Bones.end())
        return nullptr;

    return *iter;
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
            if (m_bInterupted)
                vecMoveTranslation = XMVectorSet(0.f, 0.f, 0.f, 1.f);
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

    for (_uint i = 0; i < m_iNumMeshes; ++i)
    {
        CMesh_Instance* pMeshContainer = CMesh_Instance::Create(m_pDevice, m_pContext, m_eModelType, hFile, &dwByte, pDataFilePath, this, XMLoadFloat4x4(&m_PivotMatrix));
        if (nullptr == pMeshContainer)
            return E_FAIL;

        m_InstanceMeshes.push_back(pMeshContainer);
    }

    if (FAILED(Create_Materials(hFile, &dwByte)))
        return E_FAIL;

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


        if (FAILED(Create_HierarchyNodes(hFile, &dwByte)))
            return E_FAIL;

        for (auto& pMeshContainer : m_Meshes)
            pMeshContainer->SetUp_Bones(hFile, &dwByte, this);

        if (FAILED(Create_Animations(hFile, &dwByte, szTAddFullPath)))
            return E_FAIL;

        CloseHandle(hFile);
    }

    return S_OK;
}

HRESULT CModel::SetUp_Material(CShader* pShader, const char* pConstantName, _uint iMeshIndex, aiTextureType eType, _uint /*TextureNum*/)
{
    if (iMeshIndex >= m_iNumMeshes)
        return E_FAIL;

    if (TYPE_NONANIM_INSTANCE != m_eModelType)
    {
        if (nullptr == m_Materials[m_Meshes[iMeshIndex]->Get_MaterialIndex()].pMaterials[eType])
            return S_OK;

        return pShader->Set_ShaderResourceView(pConstantName, m_Materials[m_Meshes[iMeshIndex]->Get_MaterialIndex()].pMaterials[eType]->Get_SRV());
    }
    else
    {
        if (nullptr == m_Materials[m_InstanceMeshes[iMeshIndex]->Get_MaterialIndex()].pMaterials[eType])
            return S_OK;

        return pShader->Set_ShaderResourceView(pConstantName, m_Materials[m_InstanceMeshes[iMeshIndex]->Get_MaterialIndex()].pMaterials[eType]->Get_SRV());
    }
}

_bool CModel::Play_Animation(_float fTimeDelta, _bool isLoop, const char* pBoneName, _float fInterpolatonTime)
{
    _bool isAnimLoop = false;

    if (m_bInterupted)
    {
        _bool isLinearEnd = false;

        m_fLinearCurrentTime += fTimeDelta;

        vector<CChannel*> PreChannels = m_Animations[m_iPreAnimIndex]->Get_Channels();
        vector<CChannel*> CurChannels = m_Animations[m_iCurrentAnimIndex]->Get_Channels();

        for (_int i = 0; i < PreChannels.size(); ++i)
        {
            _float fPreLinearTime = PreChannels[i]->Get_LinearKeyFrame().fTime;
            _float fCurLinearTime = CurChannels[i]->Get_LinearKeyFrame().fTime;

            PreChannels[i]->Invalidate_TransformationMatrix(fPreLinearTime + fTimeDelta, pBoneName);
            CurChannels[i]->Invalidate_TransformationMatrix(fCurLinearTime + fTimeDelta, pBoneName);

            _vector vScale, vRotation, vPosition;

            if (m_fLinearCurrentTime >= fInterpolatonTime)
            {
                KEYFRAME CurLinearKeyFrame = CurChannels[i]->Get_LinearKeyFrame();

                vScale = XMLoadFloat3(&CurLinearKeyFrame.vScale);
                vRotation = XMLoadFloat4(&CurLinearKeyFrame.vRotation);
                vPosition = XMLoadFloat3(&CurLinearKeyFrame.vPosition);
                vPosition = XMVectorSetW(vPosition, 1.f);

                isLinearEnd = true;
            }
            else
            {
                _vector vSourScale, vSourRotation, vSourPosition;
                _vector vDestScale, vDestRotation, vDestPosition;

                KEYFRAME PreLinearKeyFrame = PreChannels[i]->Get_LinearKeyFrame();
                KEYFRAME CurLinearKeyFrame = CurChannels[i]->Get_LinearKeyFrame();

                vSourScale = XMLoadFloat3(&PreLinearKeyFrame.vScale);
                vSourRotation = XMLoadFloat4(&PreLinearKeyFrame.vRotation);
                vSourPosition = XMLoadFloat3(&PreLinearKeyFrame.vPosition);

                vDestScale = XMLoadFloat3(&CurLinearKeyFrame.vScale);
                vDestRotation = XMLoadFloat4(&CurLinearKeyFrame.vRotation);
                vDestPosition = XMLoadFloat3(&CurLinearKeyFrame.vPosition);

                _float fRatio = m_fLinearCurrentTime / fInterpolatonTime;

                vScale = XMVectorLerp(vSourScale, vDestScale, fRatio);
                vRotation = XMQuaternionSlerp(vSourRotation, vDestRotation, fRatio);
                vPosition = XMVectorLerp(vSourPosition, vDestPosition, fRatio);
                vPosition = XMVectorSetW(vPosition, 1.f);
            }

            _matrix	TransformationMatrix = XMMatrixAffineTransformation(vScale, XMVectorSet(0.f, 0.f, 0.f, 1.f), vRotation, vPosition);

            PreChannels[i]->Get_BoneNode()->Set_TransformationMatrix(TransformationMatrix);
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
        isAnimLoop = m_Animations[m_iCurrentAnimIndex]->Invalidate_TransformationMatrix(fTimeDelta, isLoop, pBoneName);

    for (auto& pBoneNode : m_Bones)
        pBoneNode->Invalidate_CombinedTransformationmatrix(pBoneName);

    return isAnimLoop;
}

HRESULT CModel::Render(CShader* pShader, _uint iMeshIndex, _uint iPassIndex)
{
    if (TYPE_ANIM == m_eModelType)
    {
        _float4x4		BoneMatrix[400];

        m_Meshes[iMeshIndex]->Get_BoneMatrices(BoneMatrix, XMLoadFloat4x4(&m_PivotMatrix));

        pShader->Set_MatrixArray("g_BoneMatrices", BoneMatrix, 400);
    }

    pShader->Begin(iPassIndex);

    if (TYPE_NONANIM_INSTANCE != m_eModelType)
        m_Meshes[iMeshIndex]->Render();
    else
        m_InstanceMeshes[iMeshIndex]->Render();

    return S_OK;
}

HRESULT CModel::RenderShadow(CShader* pShader, _uint iMeshIndex, _uint /*iLevelIndex*/, _uint iPassIndex)
{
    if (TYPE_ANIM == m_eModelType)
    {
        _float4x4		BoneMatrix[400];

        m_Meshes[iMeshIndex]->Get_BoneMatrices(BoneMatrix, XMLoadFloat4x4(&m_PivotMatrix));

        pShader->Set_MatrixArray("g_BoneMatrices", BoneMatrix, 400);

    }

    pShader->Begin(iPassIndex);

    if (TYPE_NONANIM_INSTANCE != m_eModelType)
        m_Meshes[iMeshIndex]->Render();
    else
        m_InstanceMeshes[iMeshIndex]->Render();

    return S_OK;
}

HRESULT CModel::Set_AnimationReset() { m_Animations[m_iCurrentAnimIndex]->Set_TimeReset(); return S_OK; }
_bool CModel::Picking(CTransform* pTransform, _float3* pOut)
{
    for (_uint i = 0; i < m_iNumMeshes; ++i)
    {
        if (m_Meshes[i]->Picking(pTransform, pOut))
            return true;
    }

    return false;
}

void CModel::Update(_float fRadius)
{
    if (TYPE_NONANIM_INSTANCE == m_eModelType)
    {
        for (auto& pInstanceMesh : m_InstanceMeshes)
            pInstanceMesh->Update(fRadius);
    }
    if (TYPE_ANIM == m_eModelType)
    {
        _float4x4 BoneMatrix[400];
        for (auto& mesh : m_Meshes)
        {
            mesh->Get_BoneMatrices(BoneMatrix, XMLoadFloat4x4(&m_PivotMatrix));
            mesh->Update_PickingVertices(BoneMatrix, XMLoadFloat4x4(&m_PivotMatrix));
        }
    }
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
    if (nullptr == hFile) 
        return E_FAIL;

    ReadFile(hFile, &m_iNumMaterials, sizeof(_uint), pdwByte, nullptr);

    for (_uint i = 0; i < m_iNumMaterials; ++i)
    {
        MODELMATERIAL ModelMaterial;
        ZeroMemory(&ModelMaterial, sizeof(MODELMATERIAL));

        for (_uint j = 0; j < AI_TEXTURE_TYPE_MAX; ++j)
        {
            char szFullPath[MAX_PATH] = "";
            ReadFile(hFile, szFullPath, sizeof(char) * MAX_PATH, pdwByte, nullptr);

            if (!strcmp(szFullPath, ""))
                continue;

            _tchar szTFullPath[MAX_PATH] = TEXT("");

            MultiByteToWideChar(CP_ACP, 0, szFullPath, (int)strlen(szFullPath), szTFullPath, MAX_PATH);

            ModelMaterial.pMaterials[j] = CTexture::Create(m_pDevice, m_pContext, szTFullPath);
            if (nullptr == ModelMaterial.pMaterials[j])
                return E_FAIL;
        }

        m_Materials.push_back(ModelMaterial);
    }

    return S_OK;
}

HRESULT CModel::Create_HierarchyNodes(HANDLE hFile, _ulong* pdwByte, CBone* pParent)
{
    BINBONE BinBone;

    ReadFile(hFile, &BinBone, sizeof(BINBONE), pdwByte, nullptr);

    CBone* pHierarchyNode = CBone::Create(BinBone, pParent);
    if (nullptr == pHierarchyNode)
        return E_FAIL;

    m_Bones.push_back(pHierarchyNode);

    for (_uint i = 0; i < BinBone.iNumChildren; ++i)
        Create_HierarchyNodes(hFile, pdwByte, pHierarchyNode);

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
