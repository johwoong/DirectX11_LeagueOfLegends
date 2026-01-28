#include "Layer.h"

CLayer::CLayer()
{
}

CComponent* CLayer::Get_Component(const _tchar* pComponentTag, _uint iIndex)
{
    if (iIndex >= m_GameObjects.size())
        return nullptr;

    auto iter = m_GameObjects.begin();

    for (_uint i = 0; i < iIndex; ++i)
        ++iter;

    return (*iter)->Find_Component(pComponentTag);
}

CGameObject* CLayer::Get_Object(_uint iIndex)
{
    if (m_GameObjects.size() <= iIndex)
        return nullptr;

    auto iter = m_GameObjects.begin();

    for (_uint i = 0; i < iIndex; ++i)
        ++iter;

    return *iter;
}

HRESULT CLayer::Initialize(_uint iLevelIdx)
{
    m_iLevel = iLevelIdx;
    return S_OK;
}

HRESULT CLayer::Add_GameObject(CGameObject* pGameObject)
{
    if (nullptr == pGameObject)
        return E_FAIL;

    m_GameObjects.push_back(pGameObject);

    return S_OK;
}

HRESULT CLayer::Out_GameObject(CGameObject* pGameObject)
{
    for (auto iter = m_GameObjects.begin(); iter != m_GameObjects.end();)
    {
        if ((*iter) == pGameObject)
        {
            iter = m_GameObjects.erase(iter);
            return S_OK;
        }
        else
            iter++;
    }
    return S_OK;
}

HRESULT CLayer::Out_GameObjectList()
{
    for (auto iter = m_GameObjects.begin(); iter != m_GameObjects.end();)
        iter = m_GameObjects.erase(iter);

    return S_OK;
}

void CLayer::Update(_float fTimeDelta)
{
    for (auto iter = m_GameObjects.begin(); iter != m_GameObjects.end();)
    {
        if ((*iter) != nullptr)
        {
            int iEvent = (*iter)->Update(fTimeDelta);

            if (iEvent == OBJ_DEAD)
            {
                // ★ 핵심: Update 중 절대 Safe_Release 하지 마라
                iter = m_GameObjects.erase(iter);
            }
            else if (iEvent == OBJ_POOL)
            {
                iter = m_GameObjects.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
        else
        {
            ++iter;
        }
    }
}

void CLayer::Late_Update(_float fTimeDelta)
{
    for (auto& pGameObject : m_GameObjects)
    {
        if (nullptr != pGameObject)
            pGameObject->Late_Update(fTimeDelta);
    }
}

CLayer* CLayer::Create(_uint iLevelIdx)
{
    CLayer* pLayer = new CLayer;
    if (FAILED(pLayer->Initialize(iLevelIdx)))
    {
        MSG_BOX("Failed to Created : CLayer");
        Safe_Release(pLayer);
    }

    return pLayer;
}

void CLayer::Free()
{
    for (auto& pGameObejct : m_GameObjects)
    {
        if (pGameObejct != nullptr)
            Safe_Release(pGameObejct);
    }

    m_GameObjects.clear();
}

