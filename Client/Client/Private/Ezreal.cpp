#include "Ezreal.h"

#include "GameInstance.h"
#include "Channel.h"

_bool CEzreal::Is_AnimationLoop(_uint eAnimId)
{
	switch ((ANIM)eAnimId)
	{
	case IDLE:
	case RUN:
		return true;
	}
	return false;
}

void CEzreal::Ready_Animation_Event()
{
	CAnimation* pAnim = m_pModelCom->Get_Animations()[ATTACK1];
	if (!pAnim) return;

	ANIMEVENT evt{};
	evt.fStartTime = 11.f;
	evt.fEndTime = 13.f;
	evt.eType = ANIMEVENT::EVENT_PROJECTILE;

	pAnim->Add_Event(evt);;
	
	pAnim = m_pModelCom->Get_Animations()[ATTACK2];
	evt.fStartTime = 10.f;
	evt.fEndTime = 12.f;
	evt.eType = ANIMEVENT::EVENT_PROJECTILE;
	pAnim->Add_Event(evt);;

	pAnim = m_pModelCom->Get_Animations()[ATTACK3];
	evt.fStartTime = 10.f;
	evt.fEndTime = 12.f;
	evt.eType = ANIMEVENT::EVENT_PROJECTILE;
	pAnim->Add_Event(evt);;

	pAnim = m_pModelCom->Get_Animations()[ATTACK4];
	evt.fStartTime = 11.2f;
	evt.fEndTime = 13.f;
	evt.eType = ANIMEVENT::EVENT_PROJECTILE;
	pAnim->Add_Event(evt);;

	pAnim = m_pModelCom->Get_Animations()[CRIT];
	evt.fStartTime = 8.7f;
	evt.fEndTime = 11.f;
	evt.eType = ANIMEVENT::EVENT_PROJECTILE;
	pAnim->Add_Event(evt);;

	pAnim = m_pModelCom->Get_Animations()[Q];
	evt.fStartTime = 3.78f;
	evt.fEndTime = 5.f;
	evt.eType = ANIMEVENT::EVENT_PROJECTILE;
	pAnim->Add_Event(evt);;


	pAnim = m_pModelCom->Get_Animations()[W];
	evt.fStartTime = 7.15f;
	evt.fEndTime = 8.f;
	evt.eType = ANIMEVENT::EVENT_PROJECTILE;
	pAnim->Add_Event(evt);;
	
	if (!pAnim) return;
	
}


CEzreal::CEzreal(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CChampion(pDevice, pContext)
{
}

CEzreal::CEzreal(const CEzreal& rhs)
	: CChampion(rhs)
{
}

HRESULT CEzreal::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CEzreal::Initialize(void* pArg)
{
	m_ePlayerID = EZREAL;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;
	m_tStat.AttackRange = 280.f;
	Set_Scale(_float3(1.1f, 1.1f, 1.1f));
	SetSkillInfo(); 
	return S_OK;
}

void CEzreal::SetSkillInfo()
{
	m_ChampionIconTag = L"Prototype_Component_Texture_Ezreal_Icon"; 
	m_Skills[0] = { L"Rising Spell Force", L"Prototype_Component_Texture_EzrealSkill_Passive", 0.f };
	m_Skills[1] = { L"Mystic Shot", L"Prototype_Component_Texture_EzrealSkill_Q", 0.f };
	m_Skills[2] = { L"Essence Flux", L"Prototype_Component_Texture_EzrealSkill_W", 0.f };
	m_Skills[3] = { L"Arcane Shift", L"Prototype_Component_Texture_EzrealSkill_E", 0.f };
	m_Skills[4] = { L"Trueshot Barrage", L"Prototype_Component_Texture_EzrealSkill_R", 0.f };
}

HRESULT CEzreal::Ready_Components(void* pArg)
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
	if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Ezreal"), (CComponent**)&m_pModelCom)))
		return E_FAIL;

	return S_OK;
}

CEzreal* CEzreal::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CEzreal* pInstance = new CEzreal(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CEzreal");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CEzreal::Clone(void* pArg)
{
	CEzreal* pInstance = new CEzreal(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CEzreal");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CEzreal::Free()
{
	__super::Free();
}
