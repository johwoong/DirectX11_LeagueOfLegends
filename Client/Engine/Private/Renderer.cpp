#include "Renderer.h"
#include "GameInstance.h"
#include "GameObject.h"

CRenderer::CRenderer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CComponent(pDevice, pContext)
{
}

HRESULT CRenderer::Initialize_Prototype()
{
    D3D11_VIEWPORT ViewportDesc;
    ZeroMemory(&ViewportDesc, sizeof ViewportDesc);

    _uint iNumViewports = 1;
    m_pContext->RSGetViewports(&iNumViewports, &ViewportDesc);


    return S_OK;
}

HRESULT CRenderer::Initialize(void* pArg)
{
    return S_OK;
}

HRESULT CRenderer::Add_RenderGroup(RENDERGROUP eRenderGroup, CGameObject* pGameObject)
{
    if (nullptr == pGameObject)
        return E_FAIL;

    m_GameObjects[eRenderGroup].push_back(pGameObject);

    Safe_AddRef(pGameObject);

    return S_OK;
}

HRESULT CRenderer::Render_GameObjects()
{
    if (FAILED(Render_Priority()))
        return E_FAIL;

    if (FAILED(Render_NonAlphaBlend()))
        return E_FAIL;

    if (FAILED(Render_AlphaBlend()))
        return E_FAIL;

#ifdef _DEBUG
    Render_Debug();
#endif

    if(FAILED(Render_UI()))
        return E_FAIL;


    return S_OK;
}

#ifdef _DEBUG
HRESULT CRenderer::Add_Debug(CComponent* pDebugCom)
{
    m_DebugComponents.push_back(pDebugCom);

    Safe_AddRef(pDebugCom);

    return S_OK;
}

void CRenderer::Debug_Clear()
{
    for (auto& pComponent : m_DebugComponents)
    {
        Safe_Release(pComponent);
    }

    m_DebugComponents.clear();
}

#endif // DEBUG

HRESULT CRenderer::Render_Priority()
{
    for (auto& pGameObject : m_GameObjects[RENDER_PRIORITY])
    {
        if (nullptr != pGameObject)
        {
            pGameObject->Render();
            Safe_Release(pGameObject);
        }
    }

    m_GameObjects[RENDER_PRIORITY].clear();

    return S_OK;
}

HRESULT CRenderer::Render_NonAlphaBlend()
{
    for (auto& pGameObject : m_GameObjects[RENDER_NOALPHABLEND])
    {
        if (nullptr != pGameObject)
        {
            pGameObject->Render();
            Safe_Release(pGameObject);
        }
    }

    m_GameObjects[RENDER_NOALPHABLEND].clear();

    return S_OK;
}

HRESULT CRenderer::Render_AlphaBlend()
{
    m_GameObjects[RENDER_ALPHABLEND].sort([](CGameObject* pSour, CGameObject* pDest)
    {
        return pSour->Get_CamDistance() > pDest->Get_CamDistance();
    });
    for (auto& pGameObject : m_GameObjects[RENDER_ALPHABLEND])
    {
        if (nullptr != pGameObject)
        {
            pGameObject->Render();
            Safe_Release(pGameObject);
        }
    }

    m_GameObjects[RENDER_ALPHABLEND].clear();
    return S_OK;
}

#ifdef _DEBUG
HRESULT CRenderer::Render_Debug()
{
    if (CGameInstance::GetInstance()->Get_CurrentLevelIndex() == 1)
    {
        Debug_Clear();
        return E_FAIL;
    }

    

    for (auto& pComponent : m_DebugComponents)
    {
        if (m_bRenderComponentDebug)
        {
            if (nullptr != pComponent)
                pComponent->Render();
        }

        Safe_Release(pComponent);
    }

    m_DebugComponents.clear();

    if (CGameInstance::GetInstance()->Key_Up(DIK_F6))
        m_bRenderDebug = !m_bRenderDebug;
    if (CGameInstance::GetInstance()->Key_Up(DIK_F7))
        m_bRenderComponentDebug = !m_bRenderComponentDebug;

    return S_OK;
}
#endif // DEBUG

HRESULT CRenderer::Render_UI()
{
    for (auto& pGameObject : m_GameObjects[RENDER_UI_BACK])
    {
        if (nullptr != pGameObject)
        {
            pGameObject->Render();
            Safe_Release(pGameObject);
        }
    }

    m_GameObjects[RENDER_UI_BACK].clear();

    for (auto& pGameObject : m_GameObjects[RENDER_UI_FRONT])
    {
        if (nullptr != pGameObject)
        {
            pGameObject->Render();
            Safe_Release(pGameObject);
        }
    }

    m_GameObjects[RENDER_UI_FRONT].clear();
    return S_OK;
}


CRenderer* CRenderer::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CRenderer* pInstance = new CRenderer(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CRenderer");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CComponent* CRenderer::Clone(void* pArg)
{
    AddRef();

    return this;
}

void CRenderer::Free()
{
    __super::Free();

}
