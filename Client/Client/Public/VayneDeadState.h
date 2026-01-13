#pragma once
#include "VayneState.h"

NS_BEGIN(Client)
NS_BEGIN(Vayne)

class CDeadState : public CVayneState
{
public:
    CDeadState(class CVayne* pVayne);

public:
    virtual CVayneState* AI_Behaviour(_float fTimeDelta) override;
    virtual CVayneState* Update(_float fTimeDelta) override;
    virtual CVayneState* LateUpdate(_float fTimeDelta) override;

    virtual void Enter() override;
    virtual void Exit()  override;

};

NS_END
NS_END