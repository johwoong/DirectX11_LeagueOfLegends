#pragma once
#include "Minion_State.h"

NS_BEGIN(Client)
NS_BEGIN(Minion)
class CDeadState :
    public CMinionState
{
public:
    CDeadState(class CMinion* pMinion);

public:
    virtual CMinionState* AI_Behaviour(_float fTimeDelta) override;
    virtual CMinionState* Update(_float fTimeDelta)       override;
    virtual CMinionState* Late_Update(_float fTimeDelta)   override;

    virtual void Enter() override;
    virtual void Exit()  override;

private:
    _int   m_iAttackIndex = 0;   // 0,1,2 반복 (근거리 3타 기준)
    _float m_fAttackElapsed = 0.f;
};
NS_END
NS_END
