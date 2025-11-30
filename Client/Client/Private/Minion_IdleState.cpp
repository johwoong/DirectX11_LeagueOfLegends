#include "Minion_IdleState.h"
#include "GameInstance.h"
#include "Minion_MoveState.h" 

USING(Client);
using namespace Minion;

CIdleState::CIdleState(class CMinion* pMinion)
{
    m_pOwner = pMinion;
    m_fIdleTime = 0.f;
}


CMinionState* CIdleState::AI_Behaviour(_float fTimeDelta)
{
    return nullptr;
}


CMinionState* CIdleState::Update(_float fTimeDelta)
{
    if (!m_pOwner)
        return nullptr;

    m_bIsAnimationFinished =
        m_pOwner->Get_Model()->Play_Animation(
            fTimeDelta,
            m_pOwner->Is_AnimationLoop(m_pOwner->Get_Model()->Get_CurrentAnimIndex()),
            nullptr,
            0.1f);

    m_fIdleTime += fTimeDelta;

    if (m_fIdleTime >= 0.3f)
    {
        return new CMoveState(m_pOwner);
    }

    return nullptr;
}

CMinionState* CIdleState::Late_Update(_float fTimeDelta)
{
    return nullptr;
}

void CIdleState::Enter()
{
    m_eState = STATE_IDLE;
    m_fIdleTime = 0.f;

    if (m_pOwner && m_pOwner->Get_Model())
    {
        m_pOwner->Get_Model()->Set_CurrentAnimIndex(m_pOwner->GetIdleAnimIndex());
    }
}

void CIdleState::Exit()
{
}
