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

HRESULT CModelManager::Create_Model(LEVEL eLevel, const _tchar* pPrototypeTag, const _tchar* pLayerTag, ID3D11Device* pDevice, ID3D11DeviceContext* pContext, CModel::TYPE eModelType, _fmatrix PivotMatrix, _bool bCreatePrototype)
{
    CGameInstance* pGameInstance = CGameInstance::GetInstance();

    NONANIMDESC NonAnimDesc;
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
