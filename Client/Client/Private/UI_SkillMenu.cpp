
#include "UI_SkillMenu.h"

#include "GameInstance.h"

USING(Client)  // 네임스페이스 사용

CUI_SkillMenu::CUI_SkillMenu(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CUI_Base(pDevice, pContext)
{
}

CUI_SkillMenu::CUI_SkillMenu(const CUI_SkillMenu& rhs)
	: CUI_Base(rhs)
{
}

HRESULT CUI_SkillMenu::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CUI_SkillMenu::Initialize(void* pArg)
{
	m_fSize = { 600.f, 200.f };
	m_fPosition = { 960.f, 1000.f };
	m_fAlpha = 1.f;
	m_eShaderID = UI_ALPHABLEND;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	// ?? SkillImage 5개 생성
	for (int i = 0; i < 5; ++i)
	{
		CUI_SkillImage::SkillDesc desc = {};
		desc.SlotIndex = i;
		desc.IconTag = L"Prototype_Component_Texture_DefaultSkill";
		desc.Cooldown = 0.f;

		CGameObject* pObj = nullptr;
		if (FAILED(m_pGameInstance->Add_GameObject_Out(
			TEXT("Prototype_GameObject_UI_SkillImage"),
			LEVEL_GAMEPLAY,
			TEXT("Layer_UI"),
			pObj, &desc)))
			return E_FAIL;

		if (pObj)
		{
			m_pSkillImages.push_back(static_cast<CUI_SkillImage*>(pObj));
		}
	}

	// ? 여기서 PlayerManager에 SkillMenu 전달
	CPlayerManager::GetInstance()->Set_SkillMenu(this);
	return S_OK;
}

int CUI_SkillMenu::Update(_float fTimeDelta)
{
	return OBJ_NOEVENT;
}

void CUI_SkillMenu::Late_Update(_float fTimeDelta)
{
	if (nullptr != m_pRendererCom)
		m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_UI_BACK, this);
}

HRESULT CUI_SkillMenu::Render()
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

HRESULT CUI_SkillMenu::Ready_Components(void* pArg)
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
		TEXT("Prototype_Component_Texture_SkillInterface"), (CComponent**)&m_pTextureCom)))
		return E_FAIL;

	// VIBuffer
	if (FAILED(__super::Add_Components(TEXT("Com_VIBuffer"), LEVEL_STATIC,
		TEXT("Prototype_Component_VIBuffer_Rect"), (CComponent**)&m_pVIBufferCom)))
		return E_FAIL;

	return S_OK;
}

HRESULT CUI_SkillMenu::SetUp_ShaderResources()
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

void CUI_SkillMenu::Set_SkillIcons(const CChampion::SkillInfo* pSkills)
{
	for (int i = 0; i < 5; ++i)
	{
		if (i < m_pSkillImages.size())
			m_pSkillImages[i]->Set_Icon(pSkills[i].IconTag);
	}
}


CUI_SkillMenu* CUI_SkillMenu::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CUI_SkillMenu* pInstance = new CUI_SkillMenu(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Cloned : CUI_Skillbutton");
		Safe_Release(pInstance);
	}

	return pInstance;
}


CGameObject* CUI_SkillMenu::Clone(void* pArg)
{
	CUI_SkillMenu* pInstance = new CUI_SkillMenu(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CUI_Skillbutton");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CUI_SkillMenu::Free()
{
	m_pSkillImages.clear();
	__super::Free();
}