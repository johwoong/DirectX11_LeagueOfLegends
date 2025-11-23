#include "Minion_ChaseState.h"
#include "Minion_AttackState.h"
#include "GameInstance.h"
#include "Minion_MoveState.h"

USING(Client);
using namespace Minion;

CChaseState::CChaseState(class CMinion* pMinion)
{
    m_pOwner = pMinion;
    m_fIdleTime = 0.f;
}


CMinionState* CChaseState::AI_Behaviour(_float fTimeDelta)
{
    if (!m_pOwner) return nullptr;

    // 1) 현재 타겟 상태 확인
    CActor* pTarget = m_pOwner->Check_FiledTarget();

    // 타겟 잃었으면 바로 다시 찾기 시도
    if (!pTarget)
    {
        m_pOwner->Find_Target();
        pTarget = m_pOwner->Get_Target();

        // 그래도 없으면 라인 따라가는 MoveState 로
        if (!pTarget)
            return new CMoveState(m_pOwner);
    }

    // 2) 공격 사거리면 AttackState
    if (m_pOwner->Is_TargetInAttackRange())
        return new CAttackState(m_pOwner);

    return nullptr;
}


CMinionState* CChaseState::Update(_float fTimeDelta)
{
    if (!m_pOwner)
        return nullptr;

    m_bIsAnimationFinished =
        m_pOwner->Get_Model()->Play_Animation(
            fTimeDelta,
            m_pOwner->Is_AnimationLoop(m_pOwner->Get_Model()->Get_CurrentAnimIndex()),
            nullptr,
            0.1f);

    // 1) 타겟 체크 + 재탐색
    CActor* pTarget = m_pOwner->Check_FiledTarget();
    if (!pTarget)
    {
        m_pOwner->Find_Target();
        pTarget = m_pOwner->Get_Target();

        if (!pTarget)
            return new CMoveState(m_pOwner);
    }

    // 2) 사거리 안이면 Attack
    if (m_pOwner->Is_TargetInAttackRange())
        return new CAttackState(m_pOwner);

    // 3) 아직 멀면 따라가기
    m_pOwner->Follow_Target(fTimeDelta);

    return nullptr;
}

CMinionState* CChaseState::Late_Update(_float fTimeDelta)
{
    return nullptr;
}

void CChaseState::Enter()
{
    m_eState = STATE_CHASE;
    m_fIdleTime = 0.f;

    if (m_pOwner && m_pOwner->Get_Model())
    {
        // Chase 는 그냥 Run 애니 사용
        m_pOwner->Get_Model()->Set_CurrentAnimIndex(m_pOwner->GetRunAnimIndex());
    }
}

void CChaseState::Exit()
{
}