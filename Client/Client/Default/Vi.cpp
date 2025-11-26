#include "Vi.h"

#include "GameInstance.h"

_bool CVi::Is_AnimationLoop(_uint eAnimId)
{
	switch ((ANIM)eAnimId)
	{
	case IDLE1:
	case IDLE2:
	case RUN1:
		return true;
	}
	return false;
}


CVi::CVi(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CChampion(pDevice, pContext)
{
}

CVi::CVi(const CVi& rhs)
	: CChampion(rhs)
{
}

HRESULT CVi::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CVi::Initialize(void* pArg)
{
	m_ePlayerID = RENEKTON;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;
	Set_Scale(_float3(0.01f, 0.01f, 0.01f));
	SetSkillInfo();
	return S_OK;
}

void CVi::SetSkillInfo()
{
	m_ChampionIconTag = L"Prototype_Component_Texture_Vi_Icon";
	m_Skills[0] = { L"Rising Sell Force", L"Prototype_Component_Texture_ViSkill_Passive", 0.f };
	m_Skills[1] = { L"Mystic Sht", L"Prototype_Component_Texture_ViSkill_Q", 0.f };
	m_Skills[2] = { L"Essence lux", L"Prototype_Component_Texture_ViSkill_W", 0.f };
	m_Skills[3] = { L"Arcane hift", L"Prototype_Component_Texture_ViSkill_E", 0.f };
	m_Skills[4] = { L"Truesht Barrage", L"Prototype_Component_Texture_ViSkill_R", 0.f };
}


HRESULT CVi::Ready_Components(void* pArg)
{
	/* For.Com_Renderer */
	if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRendererCom)))
		return E_FAIL;

	/* For.Com_Transform */
	CTransform::TRANSFORMDESC TransformDesc;
	ZeroMemory(&TransformDesc, sizeof(CTransform::TRANSFORMDESC));
	TransformDesc.fSpeedPerSec = 5.f;
	TransformDesc.fRotationPerSec = XMConvertToRadians(90.0f);
	if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC, TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom, &TransformDesc)))
		return E_FAIL;


	/* For.Com_Shader */
	if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxAnimModel"), (CComponent**)&m_pShaderCom)))
		return E_FAIL;

	/* For.Com_Model*/
	if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Vi"), (CComponent**)&m_pModelCom)))
		return E_FAIL;

	return S_OK;
}


CVi* CVi::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CVi* pInstance = new CVi(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CVi");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CVi::Clone(void* pArg)
{
	CVi* pInstance = new CVi(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CRenekton");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CVi::Free()
{
	__super::Free();
}
