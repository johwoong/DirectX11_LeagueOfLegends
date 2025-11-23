#include "Minion_DeadState.h"
#include "GameInstance.h"
#include "Minion_MoveState.h" 
#include "Minion_Melee.h"
#include "Minion_Range.h"

USING(Client);
using namespace Minion;

CDeadState::CDeadState(class CMinion* pMinion)
{
    m_pOwner = pMinion;
}


CMinionState* CDeadState::AI_Behaviour(_float fTimeDelta)
{
    return nullptr;
}


CMinionState* CDeadState::Update(_float fTimeDelta)
{
    if (!m_pOwner)
        return nullptr;

    // ★ 죽음 애니메이션은 무조건 1번만 재생
    m_bIsAnimationFinished =
        m_pOwner->Get_Model()->Play_Animation(
            fTimeDelta * 2.f,
            false,      // ← loop 끄기
            nullptr,
            0.1f);

    // ★ 애니 끝나면 실제로 삭제 플래그 세팅
    if (m_bIsAnimationFinished)
    {
        m_pOwner->Mark_Dead();   // CMinion::Update 맨 위에서 OBJ_DEAD 리턴
    }

    return nullptr;
}

CMinionState* CDeadState::Late_Update(_float fTimeDelta)
{
    return nullptr;
}

void CDeadState::Enter()
{
    m_eState = STATE_DEAD;

    if (!m_pOwner || !m_pOwner->Get_Model())
        return;

    MINION_TYPE type = m_pOwner->Get_MinType();
    _uint iAnim = 0;

    if (type == MINION_MELEE)
    {
        // 근접 미니언 죽는 애니
        iAnim = CMinion_Melee::ANIM::DEATH;  // 실제 enum에 맞게 골라
    }
    else
    {
        // 원거리 미니언 죽는 애니
        iAnim = CMinion_Range::ANIM::DEATH1;  // 원하면 DEATH2/3 써도 됨
    }

    m_pOwner->Get_Model()->Set_CurrentAnimIndex(iAnim);
}

void CDeadState::Exit()
{
}
