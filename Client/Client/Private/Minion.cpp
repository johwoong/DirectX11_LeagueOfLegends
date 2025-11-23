#include "Minion.h"
#include "MinionManager.h"
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

// ======================= 기본 Initialize =======================

HRESULT CMinion::Initialize_Prototype()
{
    // 공통으로 쓸 게 있으면 여기서
    return __super::Initialize_Prototype();
}

HRESULT CMinion::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    // ★ 먼저 pArg에서 팀/라인/타입 세팅
    if (pArg)
    {
        MINION_SPAWNDESC* pDesc = reinterpret_cast<MINION_SPAWNDESC*>(pArg);

        m_eTeam = pDesc->eTeam;
        m_eLane = pDesc->eLane;
        m_eMinionType = pDesc->eMinionType;
        m_iLaneIndex = pDesc->iLaneIndex;
        m_iWaveIndex = pDesc->iWaveIndex;
        m_vTargetPos = pDesc->vTargetPos;   // ★ 추가
    }

    // ★ 슬롯 인덱스 기반으로 퍼질 오프셋 계산
    Compute_LaneOffset();

    // 팀/라인 세팅 끝난 후
    if (CMinionManager* pMgr = CMinionManager::GetInstance())
    {
        const auto& path = pMgr->GetLanePath(m_eLane);
        m_iLaneNodeIndex = pMgr->GetLaneStartIndex(m_eTeam, path);  
    }

    // ★ 그 다음에 Ready_Components 호출 (이제 m_eTeam 값이 올바르게 들어가 있음)
    if (FAILED(Ready_Components(pArg)))
        return E_FAIL;

    CNavigation::NAVDESC NaviDesc;
    ZeroMemory(&NaviDesc, sizeof NaviDesc);
    if (FAILED(__super::Add_Components(TEXT("Com_Navigation"), LEVEL_STATIC, TEXT("Prototype_Component_ChampionNavigation"), (CComponent**)&m_pNavigationCom, &NaviDesc)))
        return E_FAIL;

    // 기본 스탯 초기화
    m_tStats.m_fCurrentHp = m_tStats.m_fMaxHp;

    if (m_tStats.m_fAttackSpeed > 0.f)
        m_fAttackCoolTime = 1.f / m_tStats.m_fAttackSpeed;
    else
        m_fAttackCoolTime = 1.f;

    m_fAttackTimer = 0.f;
    m_eState = STATE_MOVE;

    Compute_CurrentIndex();
    return S_OK;
}

// ======================= Update / Late_Update =======================

int CMinion::Update(_float fTimeDelta)
{
    if (m_bDead)
        return OBJ_POOL;

    if (FAILED(__super::Update(fTimeDelta)))
        return -1;

    m_fAttackTimer += fTimeDelta;
    Compute_CurrentIndex();

    if (m_tStats.m_fCurrentHp <= 0.f && m_eState != STATE_DIE)
    {
        m_tStats.m_fCurrentHp = 0.f;
        m_eState = STATE_DIE;
        On_Death();
    }

    return 0;
}

void CMinion::Late_Update(_float fTimeDelta)
{
    __super::Late_Update(fTimeDelta);
    if (Is_Frustum(Check_CullingRadius()) == false)
    {
        return;
    }
    if (m_pRendererCom != nullptr)
    {
        m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_NOALPHABLEND, this);
    }
}

// ======================= Render 계열 =======================

HRESULT CMinion::Render()
{
    if (nullptr == m_pShaderCom || m_pModelCom == nullptr)
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
}

HRESULT CMinion::Render_ShadowDepth()
{
    // 그림자용 쉐이더 세팅이 따로 있다면 여기서
    return __super::Render_ShadowDepth();
}

HRESULT CMinion::Render_EdgeDetection()
{
    // 외곽선 / 실루엣 렌더용
    return __super::Render_EdgeDetection();
}

void CMinion::Compute_CurrentIndex(LEVEL eLevel)
{
    // 필요하면 레벨별로 다른 NaviDesc 관리
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

    _vector vMyPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    _vector vTargetPos = pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

    _vector vA = XMVectorSetY(vMyPos, 0.f);
    _vector vB = XMVectorSetY(vTargetPos, 0.f);

    return XMVectorGetX(XMVector3Length(vB - vA));
}


