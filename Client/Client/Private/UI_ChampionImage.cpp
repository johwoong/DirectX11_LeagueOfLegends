#include "UI_ChampionImage.h"
#include "GameInstance.h"

CUI_ChampionImage::CUI_ChampionImage(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CUI_Base(pDevice, pContext)
{
}

CUI_ChampionImage::CUI_ChampionImage(const CUI_ChampionImage& rhs)
	: CUI_Base(rhs)
{
}

HRESULT CUI_ChampionImage::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CUI_ChampionImage::Initialize(void* pArg)
{
	if (pArg)
		m_eChampion = static_cast<CHAMPION>(*reinterpret_cast<int*>(pArg)); 

	m_fSize.x = 250.f;
	m_fSize.y = 420.f;

	switch (m_eChampion)
	{
	case EZREAL:
		m_fPosition.x = 1250.f;
		m_fPosition.y = 270.f;
		break;
	case RENEKTON:
		m_fPosition.x = 400.f;
		m_fPosition.y = 100.f;
		break;
	}

	m_eShaderID = UI_DEFAULT;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	return S_OK;
}

int CUI_ChampionImage::Update(_float fTimeDelta)
{
	if (m_pGameInstance->Get_CurrentLevelIndex() != LEVEL_LOADING)
		m_bDead = true;

	return OBJ_NOEVENT;
}

void CUI_ChampionImage::Late_Update(_float fTimeDelta)
{
	if (nullptr != m_pRendererCom)
		m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_UI_BACK, this);
}

HRESULT CUI_ChampionImage::Render()
{
	if (nullptr == m_pShaderCom || nullptr == m_pVIBufferCom)
		return E_FAIL;

	if (FAILED(SetUp_ShaderID()))
		return E_FAIL;


	m_pTransformCom->Set_Scale(CTransform::STATE_RIGHT, m_fSize.x);
	m_pTransformCom->Set_Scale(CTransform::STATE_UP, m_fSize.y);
	m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION,
		XMVectorSet(m_fPosition.x - g_WinSizeX * 0.5f,
			-m_fPosition.y + g_WinSizeY * 0.5f,
			0.f, 1.f));

	SetUp_ShaderResources(); 
	m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture", m_pTextureCom->Get_SRV(m_eChampion));
	m_pShaderCom->Begin(m_eShaderID);
	m_pVIBufferCom->Render();


	// 프레임 위치
	m_pTransformCom->Set_Scale(CTransform::STATE_RIGHT, m_fFrameSize.x);
	m_pTransformCom->Set_Scale(CTransform::STATE_UP, m_fFrameSize.y);
	m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION,
		XMVectorSet(m_fPosition.x - g_WinSizeX * 0.5f,
			-m_fPosition.y + g_WinSizeY * 0.5f,
			0.f, 1.f));

	SetUp_ShaderResources(); 
	m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture", m_pFrameTextureCom->Get_SRV(0));
	m_pShaderCom->Begin(SHADERUI_ID::UI_ALPHABLEND);
	m_pVIBufferCom->Render();

	// 원래 위치 복귀
	m_pTransformCom->Set_Scale(CTransform::STATE_RIGHT, m_fSize.x);
	m_pTransformCom->Set_Scale(CTransform::STATE_UP, m_fSize.y);

	return S_OK;
}

HRESULT CUI_ChampionImage::Ready_Components(void* pArg)
{
	/* For.Com_Renderer */
	if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRendererCom)))
		return E_FAIL;

	/* For.Com_Transform */
	if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC, TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom)))
		return E_FAIL;

	/* For.Com_Shader */
	if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC, TEXT("Prototype_Component_Shader_UI"), (CComponent**)&m_pShaderCom)))
		return E_FAIL;

	/* For.Com_Texture (Champion) */
	if (FAILED(__super::Add_Components(TEXT("Com_Texture"), LEVEL_STATIC, TEXT("Prototype_Component_Texture_ChampionImage"), (CComponent**)&m_pTextureCom)))
		return E_FAIL;

	/* For.Com_Texture (Frame) */
	if (FAILED(__super::Add_Components(TEXT("Com_Texture_Frame"), LEVEL_STATIC, TEXT("Prototype_Component_Texture_Frame"), (CComponent**)&m_pFrameTextureCom)))
		return E_FAIL;

	/* For.Com_VIBuffer */
	if (FAILED(__super::Add_Components(TEXT("Com_VIBuffer"), LEVEL_STATIC, TEXT("Prototype_Component_VIBuffer_Rect"), (CComponent**)&m_pVIBufferCom)))
		return E_FAIL;

	return S_OK;
}

HRESULT CUI_ChampionImage::SetUp_ShaderResources()
{
	if (nullptr == m_pShaderCom)
		return E_FAIL;

	_float4x4 WorldMatrix = m_pTransformCom->Get_World_TP();

	if (FAILED(m_pShaderCom->Set_RawValue("g_WorldMatrix", &WorldMatrix, sizeof(_float4x4))))
		return E_FAIL;
	if (FAILED(m_pShaderCom->Set_RawValue("g_ViewMatrix", &m_ViewMatrix, sizeof(_float4x4))))
		return E_FAIL;
	if (FAILED(m_pShaderCom->Set_RawValue("g_ProjMatrix", &m_ProjMatrix, sizeof(_float4x4))))
		return E_FAIL;

	return S_OK;
}

CUI_ChampionImage* CUI_ChampionImage::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CUI_ChampionImage* pInstance = new CUI_ChampionImage(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CUI_Loading");
		Safe_Release(pInstance);
	}

	return pInstance;
}


CGameObject* CUI_ChampionImage::Clone(void* pArg)
{
	CUI_ChampionImage* pInstance = new CUI_ChampionImage(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CUI_Loading");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CUI_ChampionImage::Free()
{
	Safe_Release(m_pFrameTextureCom);
	__super::Free();
}