#include "Enemy.h"
#include "Player.h"
#include "PickingManager.h"
#include "PlayerManager.h"
#include "Champion.h"
#include "CursorManager.h"
#include "Collision_Manager.h"
#include "UI_Enemy_HpBar.h"
#include "Minion.h"
#include "Building.h"

namespace
{
	bool    g_bEnemyDestLoaded = false;
	_float3 g_vEnemyLaneDest[3]; 

	// 목저지 점 한개일때 
	void Load_EnemyDest()
	{
		if (g_bEnemyDestLoaded)
			return;

		const _tchar* files[3] =
		{
			TEXT("../../../Bin/Data/PositionData/EnemyTopDestnation.dat"),
			TEXT("../../../Bin/Data/PositionData/EnemyMidDestnation.dat"),
			TEXT("../../../Bin/Data/PositionData/EnemyBottomDestnation.dat"),
		};

		ZeroMemory(g_vEnemyLaneDest, sizeof(g_vEnemyLaneDest));

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

			// 첫 번째 포인트만 목적지로 사용
			ReadFile(hFile, &d, sizeof(NONANIMDESC), &bytes, nullptr);
			if (bytes != 0)
				g_vEnemyLaneDest[i] = d.vPosition;

			CloseHandle(hFile);
		}

		g_bEnemyDestLoaded = true;
	}

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

// ?? Enemy 입장에서 "공격 가능한 타겟인가?" 필터
static bool IsEnemyValidTargetForAI(CActor* pA)
{
	if (!pA || pA->Get_Dead())
		return false;

	// 블루팀 미니언만
	if (auto pMinion = dynamic_cast<CMinion*>(pA))
	{
		if (pMinion->Get_Team() != TEAM_BLUE)
			return false;

		return true;
	}

	// 블루팀 건물만
	if (auto pBuilding = dynamic_cast<CBuilding*>(pA))
	{
		if (pBuilding->Get_TeamType() != TEAM_BLUE)
			return false;

		return true;
	}

	if (auto pChamp = dynamic_cast<CChampion*>(pA))
	{
		return true;
	}

	return false;
}


CEnemy::CEnemy(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CActor(pDevice, pContext)
{
}

CEnemy::CEnemy(const CEnemy& rhs)
	: CActor(rhs)
{
}

HRESULT CEnemy::Initialize_Prototype()
{
	if (FAILED(__super::Initialize_Prototype()))
		return E_FAIL;

	return S_OK;
}

HRESULT CEnemy::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	if (FAILED(Ready_Components(pArg)))
		return E_FAIL;


	Ready_Animation_Event();

	NONANIMDESC ModelDesc;
	if (pArg)
		memcpy(&ModelDesc, pArg, sizeof(NONANIMDESC));

	if (pArg)
	{
		_vector vPosition = XMLoadFloat3(&ModelDesc.vPosition);
		vPosition = XMVectorSetW(vPosition, 1.f);
		m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vPosition);
		Set_Scale(ModelDesc.vScale);

		if (ModelDesc.m_fAngle != 0)
			m_pTransformCom->Rotation(XMVectorSet(0.f, 1.f, 0.f, 0.f), XMConvertToRadians(ModelDesc.m_fAngle));
	}
	CCollision_Manager::GetInstance()->Add_CollisionGroup(CCollision_Manager::COLLISION_ENEMY, this);

	Compute_CurrentIndex();
	CGameObject* pObj = nullptr;

	if (FAILED(m_pGameInstance->Add_GameObject_Out(
		TEXT("Prototype_GameObject_EnemyHpBar"),
		LEVEL_GAMEPLAY,
		TEXT("Layer_UI_HpBar"),
		pObj,
		this)))
	{
		return E_FAIL;
	}

	m_pHpBar = dynamic_cast<CEnemy_HpBar*>(pObj);
	if (m_pHpBar)
		m_pHpBar->Set_Owner(this);

	return S_OK;
}

int CEnemy::Update(_float fTimeDelta)
{
	if (m_bDead)
		return OBJ_DEAD;

	// ★ 모든 Enemy 스킬 타이머 공통 증가
	Tick_SkillTimers(fTimeDelta);

	__super::Update(fTimeDelta);
	Update_Collider();
	
	CPickingManager::GetInstance()->Out_PickingGroup(this);
	return OBJ_NOEVENT;
}

void CEnemy::Late_Update(_float fTimeDelta)
{
	if (m_bDead)
		return;

	__super::Late_Update(fTimeDelta);

	if (m_pRendererCom)
	{
		m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_NOALPHABLEND, this);
	}

	CPickingManager::GetInstance()->Add_PickingGroup(this);
}