void CMinion::Follow_Target(_float fTimeDelta)
{
    if (!m_pTarget || !m_pTransformCom || !m_bMove)
        return;

    _vector vMyPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    _vector vTargetPos = m_pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

    // 타겟 기준 "타겟 → 나" 방향 (XZ 평면)
    _vector vDirFromTarget = XMVectorSetY(vMyPos - vTargetPos, 0.f);
    if (XMVectorGetX(XMVector3LengthSq(vDirFromTarget)) < 1e-6f)
    {
        // 완전히 겹쳐 있으면 임의 방향
        vDirFromTarget = XMVectorSet(0.f, 0.f, 1.f, 0.f);
    }
    else
    {
        vDirFromTarget = XMVector3Normalize(vDirFromTarget);
    }

    // 1) 타겟에서 공격 사거리만큼 떨어진 원 위의 기본 위치
    _vector vDesiredPos = vTargetPos + vDirFromTarget * m_tStats.m_fAttackRange;

    // 2) 타겟 기준 오른쪽 벡터
    _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    _vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vDirFromTarget));

    // 3) 내 슬롯 오프셋만큼 좌우로 퍼지게
    vDesiredPos += vRight * m_fLaneOffset;

    // 4) 현재 위치에서 그 자리까지 이동
    _vector vMoveDir = XMVectorSetY(vDesiredPos - vMyPos, 0.f);
    float lenSq = XMVectorGetX(XMVector3LengthSq(vMoveDir));
    if (lenSq < 1e-6f)
        return;

    float fDist = sqrtf(lenSq);
    float fMaxMove = m_tStats.m_fWalkSpeed * fTimeDelta;

    // 너무 가까우면 더 이상 안 움직이게
    if (fDist < 0.05f)   // 0.05f → 0.03f 정도로 줄이거나, 그대로 둬도 상관없음
        return;

    if (fMaxMove > fDist)
        fMaxMove = fDist;

    vMoveDir = XMVector3Normalize(vMoveDir);
    vMyPos += vMoveDir * fMaxMove;

    m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vMyPos);

    // 네비게이션 높이 보정
    Check_Navigation();
}

_float CMinion::Check_CullingRadius()
{
    return 100.f;
}

_bool CMinion::Is_TargetInAttackRange() const
{
    if (!m_pTarget)
        return false;

    _float dist = Target_Distance(m_pTarget);

    // LaneOffset 만큼 옆으로 빠져 있으니까 마진을 조금 줌
    _float fEffectiveRange = m_tStats.m_fAttackRange + fabsf(m_fLaneOffset) * 0.6f;

    return dist <= fEffectiveRange;
}

_bool CMinion::Is_TargetInAggroRange() const
{
    if (!m_pTarget)
        return false;

    return Target_Distance(m_pTarget) <= m_tStats.m_fAggroRadius;
}

void CMinion::Find_Target()
{
    // 이미 타겟 있으면 그대로 둠 (롤처럼 한 번 잡으면 유지)
    if (m_pTarget)
        return;

    CGameInstance* pGI = CGameInstance::GetInstance();
    Safe_AddRef(pGI);

    list<CGameObject*>* pEnemyMeleeList = nullptr;
    list<CGameObject*>* pEnemyRangeList = nullptr;

    if (m_eTeam == TEAM_BLUE)
    {
        pEnemyMeleeList = pGI->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Minion_Red_Melee"));
        pEnemyRangeList = pGI->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Minion_Red_Range"));
    }
    else
    {
        pEnemyMeleeList = pGI->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Minion_Blue_Melee"));
        pEnemyRangeList = pGI->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Minion_Blue_Range"));
    }

    CActor* pBest = nullptr;
    _float  fBestDist = m_tStats.m_fAggroRadius;   // 시야 안에서만

    const _int mySlot = m_iLaneIndex;

    auto ScanList = [&](list<CGameObject*>* pList)
        {
            if (!pList) return;

            for (CGameObject* pObj : *pList)
            {
                CMinion* pEnemy = dynamic_cast<CMinion*>(pObj);
                if (!pEnemy || pEnemy->Get_Dead())
                    continue;

                // 1) 같은 슬롯 우선
                if (pEnemy->Get_LaneIndex() != mySlot)
                    continue;

                _float dist = Target_Distance(pEnemy);
                if (dist < fBestDist)
                {
                    fBestDist = dist;
                    pBest = pEnemy;
                }
            }
        };

    // 1) 같은 슬롯의 적부터 탐색
    ScanList(pEnemyMeleeList);
    ScanList(pEnemyRangeList);

    // 2) 같은 슬롯에서 못 찾았으면 그냥 가장 가까운 놈으로 fallback
    if (!pBest)
    {
        auto ScanAny = [&](list<CGameObject*>* pList)
            {
                if (!pList) return;
                for (CGameObject* pObj : *pList)
                {
                    CMinion* pEnemy = dynamic_cast<CMinion*>(pObj);
                    if (!pEnemy || pEnemy->Get_Dead())
                        continue;

                    _float dist = Target_Distance(pEnemy);
                    if (dist < fBestDist)
                    {
                        fBestDist = dist;
                        pBest = pEnemy;
                    }
                }
            };

        ScanAny(pEnemyMeleeList);
        ScanAny(pEnemyRangeList);
    }

    m_pTarget = pBest;

    Safe_Release(pGI);
}

CActor* CMinion::Find_MinDistance_Target()
{
    return m_pTarget;
}

