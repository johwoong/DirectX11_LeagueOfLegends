#include "Minion.h"
#include "MinionManager.h"
#include "PoolManager.h"
#include "PickingManager.h"
#include "CursorManager.h"
#include "Champion.h"
#include "Building.h" 
#include "PlayerManager.h"
#include "Collision_Manager.h"
#include "UI_Minion_HpBar.h"


CMinion::CMinion(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CActor(pDevice, pContext)
{
}

CMinion::CMinion(const CMinion& rhs)
    : CActor(rhs)
    , m_tStats(rhs.m_tStats)
    , m_eState(rhs.m_eState)
    , m_eMinionType(rhs.m_eMinionType)
    , m_eTeam(rhs.m_eTeam)
    , m_eLane(rhs.m_eLane)
    , m_fAttackCoolTime(rhs.m_fAttackCoolTime)
    , m_fAttackTimer(rhs.m_fAttackTimer)
    , m_iWaveIndex(rhs.m_iWaveIndex)
    , m_iLaneIndex(rhs.m_iLaneIndex)
    , m_iGoldReward(rhs.m_iGoldReward)
    , m_iExpReward(rhs.m_iExpReward)
    , m_pModelCom(rhs.m_pModelCom)
    , m_pNavigationCom(rhs.m_pNavigationCom)
    , m_pTarget(rhs.m_pTarget)
    , m_pOrginDamageCauser(rhs.m_pOrginDamageCauser)
{
    Safe_AddRef(m_pModelCom);
    Safe_AddRef(m_pNavigationCom);
}


HRESULT CMinion::Initialize_Prototype()
{
    return __super::Initialize_Prototype();
}

HRESULT CMinion::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (pArg)
    {
        MINION_SPAWNDESC* pDesc = (MINION_SPAWNDESC*)pArg;
        m_eTeam = pDesc->eTeam;
        m_eLane = pDesc->eLane;
        m_eMinionType = pDesc->eMinionType;
        m_iLaneIndex = pDesc->iLaneIndex;
        m_iWaveIndex = pDesc->iWaveIndex;
        m_vTargetPos = pDesc->vTargetPos;
    }

    Compute_LaneOffset();

    if (CMinionManager* mgr = CMinionManager::GetInstance())
    {
        const auto& path = mgr->GetLanePath(m_eLane);
        m_iLaneNodeIndex = mgr->GetLaneStartIndex(m_eTeam, path);
    }

    if (FAILED(Ready_Components(pArg)))
        return E_FAIL;



    CNavigation::NAVDESC NaviDesc{};
    if (FAILED(__super::Add_Components(
        TEXT("Com_Navigation"), LEVEL_STATIC,
        TEXT("Prototype_Component_ChampionNavigation"),
        (CComponent**)&m_pNavigationCom, &NaviDesc)))
        return E_FAIL;

    m_tStats.m_fCurrentHp = m_tStats.m_fMaxHp;

    m_fAttackCoolTime = (m_tStats.m_fAttackSpeed > 0.f)
        ? 1.f / m_tStats.m_fAttackSpeed : 1.f;

    m_fAttackTimer = 0.f;
    m_eState = STATE_MOVE;
    Compute_CurrentIndex();
    Ready_Animation_Event();
    Create_HpBar();

    return S_OK;
}



int CMinion::Update(_float fTimeDelta)
{
    if (m_bDead)
        return OBJ_POOL;

    if (FAILED(__super::Update(fTimeDelta)))
        return -1;

    m_fAttackTimer += fTimeDelta;

    if (m_pTarget)
        Check_FiledTarget();

    Update_Collider();
    CPickingManager::GetInstance()->Out_PickingGroup(this);
    return 0;
}

void CMinion::Late_Update(_float fTimeDelta)
{
    __super::Late_Update(fTimeDelta);

    if (!Is_Frustum(Check_CullingRadius()))
        return;

    if (m_pRendererCom && Is_Frustum(Check_CullingRadius()))
        m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_NOALPHABLEND, this);

    // ?? 죽는 중 / 죽음 상태면 픽킹 그룹에 다시 안 넣기
    if (!m_bDead && !m_bDying)
        CPickingManager::GetInstance()->Add_PickingGroup(this);

    Apply_Separation(fTimeDelta);
}


