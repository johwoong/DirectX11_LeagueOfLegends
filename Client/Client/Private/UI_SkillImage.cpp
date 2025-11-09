#include "UI_SkillImage.h"

#include "GameInstance.h"

CUI_SkillImage::CUI_SkillImage(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CUI_Base(pDevice, pContext)
{
}

CUI_SkillImage::CUI_SkillImage(const CUI_SkillImage& rhs)
	: CUI_Base(rhs)
{
}

HRESULT CUI_SkillImage::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CUI_SkillImage::Initialize(void* pArg)
{
	m_fSize = { 60.f, 60.f };

	// 기본 해상도 기준
	const float fBaseY = 970.f;  // 프레임 안쪽 세로 위치
	const float fGap = 90.f;     // 슬롯 간 간격
	const float fCenterX = 915.f; // 화면 중앙

	if (pArg)
	{
		auto pDesc = reinterpret_cast<SkillDesc*>(pArg);
		const int index = pDesc->SlotIndex;

		// 인덱스별로 명확하게 지정하고 싶다면 switch 문 사용
		switch (index)
		{
		case 0: // Passive
			m_fPosition = { 758.f, 960.f };
			m_fSize = { 44.f, 44.f };
			break;
		case 1: // Q
			m_fPosition = { 825.f, fBaseY };
			break;
		case 2: // W
			m_fPosition = { 898.f, fBaseY };
			break;
		case 3: // E
			m_fPosition = { 974.f, fBaseY };
			break;
		case 4: // R
			m_fPosition = {1053.f, fBaseY };
			break;
		}
	}
	else
	{
		// 인자 없을 경우 기본값
		m_fPosition = { fCenterX, fBaseY };
	}

	m_eShaderID = UI_DEFAULT;
	m_fAlpha = 1.f;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	return S_OK;
}

int CUI_SkillImage::Update(_float fTimeDelta)
{

	return OBJ_NOEVENT;
}

void CUI_SkillImage::Late_Update(_float fTimeDelta)
{
	if (nullptr != m_pRendererCom)
		m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_UI_FRONT, this);
}

HRESULT CUI_SkillImage::Render()
{
	if (nullptr == m_pShaderCom || nullptr == m_pVIBufferCom || nullptr == m_pTextureCom)
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

HRESULT CUI_SkillImage::Ready_Components(void* pArg)
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

	// VIBuffer
	if (FAILED(__super::Add_Components(TEXT("Com_VIBuffer"), LEVEL_STATIC,
		TEXT("Prototype_Component_VIBuffer_Rect"), (CComponent**)&m_pVIBufferCom)))
		return E_FAIL;

	return S_OK;
}

HRESULT CUI_SkillImage::SetUp_ShaderResources()
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

void CUI_SkillImage::Set_Icon(const wstring& IconTag)
{
	if (IconTag.empty())
		return;

	// 기존 텍스처 해제
	Safe_Release(m_pTextureCom);

	m_pTextureCom = static_cast<CTexture*>(
		m_pGameInstance->Clone_Component(LEVEL_GAMEPLAY, IconTag.c_str())
		);

	if (!m_pTextureCom)
	{
		return;
	}
;
}

CUI_SkillImage* CUI_SkillImage::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CUI_SkillImage* pInstance = new CUI_SkillImage(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Cloned : CUI_SkillImage");
		Safe_Release(pInstance);
	}

	return pInstance;
}


CGameObject* CUI_SkillImage::Clone(void* pArg)
{
	CUI_SkillImage* pInstance = new CUI_SkillImage(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CUI_Skillbutton");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CUI_SkillImage::Free()
{
	__super::Free();
}