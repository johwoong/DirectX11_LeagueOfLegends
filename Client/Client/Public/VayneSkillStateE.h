#pragma once
#include "VayneState.h"

NS_BEGIN(Client)
NS_BEGIN(Vayne)

class CVayneSkillStateE : public CVayneState
{
public:
    CVayneSkillStateE(class CVayne* pVayne);

public:
    virtual CVayneState* AI_Behaviour(_float fTimeDelta) override;
    virtual CVayneState* Update(_float fTimeDelta) override;
    virtual CVayneState* LateUpdate(_float fTimeDelta) override;

    virtual void Enter() override;
    virtual void Exit()  override;

private:
    _bool m_bProjectileSpawned = false;
    _bool m_bIsAnimFinished = false;
};

NS_END
NS_END