HRESULT CMinion::Render()
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

HRESULT CMinion::Render_ShadowDepth()
{
    return __super::Render_ShadowDepth();
}

HRESULT CMinion::Render_EdgeDetection()
{
    return __super::Render_EdgeDetection();
}


void CMinion::Compute_CurrentIndex(LEVEL eLevel)
{
    UNREFERENCED_PARAMETER(eLevel);
    Compute_CurrentIndex();
}

void CMinion::Compute_CurrentIndex()
{
    if (!m_pNavigationCom || !m_pTransformCom)
        return;

    _vector vPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    m_pNavigationCom->Compute_CurrentIndex_byXZ(vPos);
}


_float CMinion::Target_Distance(CActor* pTarget) const
{
    if (!pTarget || !m_pTransformCom)
        return FLT_MAX;

    _vector a = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    _vector b = pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

    a = XMVectorSetY(a, 0.f);
    b = XMVectorSetY(b, 0.f);

    return XMVectorGetX(XMVector3Length(b - a));
}

void CMinion::Find_Target(_float fTimeDelta)
{
    // 이미 타겟 있으면 유지 여부만 체크
    if (m_pTarget)
    {
        Check_FiledTarget();
        return;
    }

    CActor* best = nullptr;
    float bestDist = m_tStats.m_fAggroRadius;

    // ================= 1) 적 미니언 탐색 (최우선) =================
    auto ScanMinionList = [&](const list<CGameObject*>* L)
        {
            if (!L) return;

            for (auto& obj : *L)
            {
                CMinion* m = dynamic_cast<CMinion*>(obj);
                if (!m || m->IsDeadOrDying())
                    continue;

                float d = Target_Distance(m);
                if (d < bestDist)
                {
                    bestDist = d;
                    best = m;
                }
            }
        };

    if (m_eTeam == TEAM_BLUE)
    {
        ScanMinionList(m_pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Minion_Red_Melee")));
        ScanMinionList(m_pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Minion_Red_Range")));
    }
    else // TEAM_RED
    {
        ScanMinionList(m_pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Minion_Blue_Melee")));
        ScanMinionList(m_pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Minion_Blue_Range")));
    }

    // ================= 2) 플레이어 탐색 (그 다음) =================
    {
        CChampion* pPlayer = CPlayerManager::GetInstance()->Get_ActivePlayer();

        // 예시 : 블루 플레이어만 있다고 가정 (RED 미니언이만 적으로 인식)
        if (pPlayer && m_eTeam != TEAM_BLUE)
        {
            float d = Target_Distance(pPlayer);
            if (d < bestDist)
            {
                bestDist = d;
                best = pPlayer;
            }
        }
    }

    // ================= 3) 빌딩 탐색 (타워/억제기/넥서스) =================
    {
        auto& buildingGroup =
            CCollision_Manager::GetInstance()->Get_CollisionGroup(
                CCollision_Manager::COLLISION_BUILDING);

        for (auto obj : buildingGroup)
        {
            if (!obj) continue;

            CBuilding* pB = dynamic_cast<CBuilding*>(obj);
            if (!pB || pB->Get_Dead())
                continue;

            // 같은 팀 빌딩은 무시
            if (pB->Get_TeamType() == m_eTeam)
                continue;

            float d = Target_Distance(pB);
            if (d < bestDist)
            {
                bestDist = d;
                best = pB;
            }
        }
    }

    // ================= 4) 적 챔피언(Enemy, Vayne 등) 탐색 =================
    if (m_eTeam == TEAM_BLUE)
    {
        auto& enemyGroup =
            CCollision_Manager::GetInstance()->Get_CollisionGroup(
                CCollision_Manager::COLLISION_ENEMY);

        for (auto obj : enemyGroup)
        {
            if (!obj) continue;

            CActor* pA = dynamic_cast<CActor*>(obj);
            if (!pA || pA->Get_Dead())
                continue;

            float d = Target_Distance(pA);
            if (d < bestDist)
            {
                bestDist = d;
                best = pA;
            }
        }
    }
    // ========== 5) 최종 타겟 설정 ==========
    if (best)
        m_pTarget = best;

}

CActor* CMinion::Find_MinDistance_Target()
{
    return m_pTarget;
}

CActor* CMinion::Check_FiledTarget()
{
    if (!m_pTarget)
        return nullptr;

    // 0) 엔진에서 이미 제거된 오브젝트인지부터 검사
    if (!Is_TargetStillExist(m_pTarget))
    {
        m_pTarget = nullptr;
        return nullptr;
    }

    // 1) 타겟이 미니언인 경우
    if (CMinion* pMin = dynamic_cast<CMinion*>(m_pTarget))
    {
        if (pMin->IsDeadOrDying())
        {
            m_pTarget = nullptr;
            return nullptr;
        }
    }
    else if (CBuilding* pBuild = dynamic_cast<CBuilding*>(m_pTarget))
    {
        // 1) 죽었으면 해제
        if (pBuild->Get_Dead())
        {
            m_pTarget = nullptr;
            return nullptr;
        }

        // 2) 아군 빌딩이면 타겟 자격 없음
        if (pBuild->Get_TeamType() == m_eTeam)
        {
            m_pTarget = nullptr;
            return nullptr;
        }
    }
    // 3) 그 외(챔피언 등)
    else
    {
        if (m_pTarget->Get_Dead())
        {
            m_pTarget = nullptr;
            return nullptr;
        }
    }

    // 4) 사거리 완전히 벗어나면 타겟 해제
    if (Target_Distance(m_pTarget) > m_tStats.m_fAggroRadius * 1.5f)
    {
        m_pTarget = nullptr;
        return nullptr;
    }

    return m_pTarget;
}

_bool CMinion::Is_TargetInAttackRange()
{
    // ★ 먼저 타겟 유효성 검사 + 포인터 받기
    CActor* pTarget = Check_FiledTarget();
    if (!pTarget)
        return false;

    float dist = Target_Distance(pTarget);
    return dist <= Get_DesiredAttackDist(pTarget) + 0.1f;
}

_bool CMinion::Is_TargetInAggroRange()
{
    CActor* pTarget = Check_FiledTarget();
    if (!pTarget)
        return false;

    return Target_Distance(pTarget) <= m_tStats.m_fAggroRadius;
}

_bool CMinion::Is_TargetStillExist(CActor* pTarget) const
{
    if (!pTarget)
        return false;

    CCollision_Manager* pColMgr = CCollision_Manager::GetInstance();
    if (!pColMgr)
        return false;

    // 1) 미니언 그룹
    auto& minionGroup =
        pColMgr->Get_CollisionGroup(CCollision_Manager::COLLISION_MINION);

    for (auto obj : minionGroup)
    {
        if (obj == pTarget)
            return true;
    }

    // 2) 빌딩(포탑/넥서스) 그룹
    auto& buildingGroup =
        pColMgr->Get_CollisionGroup(CCollision_Manager::COLLISION_BUILDING);

    for (auto obj : buildingGroup)
    {
        if (obj == pTarget)
            return true;
    }

    // 3) 플레이어(내 챔피언) 그룹
    auto& champGroup =
        pColMgr->Get_CollisionGroup(CCollision_Manager::COLLISION_PLAYER);

    for (auto obj : champGroup)
    {
        if (obj == pTarget)
            return true;
    }

    // 4) 적 챔피언(Vayne 등) 그룹 ?? 추가
    auto& enemyGroup =
        pColMgr->Get_CollisionGroup(CCollision_Manager::COLLISION_ENEMY);

    for (auto obj : enemyGroup)
    {
        if (obj == pTarget)
            return true;
    }

    return false;
}

void CMinion::Follow_Target(_float fTimeDelta)
{
    // 1) 타겟 유효성 검사
    CActor* pTarget = Check_FiledTarget();
    if (!pTarget || !m_bMove)
        return;

    // 2) 기본 방향(타겟 방향)
    _vector pos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    _vector tpos = pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

    _vector desired = XMVectorSetY(tpos - pos, 0.f);
    float   dist = XMVectorGetX(XMVector3Length(desired));
    if (dist < 0.001f)
        return;

    desired = XMVector3Normalize(desired);

    // 3) Separation(군집 회피)
    _vector sep = m_vAvoidDir;

    // 4) Lane offset (인덱스가 정상일 때만 사용)
    _vector laneOffsetDir = XMVectorZero();

    CMinionManager* pMgr = CMinionManager::GetInstance();
    if (pMgr)
    {
        const auto& path = pMgr->GetLanePath(m_eLane);
        if (!path.empty() &&
            m_iLaneNodeIndex >= 0 &&
            m_iLaneNodeIndex < (int)path.size() - 1)
        {
            _vector curNode = XMLoadFloat3(&path[m_iLaneNodeIndex]);
            _vector nextNode = XMLoadFloat3(
                &path[m_iLaneNodeIndex + pMgr->GetLaneStep(m_eTeam)]);

            _vector laneForward = XMVector3Normalize(nextNode - curNode);
            _vector laneRight = XMVector3Normalize(XMVector3Cross(
                XMVectorSet(0, 1, 0, 0), laneForward));

            laneOffsetDir = laneRight * (m_fLaneOffset * 0.22f);
        }
    }

    // 5) 최종 이동 방향 (타겟 + 회피 + 라인 오프셋)
    _vector finalDir = desired + sep + laneOffsetDir;
    if (XMVectorGetX(XMVector3Length(finalDir)) > 0.001f)
        finalDir = XMVector3Normalize(finalDir);

    m_vMoveDir = finalDir;

    float stopDist = Get_DesiredAttackDist(pTarget);

    // 6) 아직 공격 거리 바깥이면 전진
    if (dist > stopDist)
    {
        pos += m_vMoveDir * (m_tStats.m_fWalkSpeed * fTimeDelta);
        m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, pos);
    }

    // 7) 네비 + 바라보기
    Check_Navigation();
    Face_Target();
}

void CMinion::On_Death()
{
    // 이미 죽는 중이거나 완전히 죽은 상태면 다시 실행 X
    if (m_bDead || m_bDying || m_eState == STATE_DIE)
        return;

    m_bDying = true;
    m_eState = STATE_DIE;

    // 1) 충돌 그룹에서 제거 → 더 이상 CollisionwithGroup에서 안 잡힘
    CCollision_Manager::GetInstance()->Out_CollisionGroup(
        CCollision_Manager::COLLISION_MINION, this);

    // 2) 픽킹 그룹에서도 제거 → 마우스로도 안 찍힘
    CPickingManager::GetInstance()->Out_PickingGroup(this);

    // 3) 콜라이더 전부 삭제
    if (m_pSphereCom)
    {
        Safe_Release(m_pSphereCom);
        m_pSphereCom = nullptr;
    }
    if (m_pOBBCom)
    {
        Safe_Release(m_pOBBCom);
        m_pOBBCom = nullptr;
    }
    // 혹시 다른 콜라이더 더 있으면 여기서 전부 Safe_Release + nullptr

    // 이동/AI 중지
    m_bMove = false;
}

void CMinion::Apply_Separation(_float fTimeDelta)
{
    auto& group =
        CCollision_Manager::GetInstance()->Get_CollisionGroup(
            CCollision_Manager::COLLISION_MINION);

    _vector myPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    _vector myFlat = XMVectorSetY(myPos, 0);

    _vector avoid = XMVectorZero();
    int count = 0;

    const float avoidRadius = 1.6f;        // ★ 훨씬 넓어야 롤처럼 퍼짐
    const float avoidPower = 2.2f;        // ★ 회피 강도 UP

    for (auto obj : group)
    {
        if (!obj || obj == this) continue;

        CMinion* other = (CMinion*)obj;

        _vector oPos = other->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
        _vector oFlat = XMVectorSetY(oPos, 0);

        _vector diff = myFlat - oFlat;
        float dist = XMVectorGetX(XMVector3Length(diff));
        if (dist < 0.001f) continue;

        if (dist < avoidRadius)
        {
            float w = (avoidRadius - dist) / avoidRadius;
            avoid += XMVector3Normalize(diff) * w;
            count++;
        }
    }

    if (count > 0)
    {
        avoid /= (float)count;
        avoid *= avoidPower;

        // 스무딩
        m_vAvoidDir = XMVectorLerp(m_vLastAvoidDir, avoid, 0.25f);
        m_vLastAvoidDir = m_vAvoidDir;
    }
    else
    {
        m_vLastAvoidDir = XMVectorLerp(m_vLastAvoidDir, XMVectorZero(), 0.1f);
        m_vAvoidDir = m_vLastAvoidDir;
    }
}

// ======================= Combat / Damage =======================

_int CMinion::Take_Damage(int dmg, CActor* causer)
{
    // 이미 죽었거나 죽는 중이거나, HP가 0 이하이면 절대 맞지 않음
    if (m_bDead || m_bDying || m_eState == STATE_DIE || m_tStats.m_fCurrentHp <= 0.f)
        return 0;

    m_pOrginDamageCauser = causer;
    m_tStats.m_fCurrentHp -= dmg;

    if (m_tStats.m_fCurrentHp <= 0.f)
    {
        m_tStats.m_fCurrentHp = 0.f;

        // ?? 여기서 즉시 죽음 처리 실행
        On_Death();        // ← 이거 반드시 호출

        return 1;          // DeadState 전환용으로 그대로 리턴
    }

    // ===== 타겟 변경 로직 =====
    if (m_pTarget != nullptr)
        return 0;

    if (causer)
    {
        if (auto pBuild = dynamic_cast<CBuilding*>(causer))
        {
            if (pBuild->Get_TeamType() == m_eTeam)
                return 0;
        }

        m_pTarget = causer;
    }

    return 0;
}



void CMinion::ReUse_Setting(void* pArg)
{
    if (!pArg)
        return;

    MINION_SPAWNDESC* d = (MINION_SPAWNDESC*)pArg;

    m_eTeam = d->eTeam;
    m_eLane = d->eLane;
    m_eMinionType = d->eMinionType;
    m_iLaneIndex = d->iLaneIndex;
    m_iWaveIndex = d->iWaveIndex;
    m_vTargetPos = d->vTargetPos;

    Compute_LaneOffset();

    m_tStats.m_fCurrentHp = m_tStats.m_fMaxHp;
    m_eState = STATE_MOVE;
    m_bDead = false;
    m_pTarget = nullptr;
    m_pOrginDamageCauser = nullptr;
    m_bDying = false;
    m_fAttackTimer = 0.f;

    if (m_pTransformCom)
    {
        _vector p = XMLoadFloat3(&d->vSpawnPos);
        p = XMVectorSetW(p, 1.f);
        m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, p);
    }

    if (CMinionManager* mgr = CMinionManager::GetInstance())
    {
        const auto& path = mgr->GetLanePath(m_eLane);
        m_iLaneNodeIndex = mgr->GetLaneStartIndex(m_eTeam, path);
    }

    Compute_CurrentIndex();


    // ★ 항상 새 HpBar 생성 (기존 HpBar는 owner=null로 날려두고 스스로 죽게)
    m_pHpBar = nullptr;
    Create_HpBar();
    CCollision_Manager::GetInstance()->Add_CollisionGroup(
        CCollision_Manager::COLLISION_MINION, this);
}

_bool CMinion::Picking(_float3* PickingPoint)
{
    // ?? 1) 레드팀이 아니면 아예 타겟 불가
    if (m_eTeam != TEAM_RED)   // 또는 TEAMTYPE::TEAM_RED
        return false;

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

        // ?? 공격 커서 + 타겟 설정
        CCursorManager::GetInstance()->SetState(CURSOR_ATTACK);
        pPlayer->Set_AttackTarget(this);

        if (PickingPoint)
            *PickingPoint = vHitPos;

        return true;
    }

    ZeroMemory(&m_vMousePickedPos, sizeof(_float3));
    return false;
}

void CMinion::PickingTrue()
{
}

void CMinion::Set_Picked()
{

}

void CMinion::Mark_Dead()
{
    if (m_bDead)
        return;

    // ?? 충돌 그룹에서 제거
    CCollision_Manager::GetInstance()->Out_CollisionGroup(
        CCollision_Manager::COLLISION_MINION, this);

    if (m_pHpBar)
    {
        m_pHpBar->ReturnToPool();
        m_pHpBar = nullptr;
    }

    CPoolManager* pool = CPoolManager::GetInstance();
    if (pool)
    {
        const _tchar* tag = nullptr;

        if (m_eTeam == TEAM_BLUE)
            tag = (m_eMinionType == MINION_MELEE) ? TEXT("Layer_Minion_Blue_Melee") : TEXT("Layer_Minion_Blue_Range");
        else
            tag = (m_eMinionType == MINION_MELEE) ? TEXT("Layer_Minion_Red_Melee") : TEXT("Layer_Minion_Red_Range");

        if (tag)
            pool->Add_Pooling_Object(LEVEL_GAMEPLAY, tag, this);
    }

    m_bDead = true;
    m_bDying = false;
}

float CMinion::Get_DesiredAttackDist(CActor* pTarget) const
{
    if (!pTarget)
        return 0.f;

    float base = m_tStats.m_fAttackRange;
    float extra = fabsf(m_fLaneOffset) * 0.35f;   // 라인 오프셋 보정

    float padding = 0.f;

    // ★ 건물인 경우는 조금 더 멀리서 멈추게 (값은 취향대로 튜닝)
    if (dynamic_cast<CBuilding*>(pTarget))
        padding = 1.5f;     // 1.5 ~ 2.0 정도부터 시작해서 눈으로 맞춰봐

    return base + extra + padding;
}

void CMinion::Create_HpBar()
{
    if (m_pHpBar)
        return;
    if (nullptr == m_pGameInstance)
        return;

    CGameObject* pObj = nullptr;
    CPoolManager* pPool = CPoolManager::GetInstance();

    // 팀에 따라 PoolTag 결정
    const _tchar* szPoolTag = (m_eTeam == TEAM_BLUE)
        ? TEXT("Pool_UI_MinionHp_Blue")
        : TEXT("Pool_UI_MinionHp_Red");

    // 1) 팀 전용 풀에서 하나 꺼내본다
    if (pPool)
    {
        pObj = pPool->Pop_Pooling_Object(
            LEVEL_GAMEPLAY,
            szPoolTag);

        if (pObj)
        {
            // 새 주인으로 재세팅
            pObj->ReUse_Setting(this);

            // 렌더 레이어는 여전히 Layer_UI_MinionHp
            m_pGameInstance->ReAdd_GameObject(
                LEVEL_GAMEPLAY,
                TEXT("Layer_UI_MinionHp"),
                pObj);
        }
    }

    // 2) 해당 팀 풀에 아무것도 없으면 새로 생성
    if (!pObj)
    {
        if (FAILED(m_pGameInstance->Add_GameObject_Out(
            TEXT("Prototype_GameObject_UI_MinionHpBar"),
            LEVEL_GAMEPLAY,
            TEXT("Layer_UI_MinionHp"),
            pObj,
            this)))
        {
            return;
        }
    }

    m_pHpBar = dynamic_cast<CMinion_HpBar*>(pObj);

    if (m_pHpBar)
        m_pHpBar->Set_Owner(this);
}

void CMinion::Face_Target()
{
    // ★ 먼저 타겟 유효성 검사
    CActor* pTarget = Check_FiledTarget();
    if (!pTarget || !m_pTransformCom)
        return;

    CTransform* pMyTr = m_pTransformCom;
    CTransform* pTargetTr = pTarget->Get_Transform();
    if (!pTargetTr)
        return;

    _vector vMyPos = pMyTr->Get_State(CTransform::STATE_TRANSLATION);
    _vector vTargetPos = pTargetTr->Get_State(CTransform::STATE_TRANSLATION);

    _vector vLook = XMVectorSetY(vMyPos - vTargetPos, 0.f);

    if (XMVectorGetX(XMVector3LengthSq(vLook)) < 0.0001f)
        return;

    vLook = XMVector3Normalize(vLook);

    _vector vOldRight = pMyTr->Get_State(CTransform::STATE_RIGHT);
    float fScale = XMVectorGetX(XMVector3Length(vOldRight));
    if (fScale < 0.0001f)
        fScale = 1.f;

    _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    _vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vLook));

    vRight = vRight * fScale;
    vLook = vLook * fScale;
    vUp = XMVector3Normalize(XMVector3Cross(vLook, vRight)) * fScale;

    vRight = XMVectorSetW(vRight, 0.f);
    vUp = XMVectorSetW(vUp, 0.f);
    vLook = XMVectorSetW(vLook, 0.f);

    pMyTr->Set_State(CTransform::STATE_RIGHT, vRight);
    pMyTr->Set_State(CTransform::STATE_UP, vUp);
    pMyTr->Set_State(CTransform::STATE_LOOK, vLook);
}

