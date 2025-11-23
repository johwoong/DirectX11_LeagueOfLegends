#pragma once
#include "Minion_State.h"


NS_BEGIN(Client)
NS_BEGIN(Minion)

class CMoveState final : public CMinionState
{
public:
    explicit CMoveState(CMinion* pMinion);

public:
    virtual CMinionState* AI_Behaviour(_float fTimeDelta) override;
    virtual CMinionState* Update(_float fTimeDelta)       override;
    virtual CMinionState* Late_Update(_float fTimeDelta)   override;

    virtual void Enter() override;
    virtual void Exit()  override;

private:
    _float  m_fAccTime = 0.f;
};

NS_END
NS_END