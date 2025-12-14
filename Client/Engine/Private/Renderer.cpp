#include "Renderer.h"
#include "GameInstance.h"
#include "GameObject.h"

CRenderer::CRenderer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CComponent(pDevice, pContext)
    , m_pTarget_Manager(CTarget_Manager::GetInstance())
    , m_pLight_Manager(CLightManager::GetInstance())
{
    Safe_AddRef(m_pLight_Manager);
    Safe_AddRef(m_pTarget_Manager);
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




void CRenderer::Set_Saturation(_bool bSaturation, _float fSaturationPower)
{
    m_bSaturation = bSaturation;
    m_fSaturationPower = fSaturationPower;
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

HRESULT CRenderer::Render_Glow()
{
    /* If there are not Glow Object return. */
	if (m_GameObjects[RENDER_GLOW].empty())
		return S_OK;
	else
	{
		if (FAILED(m_pTarget_Manager->Begin_MRT(m_pContext, TEXT("MRT_Glow"))))
			return E_FAIL;

		m_GameObjects[RENDER_GLOW].sort([](CGameObject* pSour, CGameObject* pDest)
		{
			return pSour->Get_CamDistance() > pDest->Get_CamDistance();
		});

		for (auto& pGameObject : m_GameObjects[RENDER_GLOW])
		{
			if (pGameObject)
			{
				pGameObject->Render_Glow();
				Safe_Release(pGameObject);
			}
		}

		if (FAILED(m_pTarget_Manager->End_MRT(m_pContext)))
			return E_FAIL;
	}

	/* Glow */
	if (!m_GameObjects[RENDER_GLOW].empty())
	{
		if (FAILED(m_pShaderPostProcessing->Set_RawValue("g_WorldMatrix", &m_worldMatrix, sizeof(_float4x4))))
			return E_FAIL;
		if (FAILED(m_pShaderPostProcessing->Set_RawValue("g_ViewMatrix", &m_ViewMatrix, sizeof(_float4x4))))
			return E_FAIL;
		if (FAILED(m_pShaderPostProcessing->Set_RawValue("g_ProjMatrix", &m_ProjMatrix, sizeof(_float4x4))))
			return E_FAIL;

		m_pTarget_Manager->Copy_BackBufferTexture(m_pDevice, m_pContext);
		if (FAILED(m_pShaderPostProcessing->Set_ShaderResourceView("g_BackBufferTexture", m_pTarget_Manager->Get_BackBufferCopySRV())))
			return E_FAIL;

		if (FAILED(m_pTarget_Manager->Bind_ShaderResource(TEXT("Target_Glow"), m_pShaderPostProcessing, "g_BlurTexture")))
			return E_FAIL;

		/* Horizontal Blur */
		if (FAILED(m_pTarget_Manager->Begin_MRT(m_pContext, TEXT("MRT_Blur_Horizontal"))))
			return E_FAIL;

		m_pShaderPostProcessing->Begin(1);
		m_pVIBuffer->Render();

		if (FAILED(m_pTarget_Manager->End_MRT(m_pContext)))
			return E_FAIL;

		if (FAILED(m_pTarget_Manager->Bind_ShaderResource(TEXT("Target_Blur_Horizontal"), m_pShaderPostProcessing, "g_BlurTexture")))
			return E_FAIL;

		if (FAILED(m_pTarget_Manager->Begin_MRT(m_pContext, TEXT("MRT_Blur_Vertical"))))
			return E_FAIL;

		m_pShaderPostProcessing->Begin(2); /* Vertical Blur */
		m_pVIBuffer->Render();

		if (FAILED(m_pTarget_Manager->End_MRT(m_pContext)))
			return E_FAIL;

		/* Bind the Horizontally and Vertically Blurred Image to the Glow Texture. */
		if (FAILED(m_pTarget_Manager->Bind_ShaderResource(TEXT("Target_Blur_Vertical"), m_pShaderPostProcessing, "g_BlendTexture")))
			return E_FAIL;

		m_pShaderPostProcessing->Begin(0);
		m_pVIBuffer->Render();

		m_GameObjects[RENDER_GLOW].clear();

		return S_OK;
	}
}

HRESULT CRenderer::Render_Distortion()
{
    /* If there are not Distortion Object return. */
    if (m_GameObjects[RENDER_DISTORTION].empty())
        return S_OK;
    else
    {
        if (FAILED(m_pShaderPostProcessing->Set_RawValue("g_WorldMatrix", &m_worldMatrix, sizeof(_float4x4))))
            return E_FAIL;
        if (FAILED(m_pShaderPostProcessing->Set_RawValue("g_ViewMatrix", &m_ViewMatrix, sizeof(_float4x4))))
            return E_FAIL;
        if (FAILED(m_pShaderPostProcessing->Set_RawValue("g_ProjMatrix", &m_ProjMatrix, sizeof(_float4x4))))
            return E_FAIL;

        m_fDistortionTimer += CTimer_Manager::GetInstance()->Get_TimeDelta(TEXT("Timer_60"));
        if (FAILED(m_pShaderPostProcessing->Set_RawValue("g_fDistortionTimer", &m_fDistortionTimer, sizeof(_float))))
            return E_FAIL;

        for (auto& pGameObject : m_GameObjects[RENDER_DISTORTION])
        {
            if (pGameObject)
            {
                if (FAILED(m_pTarget_Manager->Begin_MRT(m_pContext, TEXT("MRT_Distortion"))))
                    return E_FAIL;

                pGameObject->Render_Distort();
                Safe_Release(pGameObject);

                if (FAILED(m_pTarget_Manager->End_MRT(m_pContext)))
                    return E_FAIL;

                m_pTarget_Manager->Copy_BackBufferTexture(m_pDevice, m_pContext);
                if (FAILED(m_pShaderPostProcessing->Set_ShaderResourceView("g_BackBufferTexture", m_pTarget_Manager->Get_BackBufferCopySRV())))
                    return E_FAIL;

                if (FAILED(m_pTarget_Manager->Bind_ShaderResource(TEXT("Target_Distortion"), m_pShaderPostProcessing, "g_DistortionTexture")))
                    return E_FAIL;

                if (!m_pDistortionNoiseTexture)
                    m_pDistortionNoiseTexture = (CTexture*)CComponentManager::GetInstance()->Clone_Component(0, TEXT("Distortion_Noise"));

                if (FAILED(m_pShaderPostProcessing->Set_ShaderResourceView("g_DistortionNoiseTexture", m_pDistortionNoiseTexture->Get_SRV())))
                    return E_FAIL;

                m_pShaderPostProcessing->Begin(6);
                m_pVIBuffer->Render();
            }
        }
    }

    m_GameObjects[RENDER_DISTORTION].clear();

    return S_OK;
}

HRESULT CRenderer::Render_EdgeDetection()
{
    /* If there are not Edge Detection Objects return. */
    if (m_GameObjects[RENDER_EDGE_DETECTION].empty())
        return S_OK;
    else
    {
        if (FAILED(m_pTarget_Manager->Begin_MRT(m_pContext, TEXT("MRT_Edge_Detection"))))
            return E_FAIL;

        for (auto& pGameObject : m_GameObjects[RENDER_EDGE_DETECTION])
        {
            pGameObject->Render_EdgeDetection();
            Safe_Release(pGameObject);
        }

        m_GameObjects[RENDER_EDGE_DETECTION].clear();

        if (FAILED(m_pTarget_Manager->End_MRT(m_pContext)))
            return E_FAIL;
    }

#pragma region Edge_Detection
    /* Edge Detection Compute */
    if (FAILED(m_pTarget_Manager->Bind_ShaderResource(TEXT("Target_Edge_Detection"), m_pShaderPostProcessing, "g_EdgeDetectionTexture")))
        return E_FAIL;

    if (FAILED(m_pTarget_Manager->Begin_MRT(m_pContext, TEXT("MRT_Edge_Detection_Compute"))))
        return E_FAIL;

    m_pShaderPostProcessing->Begin(10); /* Edge Detection Compute */
    m_pVIBuffer->Render();

    if (FAILED(m_pTarget_Manager->End_MRT(m_pContext)))
        return E_FAIL;

    /* Edge Detection Blend */
    if (FAILED(m_pTarget_Manager->Bind_ShaderResource(TEXT("Target_Edge_Detection_Compute"), m_pShaderPostProcessing, "g_EdgeDetectionComputeTexture")))
        return E_FAIL;

    m_pShaderPostProcessing->Begin(11); /* Edge Detection Blend */
    m_pVIBuffer->Render();
#pragma endregion Edge_Detection

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

    Safe_Release(m_pLight_Manager);
    Safe_Release(m_pTarget_Manager);

    Safe_Release(m_pVIBuffer);
    Safe_Release(m_pShaderDeferred);
    Safe_Release(m_pShaderPostProcessing);

    Safe_Release(m_pDistortionTexture);
    Safe_Release(m_pDistortionNoiseTexture);
    Safe_Release(m_pScreenDistortionTexture);
}
