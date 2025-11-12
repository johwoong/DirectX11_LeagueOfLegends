#include "pch.h"
#include "ModelManager.h"
#include "GameInstance.h"
#include "Model.h"
#include "GameObject.h"
#include <filesystem>


IMPLEMENT_SINGLETON(CModelManager)

CModelManager::CModelManager()
{
    m_CreatedModel.reserve(0);
    m_PrototypeTag.reserve(0);
}

HRESULT CModelManager::Create_Model(
    LEVEL eLevel,
    const _tchar* pPrototypeTag,
    const _tchar* pLayerTag,
    ID3D11Device* pDevice,
    ID3D11DeviceContext* pContext,
    CModel::TYPE eModelType,
    _fmatrix PivotMatrix,
    _bool bCreatePrototype,
    NONANIMDESC* pCustomDesc) 
{
    CGameInstance* pGameInstance = CGameInstance::GetInstance();
    pGameInstance->AddRef();
    NONANIMDESC NonAnimDesc = {};
    if (pCustomDesc) 
        memcpy(&NonAnimDesc, pCustomDesc, sizeof(NONANIMDESC));
    else
        memcpy(&NonAnimDesc, &m_InitModelDesc, sizeof(NONANIMDESC));

    char cModelTag[MAX_PATH] = "";
    WideCharToMultiByte(CP_ACP, 0, pPrototypeTag, MAX_PATH, cModelTag, MAX_PATH, NULL, NULL);
    strcpy(NonAnimDesc.pModeltag, cModelTag);

    if (FAILED(pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_NonAnim"), eLevel, pLayerTag, &NonAnimDesc)))
    {
        pGameInstance->Release();
        return E_FAIL;
    }

    pGameInstance->Release();
    return S_OK;
}

CNonAnim* CModelManager::Clone_Model(CNonAnim* pSource, LEVEL eLevel, const _tchar* pLayerTag, ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    if (!pSource)
        return nullptr;

    CTransform* pTransform = pSource->Get_Transform();
    if (!pTransform)
        return nullptr;

    NONANIMDESC Desc = pSource->Get_ModelDesc();

    // [1] Transform 값 복사
    XMStoreFloat3(&Desc.vPosition, pTransform->Get_State(CTransform::STATE_TRANSLATION));

    _float fScaleX = pTransform->Get_Scale(CTransform::STATE_RIGHT);
    _float fScaleY = pTransform->Get_Scale(CTransform::STATE_UP);
    _float fScaleZ = pTransform->Get_Scale(CTransform::STATE_LOOK);
    Desc.vScale = _float3(fScaleX, fScaleY, fScaleZ);

    Desc.vRotation = _float3(
        pTransform->Get_CurrentRotationX(),
        pTransform->Get_CurrentRotationY(),
        pTransform->Get_CurrentRotationZ()
    );

    XMStoreFloat4x4(&Desc.WorldMatrix, pTransform->Get_WorldMatrix());

    // [2] 복제 위치 보정 (겹치지 않게 살짝 이동)
    Desc.vPosition.x += 2.0f;
    Desc.vPosition.z += 2.0f;

    // [3] PrototypeTag 변환
    _tchar szPrototypeTag[MAX_PATH] = TEXT("");
    MultiByteToWideChar(CP_ACP, 0, Desc.pModeltag, (int)strlen(Desc.pModeltag), szPrototypeTag, MAX_PATH);

    // [4] 복제 생성 (? Desc를 직접 넘김)
    _matrix PivotMatrix = XMMatrixIdentity();
    if (FAILED(Create_Model(eLevel, szPrototypeTag, pLayerTag,
        pDevice, pContext, CModel::TYPE_NONANIM, PivotMatrix, false, &Desc))) // ? pCustomDesc 전달
    {
        return nullptr;
    }

    // [5] 마지막 생성된 모델 반환
    return !m_CreatedModel.empty() ? m_CreatedModel.back() : nullptr;
}

void CModelManager::Add_PrototypeTag(const _tchar* PrototypeTag)
{
    _tchar* pModelTag = new _tchar[MAX_PATH];
    wsprintf(pModelTag, PrototypeTag);
    m_PrototypeTag.push_back(pModelTag);
}

void CModelManager::Add_FileName(const _tchar* Layertag, const _tchar* FileName)
{
    const _tchar* pModelTag = Find_ModelTag(Layertag);
    if (pModelTag != nullptr)
        return;

    _tchar* szFullPath = new _tchar[MAX_PATH];
    _tchar* szLayerTag = new _tchar[MAX_PATH];

    _tcscpy(szLayerTag, Layertag);
    _tcscpy(szFullPath, FileName);

    m_ModelTags.emplace(szLayerTag, szFullPath);
}

void CModelManager::Set_AllPickedFalse()
{
    for (auto& pGameObject : m_CreatedModel)
        dynamic_cast<CActor*>(pGameObject)->Set_Picked(false);
}

const _tchar* CModelManager::Find_ModelTag(const _tchar* ModelTag)
{
    auto iter = find_if(m_ModelTags.begin(), m_ModelTags.end(), CTag_Finder(ModelTag));

    if (iter == m_ModelTags.end())
        return nullptr;

    return iter->second;
}

void CModelManager::Out_CreatedModel(CNonAnim* pGameObject)
{
    auto iter = m_CreatedModel.begin();
    while (iter != m_CreatedModel.end())
    {
        if (*iter == pGameObject)
            iter = m_CreatedModel.erase(iter);
        else
            ++iter;
    }
}

void CModelManager::Clear_Layer()
{
    for (auto& iter : m_ModelTags)
        Safe_Delete(iter.second);
    m_ModelTags.clear();
}


void CModelManager::Free()
{
    for (auto& iter : m_ModelTags)
        Safe_Delete(iter.second);
    m_ModelTags.clear();

    for (auto& iter : m_PrototypeTag)
        Safe_Delete(iter);
    m_PrototypeTag.clear();


    m_CreatedModel.clear();
}
