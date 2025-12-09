#include "Vayne.h"
#include "VayneState.h"
#include "VayneIdleState.h"
#include "Champion.h"
#include "Building.h"
#include "VayneReturnState.h"

using namespace Vayne;

namespace
{
	bool g_bEnemyLanePathLoaded = false;
	std::vector<_float3> g_EnemyLanePoints[3];

	void Load_EnemyLanePaths()
	{
		if (g_bEnemyLanePathLoaded)
			return;

		const _tchar* files[3] =
		{
			TEXT("../../../Bin/Data/PositionData/EnemyTopDestnation.dat"),
			TEXT("../../../Bin/Data/PositionData/EnemyMidDestnation.dat"),
			TEXT("../../../Bin/Data/PositionData/EnemyBottomDestnation.dat"),
		};

		for (int i = 0; i < 3; ++i)
		{
			HANDLE hFile = CreateFile(
				files[i],
				GENERIC_READ, 0, nullptr,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL, 0);

			if (hFile == INVALID_HANDLE_VALUE)
				continue;

			_ulong      bytes = 0;
			_uint       count = 0;
			NONANIMDESC d{};

			ReadFile(hFile, &count, sizeof(_uint), &bytes, nullptr);
			if (bytes == 0 || count == 0)
			{
				CloseHandle(hFile);
				continue;
			}

			g_EnemyLanePoints[i].clear();
			g_EnemyLanePoints[i].reserve(count);

			for (_uint j = 0; j < count; ++j)
			{
				ReadFile(hFile, &d, sizeof(NONANIMDESC), &bytes, nullptr);
				if (bytes == 0)
					break;

				g_EnemyLanePoints[i].push_back(d.vPosition);
			}

			CloseHandle(hFile);
		}

		g_bEnemyLanePathLoaded = true;
	}
}
const std::vector<_float3>& CVayne::Get_EnemyLanePath(LINE_TYPE eLine)
{
	Load_EnemyLanePaths();

	switch (eLine)
	{
	case LINE_TOP:    return g_EnemyLanePoints[0];
	case LINE_MID:    return g_EnemyLanePoints[1];
	case LINE_BOTTOM: return g_EnemyLanePoints[2];
	}
	static std::vector<_float3> dummy;
	return dummy;
}

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
	m_fAggroBonus = 4.f;   // 공격 사거리보다 살짝 넓게
	m_fAggroReleaseMul = 1.4f;  // 물고 난 뒤 조금만 더 쫓아감
	Set_Scale(_float3(0.012f, 0.012f, 0.012f));
	m_tStats.MaxHp = 5000.f;
	m_tStats.Hp = m_tStats.MaxHp;
	m_tStats.MoveSpeed = 4.f;
	m_tStats.AttackRange = 8.f;     
	m_tStats.AttackDamage = 60.f;
	m_tStats.AttackSpeed = 0.4f;
	// 라인 타입 (일단 MID)
	m_eLineType = LINE_MID;

	// 한 점 목적지는 그대로
	_float3 vDest = Get_EnemyLaneDest(m_eLineType);
	Set_LaneTarget(vDest);

	// ★ 경로 시작 인덱스: 내 현재 위치에서 가장 가까운 노드
	const auto& lane = CVayne::Get_EnemyLanePath(m_eLineType);
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

void CVayne::Follow_Lane(_float fTimeDelta)
{
	// =======================
// 0) 우선 타겟 찾기 / 유지
// =======================
// Enemy 공통 함수
	Find_Target();                 // 근처에 적(블루 미니언/챔프/건물)이 있으면 m_pTarget 세팅
	CActor* pTarget = Check_FiledTarget(); // 너무 멀어지면 nullptr로 해제

	// =======================
	// 1) 타겟이 있으면 → 타겟 추적 (미니언처럼)
	// =======================
	if (pTarget && m_pTransformCom)
	{
		CTransform* pTr = m_pTransformCom;

		_vector myPos = pTr->Get_State(CTransform::STATE_TRANSLATION);
		_vector tPos = pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

		// XZ 평면에서 방향 벡터
		_vector vDir = XMVectorSetY(tPos - myPos, 0.f);
		float   dist = XMVectorGetX(XMVector3Length(vDir));
		if (dist > 0.001f)
		{
			vDir = XMVector3Normalize(vDir);

			// 멈출 거리 = 공격 사거리
			float stopDist = Get_AttackRangeWorld();     // m_tStats.AttackRange 사용

			// 아직 사거리 밖이면 계속 전진
			if (dist > stopDist)
			{
				myPos += vDir * (m_tStats.MoveSpeed * fTimeDelta);
				pTr->Set_State(CTransform::STATE_TRANSLATION, myPos);
			}

			// 타겟 바라보게 회전 (대충 미니언 Face_Target 과 비슷하게)
			_vector vLook = XMVector3Normalize(XMVectorSetY(myPos - tPos, 0.f)); // ★ 여기만 변경
			_vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
			_vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vLook));

			float fScale = XMVectorGetX(XMVector3Length(pTr->Get_State(CTransform::STATE_RIGHT)));
			if (fScale < 0.0001f) fScale = 1.f;

			vRight *= fScale;
			vLook *= fScale;
			vUp = XMVector3Normalize(XMVector3Cross(vLook, vRight)) * fScale;

			vRight = XMVectorSetW(vRight, 0.f);
			vUp = XMVectorSetW(vUp, 0.f);
			vLook = XMVectorSetW(vLook, 0.f);

			pTr->Set_State(CTransform::STATE_RIGHT, vRight);
			pTr->Set_State(CTransform::STATE_UP, vUp);
			pTr->Set_State(CTransform::STATE_LOOK, vLook);
		}

		// 네비 높이 보정
		Check_Navigation();
		return;    // ★ 타겟 추적 중이면 여기서 끝
	}

	// =======================
	// 2) 타겟 없으면 → 기존 라인 이동
	// =======================
	CTransform* pTr = m_pTransformCom;
	CNavigation* pNavi = m_pNavigationCom;
	if (!pTr || !pNavi)
		return;

	// 1) 현재 목적지까지 Move_ByPath 시도
	if (!pTr->Move_ByPath(fTimeDelta, pNavi))
	{
		// 2) 도착했으면 다음 노드로
		const auto& path = CVayne::Get_EnemyLanePath(m_eLineType);
		if (path.empty())
			return;

		const int pathSize = (int)path.size();
		if (pathSize == 0)
			return;

		// BLUE → RED 방향 기준
		_int nextIdx = m_iLaneNodeIndex + 1;

		// 끝에 도달했으면 그냥 멈춤
		if (nextIdx < 0 || nextIdx >= pathSize)
			return;

		m_iLaneNodeIndex = nextIdx;

		_float3 vTarget = path[nextIdx];
		_vector vTargetV = XMLoadFloat3(&vTarget);

		pTr->Set_Destination(vTargetV, pNavi);
	}

	// 3) 높이 보정
	Check_Navigation();
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
