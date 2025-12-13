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
    CModel* pModel = m_pOwner->Get_Model();
    _bool bEnd = pModel->Play_Animation(
        fTimeDelta,
        false);   // 죽는 애니는 루프 X

    if (bEnd)
    {
        m_pOwner->Mark_Dead();   // 여기서는 풀로 돌려 보내기만
        return nullptr;
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
        iAnim = CMinion_Melee::ANIM::DEATH;
    else
        iAnim = CMinion_Range::ANIM::DEATH1;

    m_pOwner->Get_Model()->Set_CurrentAnimIndex(iAnim);
}

void CDeadState::Exit()
{
}
