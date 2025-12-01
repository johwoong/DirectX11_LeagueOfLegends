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

    CActor* pTarget = m_pOwner->Check_FiledTarget();

    if (!pTarget)
    {
        m_pOwner->Find_Target(fTimeDelta);
        pTarget = m_pOwner->Get_Target();

        if (!pTarget)
            return new CMoveState(m_pOwner);
    }

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

    CActor* pTarget = m_pOwner->Check_FiledTarget();
    if (!pTarget)
    {
        m_pOwner->Find_Target(fTimeDelta);
        pTarget = m_pOwner->Get_Target();

        if (!pTarget)
            return new CMoveState(m_pOwner);
    }

    if (m_pOwner->Is_TargetInAttackRange())
        return new CAttackState(m_pOwner);

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
        m_pOwner->Get_Model()->Set_CurrentAnimIndex(m_pOwner->GetRunAnimIndex());
    }
}

void CChaseState::Exit()
{
}