#include "VayneIdleState.h"
#include "VayneMoveState.h"
#include "VayneAttackState.h"

using namespace Vayne;

CIdleState::CIdleState(CVayne* pVayne)
{
    m_pOwner = pVayne;
    m_fTimeDeltaAcc = 0.f;
    m_fIdleTime = 0.f;   // 실제 값은 Enter()에서 랜덤 설정
}

CVayneState* CIdleState::AI_Behaviour(_float fTimeDelta)
{
    if (!m_pOwner)
        return nullptr;

    CTransform* pTr = m_pOwner->Get_Transform();
    CNavigation* pNavi = m_pOwner->Get_Navigation();
    if (!pTr || !pNavi)
        return nullptr;

    // =========================
    // 0) 먼저 타겟부터 확인
    // =========================
    CActor* pTarget = m_pOwner->Check_FiledTarget();
    if (!pTarget)
    {
        m_pOwner->Find_Target();
        pTarget = m_pOwner->Get_Target();
    }

    if (pTarget)
    {
        float dist = m_pOwner->Target_Distance(pTarget);
        float range = m_pOwner->Get_AttackRangeWorld();

        // 사거리 안 → 바로 공격 상태
        if (dist <= range * 1.1f)
            return new CAttackState(m_pOwner);
        else
            return new CMoveState(m_pOwner); // 쫓으러 감
    }

    // =========================
    // 1) 첫 포탑(8번째 노드) 도착 전 → 그쪽으로 이동시키기
    // =========================
    if (!m_pOwner->HasReachedFirstTower())
    {
        _float3 vWaitPos = m_pOwner->Get_FirstTowerWaitPos();

        _vector vMy = pTr->Get_State(CTransform::STATE_TRANSLATION);
        _vector vDst = XMLoadFloat3(&vWaitPos);

        vMy = XMVectorSetY(vMy, 0.f);
        vDst = XMVectorSetY(vDst, 0.f);

        float fDist = XMVectorGetX(XMVector3Length(vDst - vMy));

        if (fDist > 2.f)
            return new CMoveState(m_pOwner);

        m_pOwner->SetReachedFirstTower(true);
        return nullptr;
    }

    // =========================
    // 2) 이미 첫 포탑 밑에 있음 → 아군 웨이브 올 때까지 대기
    // =========================
    if (!m_pOwner->Has_AllyWaveNearby(25.f))
    {
        m_fTimeDeltaAcc = 0.f;
        return nullptr;
    }

    // =========================
    // 3) 웨이브 도착 → 랜덤 딜레이 후 Move
    // =========================
    m_fTimeDeltaAcc += fTimeDelta;
    if (m_fTimeDeltaAcc >= m_fIdleTime)
    {
        return new CMoveState(m_pOwner);
    }

    return nullptr;
}

CVayneState* CIdleState::Update(_float fTimeDelta)
{
    m_bIsAnimationFinished =
        m_pOwner->Get_Model()->Play_Animation(
            fTimeDelta,
            m_pOwner->Is_AnimationLoop(
                m_pOwner->Get_Model()->Get_CurrentAnimIndex())
        );

    return nullptr;
}

CVayneState* CIdleState::LateUpdate(_float fTimeDelta)
{
    m_pOwner->Check_Navigation();
    return nullptr;
}

void CIdleState::Enter()
{
    m_eStateId = STATE_ID::STATE_IDLE;

    static const CVayne::ANIM sIdleAnims[] =
    {
        CVayne::ANIM::IDLE,
        CVayne::ANIM::IDLE1,
        CVayne::ANIM::IDLE2,
        CVayne::ANIM::IDLE3,
        CVayne::ANIM::IDLE4
    };

    int idx = rand() % (_countof(sIdleAnims));
    m_pOwner->Get_Model()->Set_CurrentAnimIndex(sIdleAnims[idx]);

    // 1 ~ 4초 사이 랜덤 대기 (웨이브랑 간격 살짝 주기)
    m_fIdleTime = 1.f + (rand() % 3001) / 1000.f; // 1.000 ~ 4.000

    m_fTimeDeltaAcc = 0.f;
}

void CIdleState::Exit()
{
}