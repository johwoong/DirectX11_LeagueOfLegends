#include "UI_ExpBar.h"

#include "PlayerManager.h"
#include "GameInstance.h"

CUI_ExpBar::CUI_ExpBar(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CUI_Base(pDevice, pContext)
{
}

CUI_ExpBar::CUI_ExpBar(const CUI_ExpBar& rhs)
	: CUI_Base(rhs)
{
}

HRESULT CUI_ExpBar::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CUI_ExpBar::Initialize(void* pArg)
{
	m_fSize = { 350.f, 200.f };
	m_fPosition = { 600.f, 1000.f };
	m_fAlpha = 1.f;
	m_eShaderID = UI_ALPHABLEND;
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	// 챔피언 아이콘 UI 생성
	CGameObject* pObj = nullptr;
	if (FAILED(m_pGameInstance->Add_GameObject_Out(
		TEXT("Prototype_GameObject_UI_ChampionIcon"),
		LEVEL_GAMEPLAY,
		TEXT("Layer_UI"),
		pObj)))
		return E_FAIL;

	m_pChampionIcon = static_cast<CUI_ChampionIcon*>(pObj);
	m_pChampionIcon->AddRef();

	// 위치 맞추기
	m_pChampionIcon->Set_Position({ m_fPosition.x - 140.f, m_fPosition.y - 5.f });
	CPlayerManager::GetInstance()->Set_ExpBar(this);
	return S_OK;
}

int CUI_ExpBar::Update(_float fTimeDelta)
{

	return OBJ_NOEVENT;
}

void CUI_ExpBar::Late_Update(_float fTimeDelta)
{
	if (nullptr != m_pRendererCom)
		m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_UI_FRONT, this);
}

HRESULT CUI_ExpBar::Render()
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

HRESULT CUI_ExpBar::Ready_Components(void* pArg)
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
		TEXT("Prototype_Component_Texture_ExpBar"), (CComponent**)&m_pTextureCom)))
		return E_FAIL;

	// VIBuffer
	if (FAILED(__super::Add_Components(TEXT("Com_VIBuffer"), LEVEL_STATIC,
		TEXT("Prototype_Component_VIBuffer_Rect"), (CComponent**)&m_pVIBufferCom)))
		return E_FAIL;

	
	return S_OK;
}

HRESULT CUI_ExpBar::SetUp_ShaderResources()
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

void CUI_ExpBar::Set_ChampionIcon(const wstring& IconTag)
{
	if (!m_pChampionIcon) return;
	m_pChampionIcon->Set_Icon(IconTag);  // ? 그대로 적용하면 끝
}

CUI_ExpBar* CUI_ExpBar::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CUI_ExpBar* pInstance = new CUI_ExpBar(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Cloned : CUI_SkillImage");
		Safe_Release(pInstance);
	}

	return pInstance;
}


CGameObject* CUI_ExpBar::Clone(void* pArg)
{
	CUI_ExpBar* pInstance = new CUI_ExpBar(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CUI_Skillbutton");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CUI_ExpBar::Free()
{
	Safe_Release(m_pChampionIcon); 
	__super::Free();
}