#include "Minion_AttackState.h"
#include "GameInstance.h"
#include "Minion_Melee.h"
#include "Minion_Range.h"
#include "Minion_ChaseState.h"
#include "Minion_MoveState.h"
USING(Client);
using namespace Minion;

CAttackState::CAttackState(class CMinion* pMinion)
{
    m_pOwner = pMinion;
}


CMinionState* CAttackState::AI_Behaviour(_float fTimeDelta)
{
    return nullptr;
}


CMinionState* CAttackState::Update(_float fTimeDelta)
{
    if (!m_pOwner)
        return nullptr;

    m_fAttackElapsed += fTimeDelta;

    // 1) 타겟 유효성
    CActor* pTarget = m_pOwner->Check_FiledTarget();
    if (!pTarget)
    {
        // 새 타겟 재탐색
        m_pOwner->Find_Target();
        pTarget = m_pOwner->Get_Target();

        if (!pTarget)
            return new CMoveState(m_pOwner); // 아무도 없으면 다시 라인 이동
    }

    // 2) 너무 멀어지면 다시 추격
    if (!m_pOwner->Is_TargetInAttackRange())
        return new CChaseState(m_pOwner);

    // 3) 공격 애니 재생
    m_bIsAnimationFinished =
        m_pOwner->Get_Model()->Play_Animation(
            fTimeDelta,
            false,
            nullptr,
            0.1f);

    if (m_bIsAnimationFinished &&
        m_pOwner->Get_AttackTimer() >= m_pOwner->Get_AttackCoolTime())
    {
        m_pOwner->Attack_Target(fTimeDelta);
        m_pOwner->Reset_AttackTimer();

        // ★ 콤보 인덱스 증가
        _int iIdx = m_pOwner->Get_AttackIndex();
        m_pOwner->Set_AttackIndex(iIdx + 1);

        return new CAttackState(m_pOwner);
    }

    return nullptr;
}

CMinionState* CAttackState::Late_Update(_float fTimeDelta)
{
    return nullptr;
}

void CAttackState::Enter()
{
    m_eState = STATE_ATTACK;
    m_fAttackElapsed = 0.f;

    if (!m_pOwner || !m_pOwner->Get_Model())
        return;

    MINION_TYPE eType = m_pOwner->Get_MinType();

    _uint iAnim = 0;
    _int  iIdx = m_pOwner->Get_AttackIndex();

    if (eType == MINION_MELEE)
    {
        switch (iIdx % 3)
        {
        case 0: iAnim = CMinion_Melee::ANIM::ATTACK1;   break;
        case 1: iAnim = CMinion_Melee::ANIM::ATTACK2;   break;
        case 2: iAnim = CMinion_Melee::ANIM::ATTACK3_B; break;
        }
    }
    else // MINION_CASTER
    {
        switch (iIdx % 2)
        {
        case 0: iAnim = CMinion_Range::ANIM::ATTACK1; break;
        case 1: iAnim = CMinion_Range::ANIM::ATTACK2; break;
        }
    }

    m_pOwner->Get_Model()->Set_CurrentAnimIndex(iAnim);
}

void CAttackState::Exit()
{
}