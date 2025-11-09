#include "UI_MiniMap.h"
#include "GameInstance.h"

CUI_MiniMap::CUI_MiniMap(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CUI_Base(pDevice, pContext)
{
}

CUI_MiniMap::CUI_MiniMap(const CUI_MiniMap& rhs)
	: CUI_Base(rhs)
{
}

HRESULT CUI_MiniMap::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CUI_MiniMap::Initialize(void* pArg)
{
	m_fSize = { 400.f, 400.f };
	m_fPosition = { 1750.f, 900.f };
	m_fAlpha = 1.f;
	m_eShaderID = UI_ALPHABLEND;
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;


	return S_OK;
}

int CUI_MiniMap::Update(_float fTimeDelta)
{

	return OBJ_NOEVENT;
}

void CUI_MiniMap::Late_Update(_float fTimeDelta)
{
	if (nullptr != m_pRendererCom)
		m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_UI_BACK, this);
}

HRESULT CUI_MiniMap::Render()
{
	if (nullptr == m_pShaderCom || nullptr == m_pVIBufferCom)
		return E_FAIL;

	if (FAILED(SetUp_ShaderID()))
		return E_FAIL;

	if (FAILED(SetUp_ShaderResources()))
		return E_FAIL;

	int iCurrentIdx = m_pGameInstance->Get_CurrentLevelIndex();

	m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture", m_pTextureCom->Get_SRV(0));


	m_pShaderCom->Begin(m_eShaderID);
	m_pVIBufferCom->Render();

	return S_OK;
}

HRESULT CUI_MiniMap::Ready_Components(void* pArg)
{
	// Renderer
	if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC,
		TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRendererCom)))
		return E_FAIL;

	// Transform
	if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC,
		TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom)))
		return E_FAIL;

	// Shader
	if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC,
		TEXT("Prototype_Component_Shader_UI"), (CComponent**)&m_pShaderCom)))
		return E_FAIL;

	// Texture
	if (FAILED(__super::Add_Components(TEXT("Com_Texture"), LEVEL_GAMEPLAY,
		TEXT("Prototype_Component_Texture_MiniMap"), (CComponent**)&m_pTextureCom)))
		return E_FAIL;

	// VIBuffer
	if (FAILED(__super::Add_Components(TEXT("Com_VIBuffer"), LEVEL_STATIC,
		TEXT("Prototype_Component_VIBuffer_Rect"), (CComponent**)&m_pVIBufferCom)))
		return E_FAIL;


	return S_OK;
}

HRESULT CUI_MiniMap::SetUp_ShaderResources()
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



CUI_MiniMap* CUI_MiniMap::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CUI_MiniMap* pInstance = new CUI_MiniMap(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Cloned : CUI_SkillImage");
		Safe_Release(pInstance);
	}

	return pInstance;
}


CGameObject* CUI_MiniMap::Clone(void* pArg)
{
	CUI_MiniMap* pInstance = new CUI_MiniMap(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CUI_Skillbutton");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CUI_MiniMap::Free()
{
	__super::Free();
}