#include "Vayne.h"
#include "VayneState.h"
#include "VayneIdleState.h"
#include "Champion.h"
#include "Building.h"
#include "VayneReturnState.h"

using namespace Vayne;


_bool CVayne::Is_AnimationLoop(_uint eAnimId)
{
	switch ((ANIM)eAnimId)
	{
	case IDLE:
	case IDLE1:
	case IDLE2:
	case IDLE3:
	case IDLE4:
	case RUN:
	case Q_RUN:      // ?? 추가
		return true;
	default:
		return false;
			
	}

}

void CVayne::Ready_Animation_Event()
{
	CAnimation* pAnim = m_pModelCom->Get_Animations()[ATTACK1];
	if (!pAnim) return;

	ANIMEVENT evt{};
	evt.fStartTime = 6.17f;
	evt.fEndTime = 7.f;
	evt.eType = ANIMEVENT::EVENT_PROJECTILE;

	pAnim->Add_Event(evt);;

	pAnim = m_pModelCom->Get_Animations()[ATTACK2];
	evt.fStartTime = 5.4f;
	evt.fEndTime = 6.f;
	evt.eType = ANIMEVENT::EVENT_PROJECTILE;
	pAnim->Add_Event(evt);;

	pAnim = m_pModelCom->Get_Animations()[CRIT];
	evt.fStartTime = 5.2f;
	evt.fEndTime = 6.f;
	evt.eType = ANIMEVENT::EVENT_PROJECTILE;
	pAnim->Add_Event(evt);;

	pAnim = m_pModelCom->Get_Animations()[Q_ATTACK];
	evt.fStartTime = 10.f;
	evt.fEndTime = 11.f;
	evt.eType = ANIMEVENT::EVENT_PROJECTILE;
	pAnim->Add_Event(evt);;

	pAnim = m_pModelCom->Get_Animations()[E];
	evt.fStartTime = 7.f;
	evt.fEndTime = 9.f;
	evt.eType = ANIMEVENT::EVENT_PROJECTILE;
	pAnim->Add_Event(evt);;
}