HRESULT CEnemy::Render()
{
	if (!m_pShaderCom || !m_pModelCom)
		return E_FAIL;

	if (FAILED(SetUp_ShaderResources()))
		return E_FAIL;
	if (FAILED(SetUp_ShaderID()))
		return E_FAIL;

	_uint iNumMeshes = m_pModelCom->Get_NumMeshs();
	for (_uint i = 0; i < iNumMeshes; ++i)
	{
		if (FAILED(m_pModelCom->SetUp_Material(m_pShaderCom, "g_DiffuseTexture", i, aiTextureType_DIFFUSE)))
			return E_FAIL;
		if (FAILED(m_pModelCom->SetUp_Material(m_pShaderCom, "g_NormalTexture", i, aiTextureType_NORMALS)))
			return E_FAIL;

		if (FAILED(m_pModelCom->Render(m_pShaderCom, i, m_eShaderID)))
			return E_FAIL;
	}

	return S_OK;
}

HRESULT CEnemy::Render_ShadowDepth()
{
	return S_OK;
}

HRESULT CEnemy::Render_EdgeDetection()
{
	return S_OK;
}

_bool CEnemy::Picking(_float3* PickingPoint)
{
	CColider* pCollider = Get_Collider();
	if (pCollider == nullptr)
		return false;

	float   fDist = 0.f;
	_float3 vHitPos = {};

	// Collider Ray Picking
	if (pCollider->RayPick(m_pTransformCom, &vHitPos, &fDist))
	{
		CChampion* pPlayer = CPlayerManager::GetInstance()->Get_ActivePlayer();
		if (!pPlayer)
			return false;
		CCursorManager::GetInstance()->SetState(CURSOR_ATTACK);
		pPlayer->Set_AttackTarget(this);

		if (PickingPoint)
			*PickingPoint = vHitPos;

		return true;
	}

	ZeroMemory(&m_vMousePickedPos, sizeof(_float3));
	return false;
}

void CEnemy::PickingTrue()
{
}

void CEnemy::Set_Picked()
{

}

void CEnemy::Find_Target()
{
	if (m_pTarget)
	{
		Check_FiledTarget();
		return;
	}

	CActor* best = nullptr;

	// ?? 어그로 범위: 공격 사거리 + 추가 어그로 범위
	float aggroRange = m_tStats.AttackRange + m_fAggroBonus;  // ★ 여기
	float bestDist = aggroRange;;

	CCollision_Manager* pCM = CCollision_Manager::GetInstance();
	if (!pCM || !m_pTransformCom)
		return;

	auto scanGroup = [&](CCollision_Manager::COLLISION_GROUP eGroup)
		{
			auto& group = pCM->Get_CollisionGroup(eGroup);
			for (auto obj : group)
			{
				CActor* pA = dynamic_cast<CActor*>(obj);
				if (!IsEnemyValidTargetForAI(pA))
					continue;   // ?? 여기서 아군(레드) 미니언/건물 다 걸러짐

				float d = Target_Distance(pA);
				if (d < bestDist)
				{
					bestDist = d;
					best = pA;
				}
			}
		};

	// 1) 미니언 전체 중에서 "블루팀 미니언만" 필터됨
	scanGroup(CCollision_Manager::COLLISION_MINION);

	// 2) 건물 전체 중에서 "블루팀 건물만" 필터됨
	scanGroup(CCollision_Manager::COLLISION_BUILDING);

	// 3) 플레이어 챔피언 (블루 플레이어)
	scanGroup(CCollision_Manager::COLLISION_PLAYER);

	if (best)
		m_pTarget = best;
}

CActor* CEnemy::Find_MinDistance_Target()
{
	return nullptr;
}

CActor* CEnemy::Check_FiledTarget()
{
	if (!m_pTarget)
		return nullptr;

	CCollision_Manager* pCM = CCollision_Manager::GetInstance();
	if (!pCM)
	{
		m_pTarget = nullptr;
		return nullptr;
	}

	// ?? 0) 기본 조건: 아직 유효한 공격 타겟인지 (팀/죽음 체크)
	if (!IsEnemyValidTargetForAI(m_pTarget))
	{
		m_pTarget = nullptr;
		return nullptr;
	}

	auto isInGroup = [&](CCollision_Manager::COLLISION_GROUP eGroup)->bool
		{
			auto& group = pCM->Get_CollisionGroup(eGroup);
			for (auto obj : group)
			{
				if (obj == m_pTarget)
					return true;
			}
			return false;
		};

	bool exist =
		isInGroup(CCollision_Manager::COLLISION_MINION) ||
		isInGroup(CCollision_Manager::COLLISION_BUILDING) ||
		isInGroup(CCollision_Manager::COLLISION_PLAYER);

	if (!exist)
	{
		m_pTarget = nullptr;
		return nullptr;
	}


	// Check_FiledTarget
	float aggroRange = m_tStats.AttackRange + m_fAggroBonus;
	if (Target_Distance(m_pTarget) > aggroRange * m_fAggroReleaseMul)
	{
		m_pTarget = nullptr;
		return nullptr;
	}

	return m_pTarget;
}

