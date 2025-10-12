#include "GameObject.h"
#include "GameInstance.h"
CGameObject::CGameObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : m_pDevice(pDevice), m_pContext(pContext)
    , m_pGameInstance(CGameInstance::GetInstance())
{
    Safe_AddRef(m_pGameInstance);
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);
}

CGameObject::CGameObject(const CGameObject& rhs)
    : m_pDevice(rhs.m_pDevice), m_pContext(rhs.m_pContext), m_pGameInstance(CGameInstance::GetInstance())
{
    Safe_AddRef(m_pGameInstance);
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);
}



HRESULT CGameObject::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CGameObject::Initialize(void* pArg)
{
    return S_OK;
}

int CGameObject::Update(_float fTimeDelta)
{
    return OBJ_NOEVENT;
}

void CGameObject::Late_Update(_float fTimeDelta)
{
}

HRESULT CGameObject::Render()
{
    return S_OK;
}


HRESULT CGameObject::Add_Components(const _tchar* pComponentTag, _uint iLevelIndex, const _tchar* pPrototypeTag, CComponent** ppOut, void* pArg)
{
    if (Find_Component(pComponentTag))
        return E_FAIL;

    CComponent* pComponent = m_pGameInstance->Clone_Component(iLevelIndex, pPrototypeTag, pArg);
    if (!pComponent)
        return E_FAIL;

    m_Components.emplace(pComponentTag, pComponent);

    *ppOut = pComponent;
  
    
    return S_OK;
}

HRESULT CGameObject::Remove_Components(const _tchar* pComponentTag)
{
    if (!Find_Component(pComponentTag))
        return S_OK;

    for (auto iter = m_Components.begin(); iter != m_Components.end(); iter++)
    {
        if (!wcscmp(iter->first, pComponentTag))
        {
            Safe_Release(iter->second);
            m_Components.erase(iter);
            break;
        }
    }

    return S_OK;
}



void CGameObject::Compute_CamDistance(_fvector vWorldPos)
{
    _float4x4 ViewMatrix;
    CPipeLine* pPipeLine = CPipeLine::GetInstance();
    _float4 vCamPos = pPipeLine->Get_CamPosition();
    m_fCamDistance = XMVectorGetX(XMVector3Length(XMLoadFloat4(&vCamPos) - vWorldPos));

}

CComponent* CGameObject::Find_Component(const _tchar* pComponentTag)
{
    auto iter = find_if(m_Components.begin(), m_Components.end(), CTag_Finder(pComponentTag));
    if (iter == m_Components.end())
        return nullptr;

    return iter->second;
}

void CGameObject::Free()
{
    Safe_Release(m_pGameInstance);
    for (auto& Pair : m_Components)
        Safe_Release(Pair.second);
    m_Components.clear();
    Safe_Release(m_pDevice);
    Safe_Release(m_pContext);
}