_int CMinion::Find_ForwardLaneNodeIndex()
{
    CMinionManager* pMgr = CMinionManager::GetInstance();
    if (!pMgr || !m_pTransformCom)
        return m_iLaneNodeIndex;

    const auto& path = pMgr->GetLanePath(m_eLane);
    if (path.empty())
        return m_iLaneNodeIndex;

    _vector vMyPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    vMyPos = XMVectorSetY(vMyPos, 0.f);

    const int pathSize = (int)path.size();
    const int step = pMgr->GetLaneStep(m_eTeam); 

    float bestDistSq = FLT_MAX;
    int   bestIdx = m_iLaneNodeIndex;

    // ★ 항상 “진행 방향”으로만 검색
    if (step > 0)
    {
        for (int i = max(0, m_iLaneNodeIndex); i < pathSize; ++i)
        {
            _vector vNode = XMLoadFloat3(&path[i]);
            vNode = XMVectorSetY(vNode, 0.f);

            float distSq = XMVectorGetX(XMVector3LengthSq(vNode - vMyPos));
            if (distSq < bestDistSq)
            {
                bestDistSq = distSq;
                bestIdx = i;
            }
        }
    }
    else // step < 0  (RED 팀)
    {
        for (int i = min(m_iLaneNodeIndex, pathSize - 1); i >= 0; --i)
        {
            _vector vNode = XMLoadFloat3(&path[i]);
            vNode = XMVectorSetY(vNode, 0.f);

            float distSq = XMVectorGetX(XMVector3LengthSq(vNode - vMyPos));
            if (distSq < bestDistSq)
            {
                bestDistSq = distSq;
                bestIdx = i;
            }
        }
    }

    return bestIdx;
}

// ======================= Lane Offset =======================

void CMinion::Compute_LaneOffset()
{
    const float width = 1.2f;

    switch (m_iLaneIndex)
    {
    case 0: m_fLaneOffset = 0.f; break;
    case 1: m_fLaneOffset = -width; break;
    case 2: m_fLaneOffset = +width; break;
    default: m_fLaneOffset = 0.f; break;
    }

    if (m_eTeam == TEAM_RED)
        m_fLaneOffset = -m_fLaneOffset;
}

// ======================= Shader =======================

HRESULT CMinion::SetUp_ShaderResources()
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

HRESULT CMinion::SetUp_ShaderID()
{
    return S_OK;
}

_float CMinion::Check_CullingRadius()
{
    return 0.01f;
}


void CMinion::Free()
{
    __super::Free();
    CCollision_Manager::GetInstance()->Out_CollisionGroup(CCollision_Manager::COLLISION_MINION, this);
}