CActor* CMinion::Check_FiledTarget()
{
    if (!m_pTarget)
        return nullptr;

    // ★ 타겟이 미니언이면 HP/상태로 먼저 체크
    if (CMinion* pMinTarget = dynamic_cast<CMinion*>(m_pTarget))
    {
        if (pMinTarget->m_tStats.m_fCurrentHp <= 0.f ||
            pMinTarget->m_eState == STATE_DIE)
        {
            m_pTarget = nullptr;
            return nullptr;
        }
    }
    else
    {
        // 다른 타입(나중에 챔프/타워 등) 은 기존대로
        if (m_pTarget->Get_Dead())
        {
            m_pTarget = nullptr;
            return nullptr;
        }
    }

    _float dist = Target_Distance(m_pTarget);

    // 너무 멀어지면 타겟 해제(한 번 잡았던 것도 풀림)
    if (dist > m_tStats.m_fAggroRadius * 1.5f)
    {
        m_pTarget = nullptr;
        return nullptr;
    }

    return m_pTarget;
}
void CMinion::Compute_LaneOffset()
{
    const _float fSlotWidth = 1.2f; // 좌우로 퍼질 거리(원하면 나중에 조절)

    switch (m_iLaneIndex)
    {
    case 0: // 가운데
        m_fLaneOffset = 0.f;
        break;
    case 1: // 왼쪽
        m_fLaneOffset = -fSlotWidth;
        break;
    case 2: // 오른쪽
        m_fLaneOffset = +fSlotWidth;
        break;
    default:
        m_fLaneOffset = 0.f;
        break;
    }

    // 필요하면 팀에 따라 좌우 반전 (맵 방향에 따라 조절)
    if (m_eTeam == TEAM_RED)
        m_fLaneOffset = -m_fLaneOffset;
}



_int CMinion::Take_Damage(int iDamage, CActor* DamageCauser)
{
    if (m_eState == STATE_DIE)
        return 0;

    m_pOrginDamageCauser = DamageCauser;

    m_tStats.m_fCurrentHp -= (_float)iDamage;
    if (m_tStats.m_fCurrentHp <= 0.f)
    {
        m_tStats.m_fCurrentHp = 0.f;
        m_eState = STATE_DIE;

        // 공격한 놈에게 어그로 (이미 죽을 거면 의미 없지만)
        m_pTarget = DamageCauser;

        On_Death();

        return 1;
    }

    // 맞으면 그 놈에게 어그로
    if (DamageCauser)
        m_pTarget = DamageCauser;

    return 0;
}

void CMinion::ReUse_Setting(void* pArg)
{
    if (pArg == nullptr)
        return;

    MINION_SPAWNDESC* pDesc = reinterpret_cast<MINION_SPAWNDESC*>(pArg);

    // 1) 라인/팀/타입 정보 세팅
    m_eTeam = pDesc->eTeam;
    m_eLane = pDesc->eLane;
    m_eMinionType = pDesc->eMinionType;
    m_iLaneIndex = pDesc->iLaneIndex;
    m_iWaveIndex = pDesc->iWaveIndex;
    m_vTargetPos = pDesc->vTargetPos;   // ★ 추가
    // ★ 슬롯 인덱스 기반으로 퍼질 오프셋 계산
    Compute_LaneOffset();
    // 2) 체력 / 상태 초기화
    m_tStats.m_fCurrentHp = m_tStats.m_fMaxHp;
    m_eState = STATE_MOVE;     // 스폰되면 바로 이동 상태
    m_bDead = false;
    m_pTarget = nullptr;
    m_pOrginDamageCauser = nullptr;

    m_fAttackTimer = 0.f;

    // 3) 위치 세팅
    if (m_pTransformCom)
    {
        _vector vPos = XMLoadFloat3(&pDesc->vSpawnPos);
        vPos = XMVectorSetW(vPos, 1.f);
        m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vPos);
    }

    // ★ 여기서 웨이포인트 시작 인덱스 세팅
    CMinionManager* pMgr = CMinionManager::GetInstance();
    if (pMgr)
    {
        const auto& path = pMgr->GetLanePath(m_eLane);
        m_iLaneNodeIndex = pMgr->GetLaneStartIndex(m_eTeam, path); // BLUE:0, RED: path.size()-1
    }

    // 4) 내비 인덱스 다시 계산
    Compute_CurrentIndex();
}



HRESULT CMinion::SetUp_ShaderResources()
{
    if (!m_pShaderCom || !m_pTransformCom)
        return E_FAIL;

    _float4x4 WorldMatrix = m_pTransformCom->Get_World_TP();
    _float4x4 ViewMatrix = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_VIEW);
    _float4x4 ProjMatrix = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_PROJ);


    if (FAILED(m_pShaderCom->Set_RawValue("g_WorldMatrix", &WorldMatrix, sizeof(_float4x4))))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Set_RawValue("g_ViewMatrix", &ViewMatrix, sizeof(_float4x4))))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Set_RawValue("g_ProjMatrix", &ProjMatrix, sizeof(_float4x4))))
        return E_FAIL;

    return S_OK;
}

HRESULT CMinion::SetUp_ShaderID()
{
    return S_OK;
}

void CMinion::Free()
{
    __super::Free();
}