CVayne::CVayne(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CEnemy(pDevice, pContext)
{
}

CVayne::CVayne(const CVayne& rhs)
	: CEnemy(rhs)
{
}

HRESULT CVayne::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CVayne::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	CVayneState* pState = new CIdleState(this);
	m_pState = m_pState->ChangeState(m_pState, pState);

	m_eEnemyType = VAYNE;
	m_fAggroBonus = 4.f;
	m_fAggroReleaseMul = 1.4f;

	Set_Scale(_float3(0.012f, 0.012f, 0.012f));
	m_tStats.MaxHp = 100.f;
	m_tStats.Hp = m_tStats.MaxHp;
	m_tStats.MoveSpeed = 4.f;
	m_tStats.AttackRange = 8.f;
	m_tStats.AttackDamage = 60.f;
	m_tStats.AttackSpeed = 0.4f;
	m_tAggro.fAttackRange = 8.f;
	m_tAggro.fChaseRange = 10.f;   
	m_tAggro.fGiveUpRange = 16.f;   

	// 라인 타입 (이 Enemy가 어느 라인 담당인지)
	m_eLineType = LINE_MID; 

	// 한 점 목적지는 그대로
	_float3 vDest = CEnemy::Get_EnemyLaneDest(m_eLineType);
	Set_LaneTarget(vDest);

	// ★ 경로 시작 인덱스: 내 현재 위치에서 가장 가까운 노드
	const auto& lane = Get_EnemyLanePath(m_eLineType);
	m_iLaneNodeIndex = 0;
	if (!lane.empty() && m_pTransformCom)
	{
		_vector vMyPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
		vMyPos = XMVectorSetY(vMyPos, 0.f);

		float bestDistSq = FLT_MAX;
		int   bestIdx = 0;

		for (int i = 0; i < (int)lane.size(); ++i)
		{
			_vector vNode = XMLoadFloat3(&lane[i]);
			vNode = XMVectorSetY(vNode, 0.f);

			float d2 = XMVectorGetX(XMVector3LengthSq(vNode - vMyPos));
			if (d2 < bestDistSq)
			{
				bestDistSq = d2;
				bestIdx = i;
			}
		}

		m_iLaneNodeIndex = bestIdx;
	}
	m_tSkill[SKILL_E].fCooldown = 10.F;

	// ★ 베인 전용 스킬 AI 값 설정
	// Q
	// Q
	m_tSkill[SKILL_Q].fCooldown = 3.5f;
	m_tSkill[SKILL_Q].fTimer = 0.f;
	m_tSkill[SKILL_Q].fRange = 9.f;        // 공격 사거리보다 살짝 큼
	m_tSkill[SKILL_Q].fUseProb = 0.45f;    // ?? 살짝 낮춤

	// W
	m_tSkill[SKILL_W].fCooldown = 12.f;
	m_tSkill[SKILL_W].fTimer = 0.f;
	m_tSkill[SKILL_W].fRange = 7.f;
	m_tSkill[SKILL_W].fUseProb = 0.15f;


	// E  ← ★★★ 핵심
	m_tSkill[SKILL_E].fCooldown = 6.0f;    // ★ 제대로 지정
	m_tSkill[SKILL_E].fTimer = 0.f;
	m_tSkill[SKILL_E].fRange = 8.5f;       // ★ 공격 사거리랑 맞춤
	m_tSkill[SKILL_E].fUseProb = 0.85f;    // ★ 체감 확 올라감

	// R
	m_tSkill[SKILL_R].fCooldown = 60.f;
	m_tSkill[SKILL_R].fTimer = 0.f;
	m_tSkill[SKILL_R].fRange = 8.f;
	m_tSkill[SKILL_R].fUseProb = 0.05f;

	return S_OK;
}

int CVayne::Update(_float fTimeDelta)
{
	// ★ 무조건 Enemy Update 먼저 태워야 함
	int iEvent = __super::Update(fTimeDelta);

	// 죽어있으면 AI / State 돌리지 말고 끝
	if (m_bDeadState)
		return iEvent;

	AI_Behavior(fTimeDelta);
	Update_State(fTimeDelta);

	// Q 강화 시간 처리
	if (m_bQEmpowered && !m_bConsumeQThisAttack)
	{
		m_fQEmpoweredTime -= fTimeDelta;
		if (m_fQEmpoweredTime <= 0.f)
			m_bQEmpowered = false;
	}

	return iEvent;

}

void CVayne::Late_Update(_float fTimeDelta)
{
	if (m_bDead)
		return;

	__super::Late_Update(fTimeDelta);
	LateUpdate_State(fTimeDelta);
}

HRESULT CVayne::Render()
{
	__super::Render();
	return S_OK;
}

HRESULT CVayne::Render_Glow()
{
	return S_OK;
}

void CVayne::AI_Behavior(_float fTimeDelta)
{
	CVayneState* pNewState = m_pState->AI_Behaviour(fTimeDelta);
	if (pNewState)
		m_pState = m_pState->ChangeState(m_pState, pNewState);
}

void CVayne::Update_State(_float fTimeDelta)
{
	CVayneState* pNewState = m_pState->Update(fTimeDelta);
	if (pNewState)
		m_pState = m_pState->ChangeState(m_pState, pNewState);
}

void CVayne::LateUpdate_State(_float fTimeDelta)
{
	CVayneState* pNewState = m_pState->LateUpdate(fTimeDelta);
	if (pNewState)
		m_pState = m_pState->ChangeState(m_pState, pNewState);
}

