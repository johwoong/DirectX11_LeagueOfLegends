
#include "Minion_State.h"
#include "Minion.h"

USING(Client);

CMinion_State::CMinion_State(CMinion* pOwner, STATE_ID eStateId)
    : m_pOwner(pOwner)
    , m_eStateId(eStateId)
{
}

CMinion_State* CMinion_State::ChangeState(CMinion_State* pCurrentState, CMinion_State* pNewState)
{
    if (pCurrentState && pCurrentState != pNewState)
        Safe_Delete(pCurrentState);

    return pNewState;
}