#include "Temoo.h"

#include "GameInstance.h"
#include "Channel.h"

_bool CTemoo::Is_AnimationLoop(_uint eAnimId)
{
	switch ((ANIM)eAnimId)
	{
	case IDLE:
	case IDLE2:
	case IDLE3:
	case RUN:
	case W:
		return true;
	}
	return false;
}

void CTemoo::Ready_Animation_Event()
{
	CAnimation* pAnim = m_pModelCom->Get_Animations()[ATTACK1];
	if (!pAnim) return;

	ANIMEVENT evt{};
	evt.fStartTime = 8.f;
	evt.fEndTime = 10.f;
	evt.eType = ANIMEVENT::EVENT_PROJECTILE;

	pAnim->Add_Event(evt);;

	pAnim = m_pModelCom->Get_Animations()[CRIT];
	evt.fStartTime = 8.f;
	evt.fEndTime = 10.f;
	evt.eType = ANIMEVENT::EVENT_PROJECTILE;
	pAnim->Add_Event(evt);;

	pAnim = m_pModelCom->Get_Animations()[Q];
	evt.fStartTime = 7.f;
	evt.fEndTime = 9.f;
	evt.eType = ANIMEVENT::EVENT_PROJECTILE;
	pAnim->Add_Event(evt);;

	pAnim = m_pModelCom->Get_Animations()[R_0];
	evt.fStartTime = 3.f;
	evt.fEndTime = 4.f;
	evt.eType = ANIMEVENT::EVENT_PROJECTILE;
	pAnim->Add_Event(evt);;

	pAnim = m_pModelCom->Get_Animations()[R_180];
	evt.fStartTime = 3.f;
	evt.fEndTime = 4.f;
	evt.eType = ANIMEVENT::EVENT_PROJECTILE;
	pAnim->Add_Event(evt);;

	pAnim = m_pModelCom->Get_Animations()[R_90];
	evt.fStartTime = 3.f;
	evt.fEndTime = 4.f;
	evt.eType = ANIMEVENT::EVENT_PROJECTILE;
	pAnim->Add_Event(evt);;

	pAnim = m_pModelCom->Get_Animations()[R_45];
	evt.fStartTime = 3.f;
	evt.fEndTime = 4.f;
	evt.eType = ANIMEVENT::EVENT_PROJECTILE;
	pAnim->Add_Event(evt);;

	pAnim = m_pModelCom->Get_Animations()[R_MINUS180];
	evt.fStartTime = 3.f;
	evt.fEndTime = 4.f;
	evt.eType = ANIMEVENT::EVENT_PROJECTILE;
	pAnim->Add_Event(evt);

	pAnim = m_pModelCom->Get_Animations()[R_MINUS45];
	evt.fStartTime = 3.f;
	evt.fEndTime = 4.f;
	evt.eType = ANIMEVENT::EVENT_PROJECTILE;
	pAnim->Add_Event(evt);;

	pAnim = m_pModelCom->Get_Animations()[R_MINUS90];
	evt.fStartTime = 3.f;
	evt.fEndTime = 4.f;
	evt.eType = ANIMEVENT::EVENT_PROJECTILE;
	pAnim->Add_Event(evt);;

	pAnim = m_pModelCom->Get_Animations()[DEATH];
	evt.fStartTime = 79.f;
	evt.fEndTime = 80.f;
	evt.eType = ANIMEVENT::EVENT_STATE;
	pAnim->Add_Event(evt);;
}


CTemoo::CTemoo(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CChampion(pDevice, pContext)
{
}

CTemoo::CTemoo(const CTemoo& rhs)
	: CChampion(rhs)
{
}

HRESULT CTemoo::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CTemoo::Initialize(void* pArg)
{
	m_ePlayerID = TEEMO;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;
	m_tStat.AttackRange = 230.f;
	m_tStat.AttackDamage = 100.f;
	m_tStat.MaxHp = 100.f;
	m_tStat.Hp = m_tStat.MaxHp;
	Set_Scale(_float3(0.01f, 0.01f, 0.01f));
	SetSkillInfo();
	Set_DeathAnim(CTemoo::ANIM::DEATH);
	Set_SpawnAnim(CTemoo::ANIM::SPAWN);   // 네 enum 실제 이름으로
	return S_OK;
}

void CTemoo::SetSkillInfo()
{
	m_ChampionIconTag = L"Prototype_Component_Texture_Teemo_Icon";
	m_Skills[0] = { L"Rising Spell Force", L"Prototype_Component_Texture_TemooSkill_Passive", 0.f };
	m_Skills[1] = { L"Mystic Shot", L"Prototype_Component_Texture_TemooSkill_Q", 0.f };
	m_Skills[2] = { L"Essence Flux", L"Prototype_Component_Texture_TemooSkill_W", 0.f };
	m_Skills[3] = { L"Arcane Shift", L"Prototype_Component_Texture_TemooSkill_E", 0.f };
	m_Skills[4] = { L"Trueshot Barrage", L"Prototype_Component_Texture_TemooSkill_R", 0.f };
}

HRESULT CTemoo::Ready_Components(void* pArg)
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
	if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Teemo"), (CComponent**)&m_pModelCom)))
		return E_FAIL;

	CColider::COLLIDERDESC ColliderDesc;
	ZeroMemory(&ColliderDesc, sizeof(CColider::COLLIDERDESC));
	ColliderDesc.vScale = _float3(1.2f, 1.2f, 1.2f);
	ColliderDesc.vRotation = _float3(0.f, 0.f, 0.f);
	ColliderDesc.vPosition = _float3(0.f, 0.5f, 0.f);
	if (FAILED(__super::Add_Components(TEXT("Com_SPHERE"), LEVEL_STATIC, TEXT("Prototype_Component_Colider_Sphere"), (CComponent**)&m_pSphereCom, &ColliderDesc)))
		return E_FAIL;

	return S_OK;
}

CTemoo* CTemoo::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CTemoo* pInstance = new CTemoo(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CTemoo");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CTemoo::Clone(void* pArg)
{
	CTemoo* pInstance = new CTemoo(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTemoo");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CTemoo::Free()
{
	__super::Free();
}