_int CVayne::Take_Damage(int iDamage, CActor* DamageCauser)
{
	_int iHp = __super::Take_Damage(iDamage, DamageCauser);
	if (iHp <= 0)
		return iHp;

	if (!m_pState || dynamic_cast<CReturnState*>(m_pState))
		return iHp;

	// 1) 포탑(혹은 건물)에게 맞은 경우 → 무조건 도망
	if (auto pBuilding = dynamic_cast<CBuilding*>(DamageCauser))
	{
		CVayneState* pNew = new CReturnState(this);
		m_pState = m_pState->ChangeState(m_pState, pNew);
		return iHp;
	}

	// 2) 플레이어나 기타 경우 → 확률로 도망
	if (auto pChamp = dynamic_cast<CChampion*>(DamageCauser))
	{
		int roll = rand() % 100;
		if (roll < 40)
		{
			CVayneState* pNew = new CReturnState(this);
			m_pState = m_pState->ChangeState(m_pState, pNew);
		}
	}

	return iHp;
}

void CVayne::Check_Navigation()
{
	_vector vPosition = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
	_float m_fWalkingHeight = m_pNavigationCom->Compute_Height(vPosition, 0.f);

	vPosition = XMVectorSetY(vPosition, m_fWalkingHeight);
	m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vPosition);
}

void CVayne::OnRespawned()
{
	// 1) 상태머신은 "ChangeState"로 정상 진입시켜라 (Safe_Delete 하지마)
	if (m_pState)
	{
		CVayneState* pNew = new CIdleState(this);
		m_pState = m_pState->ChangeState(m_pState, pNew);
	}
	else
	{
		m_pState = new CIdleState(this);
	}

	// 2) 베인 전용 플래그 리셋
	m_bJustAttacked = false;
	m_fPostAttackMoveTime = 0.f;

	m_vKitingDest = XMVectorZero();
	m_bDoKiting = false;

	m_bQEmpowered = false;
	m_bConsumeQThisAttack = false;
	m_fQEmpoweredTime = 0.f;

	m_bSkillLock = false;

	// 3) 부활 위치 기준으로 라인 경로 인덱스/목적지 재설정 (중요)
	if (m_pTransformCom && m_pNavigationCom)
	{
		// 부활 위치에서 가장 가까운 노드로 destination 다시 잡기
		RefreshLaneNodeByPosition();
	}

	// 4) 애니도 Idle로 확실히
	if (m_pModelCom)
		m_pModelCom->Set_CurrentAnimIndex(CVayne::ANIM::IDLE);
}


HRESULT CVayne::Ready_Components(void* pArg)
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
	if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Vayne"), (CComponent**)&m_pModelCom)))
		return E_FAIL;

	CNavigation::NAVDESC NaviDesc;
	ZeroMemory(&NaviDesc, sizeof NaviDesc);
	if (FAILED(__super::Add_Components(TEXT("Com_Navigation"), LEVEL_STATIC, TEXT("Prototype_Component_ChampionNavigation"), (CComponent**)&m_pNavigationCom, &NaviDesc)))
		return E_FAIL;


	CColider::COLLIDERDESC ColliderDesc;
	ZeroMemory(&ColliderDesc, sizeof(CColider::COLLIDERDESC));
	ColliderDesc.vScale = _float3(1., 1.f, 1.f);
	ColliderDesc.vRotation = _float3(0.f, 0.f, 0.f);
	ColliderDesc.vPosition = _float3(0.f, 80.f, 0.f);
	if (FAILED(__super::Add_Components(TEXT("Com_SPHERE"), LEVEL_STATIC, TEXT("Prototype_Component_Colider_Sphere"), (CComponent**)&m_pSphereCom, &ColliderDesc)))
		return E_FAIL;
	return S_OK;
}

HRESULT CVayne::SetUp_ShaderID()
{
	m_eShaderID = SHADER_ANIMDEFAULT;

	return S_OK;
}

CVayne* CVayne::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CVayne* pInstance = new CVayne(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CVayne");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CVayne::Clone(void* pArg)
{
	CVayne* pInstance = new CVayne(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CVayne");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CVayne::Free()
{
	__super::Free();
	Safe_Delete(m_pState);
}