void CEnemy::Follow_Lane(_float fTimeDelta)
{
	// =======================
   // 0) 타겟 찾기 / 유지
   // =======================
	Find_Target();
	CActor* pTarget = Check_FiledTarget();

	// =======================
	// 1) 타겟 있으면 → 추적
	// =======================
	if (pTarget && m_pTransformCom)
	{
		m_bWasChasing = true;   // ★ 지금은 추격 중

		CTransform* pTr = m_pTransformCom;

		_vector myPos = pTr->Get_State(CTransform::STATE_TRANSLATION);
		_vector tPos = pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

		_vector vDir = XMVectorSetY(tPos - myPos, 0.f);
		float   dist = XMVectorGetX(XMVector3Length(vDir));
		if (dist > 0.001f)
		{
			vDir = XMVector3Normalize(vDir);

			float stopDist = Get_AttackRangeWorld();

			if (dist > stopDist)
			{
				myPos += vDir * (m_tStats.MoveSpeed * fTimeDelta);
				pTr->Set_State(CTransform::STATE_TRANSLATION, myPos);
			}

			_vector vLook = XMVector3Normalize(XMVectorSetY(myPos - tPos, 0.f));
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

		Check_Navigation();
		return;
	}

	// =======================
	// 2) 타겟 없음 → 라인 이동
	// =======================
	CTransform* pTr = m_pTransformCom;
	CNavigation* pNav = m_pNavigationCom;
	if (!pTr || !pNav)
		return;

	// ★ 방금 전까지는 추격 중이었는데, 지금은 타겟이 없을 때만
	//    한 번 인덱스 보정
	if (m_bWasChasing)
	{
		RefreshLaneNodeByPosition();
		m_bWasChasing = false;
	}

	// 기존 Move_ByPath 로직 그대로
	if (!pTr->Move_ByPath(fTimeDelta, pNav))
	{
		const auto& path = Get_EnemyLanePath(m_eLineType);
		if (path.empty())
			return;

		const int pathSize = (int)path.size();
		if (pathSize == 0)
			return;

		_int nextIdx = m_iLaneNodeIndex + 1;

		if (nextIdx < 0 || nextIdx >= pathSize)
			return;

		m_iLaneNodeIndex = nextIdx;

		_float3 vTarget = path[nextIdx];
		_vector vTargetV = XMLoadFloat3(&vTarget);

		pTr->Set_Destination(vTargetV, pNav);
	}

	Check_Navigation();
}

void CEnemy::Compute_CurrentIndex()
{
	if (!m_pNavigationCom || !m_pTransformCom)
		return;

	_vector vPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
	m_pNavigationCom->Compute_CurrentIndex_byXZ(vPos);
}

_float3 CEnemy::Get_EnemyLaneDest(LINE_TYPE eLine)
{
	Load_EnemyDest();   

	switch (eLine)
	{
	case LINE_TOP:    return g_vEnemyLaneDest[0];
	case LINE_MID:    return g_vEnemyLaneDest[1];
	case LINE_BOTTOM: return g_vEnemyLaneDest[2];
	default:          return g_vEnemyLaneDest[1]; 
	}
}

const vector<_float3>& CEnemy::Get_EnemyLanePath(LINE_TYPE eLine)
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

void CEnemy::RefreshLaneNodeByPosition()
{
	const auto& path = Get_EnemyLanePath(m_eLineType);
	if (path.empty() || !m_pTransformCom)
		return;
	_vector vMy = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
	_float3 myPos;
	XMStoreFloat3(&myPos, XMVectorSetY(vMy, 0.f));

	int pathSize = (int)path.size();
	if (pathSize <= 0)
		return;

	if (m_iLaneNodeIndex < 0)
		m_iLaneNodeIndex = 0;
	if (m_iLaneNodeIndex >= pathSize)
		m_iLaneNodeIndex = pathSize - 1;

	// 경로를 지났으면 다음 경로로 
	while (m_iLaneNodeIndex < pathSize - 1)
	{
		const _float3& n0 = path[m_iLaneNodeIndex];
		const _float3& n1 = path[m_iLaneNodeIndex + 1];

		_float3 seg = { n1.x - n0.x, 0.f, n1.z - n0.z };      
		_float3 rel = { myPos.x - n0.x, 0.f, myPos.z - n0.z };       

		float segLenSq = seg.x * seg.x + seg.z * seg.z;
		if (segLenSq < 0.0001f)
		{
			++m_iLaneNodeIndex;
			continue;
		}

		float dot = seg.x * rel.x + seg.z * rel.z;

		if (dot >= 0.f)
		{
			++m_iLaneNodeIndex;
			continue;
		}
		else
		{
			break;
		}
	}

	if (m_iLaneNodeIndex >= pathSize)
		m_iLaneNodeIndex = pathSize - 1;

	const _float3& dst = path[m_iLaneNodeIndex];
	_vector vDst = XMLoadFloat3(&dst);
	m_pTransformCom->Set_Destination(vDst, m_pNavigationCom);
}

_float CEnemy::Target_Distance(CActor* pTarget) const
{
	if (!pTarget || !m_pTransformCom)
		return FLT_MAX;

	_vector a = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
	_vector b = pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

	a = XMVectorSetY(a, 0.f);
	b = XMVectorSetY(b, 0.f);

	return XMVectorGetX(XMVector3Length(b - a));
}

HRESULT CEnemy::SetUp_ShaderResources()
{
	if (!m_pShaderCom || !m_pTransformCom)
		return E_FAIL;

	_float4x4 w = m_pTransformCom->Get_World_TP();
	_float4x4 v = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_VIEW);
	_float4x4 p = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_PROJ);

	if (FAILED(m_pShaderCom->Set_RawValue("g_WorldMatrix", &w, sizeof(_float4x4)))) return E_FAIL;
	if (FAILED(m_pShaderCom->Set_RawValue("g_ViewMatrix", &v, sizeof(_float4x4)))) return E_FAIL;
	if (FAILED(m_pShaderCom->Set_RawValue("g_ProjMatrix", &p, sizeof(_float4x4)))) return E_FAIL;

	return S_OK;
}

