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
	m_tStats.MaxHp = 5000.f;
	m_tStats.Hp = m_tStats.MaxHp;
	m_tStats.MoveSpeed = 4.f;
	m_tStats.AttackRange = 8.f;
	m_tStats.AttackDamage = 60.f;
	m_tStats.AttackSpeed = 0.4f;

	// 라인 타입 (이 Enemy가 어느 라인 담당인지)
	m_eLineType = LINE_MID;   // 탑/바텀 Enemy는 LINE_TOP / LINE_BOTTOM 설정

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

	// ★ 베인 전용 스킬 AI 값 설정
	// Q
	m_tSkill[SKILL_Q].fCooldown = 8.f;
	m_tSkill[SKILL_Q].fTimer = 0.f;
	m_tSkill[SKILL_Q].fRange = 7.f;
	m_tSkill[SKILL_Q].fUseProb = 0.25f;  // 25%

	// W
	m_tSkill[SKILL_W].fCooldown = 12.f;
	m_tSkill[SKILL_W].fTimer = 0.f;
	m_tSkill[SKILL_W].fRange = 7.f;
	m_tSkill[SKILL_W].fUseProb = 0.15f;

	// E
	m_tSkill[SKILL_E].fCooldown = 10.f;
	m_tSkill[SKILL_E].fTimer = 0.f;
	m_tSkill[SKILL_E].fRange = 5.f;
	m_tSkill[SKILL_E].fUseProb = 0.1f;

	// R
	m_tSkill[SKILL_R].fCooldown = 60.f;
	m_tSkill[SKILL_R].fTimer = 0.f;
	m_tSkill[SKILL_R].fRange = 8.f;
	m_tSkill[SKILL_R].fUseProb = 0.05f;

	return S_OK;
}

int CVayne::Update(_float fTimeDelta)
{
	// 1) 이미 죽었으면 바로 리턴
	if (m_bDead)
		return OBJ_DEAD;

	// 2) 부모 Update 호출하고, 여기 리턴값을 그대로 반영
	int iEvent = __super::Update(fTimeDelta);
	if (iEvent < 0)       // OBJ_DEAD(-1) 이나 OBJ_POOL(-2) 같은 값이면
		return iEvent;

	// 3) 살아 있는 경우에만 AI / State 진행
	AI_Behavior(fTimeDelta);
	Update_State(fTimeDelta);

	m_fTimeDeltaAcc += fTimeDelta;

	return OBJ_NOEVENT;
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
	// 1) 기본 Enemy 데미지 처리 (HP 감소 + 죽으면 Mark_Dead)
	_int iHp = __super::Take_Damage(iDamage, DamageCauser);

	// 이미 죽었으면 더 할 것 없음
	if (iHp <= 0)
		return iHp;

	// 상태가 없거나 이미 ReturnState라면 또 바꿀 필요 X
	if (!m_pState || dynamic_cast<CReturnState*>(m_pState))
		return iHp;

	// 2) 공격자가 Player(Champion) 이거나 Building 인지 체크
	bool bFromPlayerOrBuilding =
		(nullptr != dynamic_cast<CChampion*>(DamageCauser)) ||
		(nullptr != dynamic_cast<CBuilding*>(DamageCauser));

	if (bFromPlayerOrBuilding)
	{
		int roll = rand() % 100;
		if (roll < 40)   // 0~19 → 20%
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