HRESULT CEnemy::SetUp_ShaderID()
{
	return S_OK;
}

_int CEnemy::Take_Damage(int iDamage, CActor* DamageCauser)
{
	m_tStats.Hp -= iDamage;
	if (m_tStats.Hp < 0.f)
		m_tStats.Hp = 0.f;

	if (m_tStats.Hp <= 0.f)
	{
		Mark_Dead();
	}

	return (_int)m_tStats.Hp;
}

void CEnemy::Mark_Dead()
{
	if (m_bDead)
		return;

	m_bDead = true;

	CPlayerManager* pPM = CPlayerManager::GetInstance();
	if (pPM)
	{
		CChampion* pChampion = pPM->Get_ActivePlayer();
		if (pChampion && pChampion->Get_AttackTarget() == this)
		{
			pChampion->Set_AttackTarget(nullptr);
		}
	}

	CCollision_Manager::GetInstance()->Out_CollisionGroup(CCollision_Manager::COLLISION_ENEMY, this);
	CPickingManager::GetInstance()->Out_PickingGroup(this);
}



void CEnemy::Tick_SkillTimers(float fDeltaTime)
{
	for (int i = 0; i < (int)SKILL_END; ++i)
		m_tSkill[i].fTimer += fDeltaTime;
}

bool CEnemy::CanUseSkill(AISKILLTYPE eType, float fDist) const
{
	if (eType <= SKILL_NONE || eType >= SKILL_END)
		return false;

	const AISKILLINFO& info = m_tSkill[eType];

	if (info.fTimer < info.fCooldown)
		return false;

	if (fDist > info.fRange)
		return false;

	return true;
}

void CEnemy::ConsumeSkill(AISKILLTYPE eType)
{
	if (eType <= SKILL_NONE || eType >= SKILL_END)
		return;

	m_tSkill[eType].fTimer = 0.f;
}

CEnemy::AISKILLTYPE CEnemy::DecideRandomSkill(float fDistToTarget)
{
	AISKILLTYPE candList[] =
	{
		SKILL_Q,
		SKILL_E,
		SKILL_W,
		SKILL_R
	};

	for (AISKILLTYPE type : candList)
	{
		if (!CanUseSkill(type, fDistToTarget))
			continue;

		const AISKILLINFO& info = m_tSkill[type];

		float r = (rand() % 1000) / 1000.f; // 0~1
		if (r <= info.fUseProb)
		{
			m_eNextSkill = type;
			return type;
		}
	}

	m_eNextSkill = SKILL_NONE;
	return SKILL_NONE;
}



void CEnemy::Free()
{
	__super::Free();
	CCollision_Manager::GetInstance()->Out_CollisionGroup(CCollision_Manager::COLLISION_ENEMY, this);
}
