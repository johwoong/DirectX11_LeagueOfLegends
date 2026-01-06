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
    virtual void Exit() override;

private:
    bool m_bProjectileSpawned = false;   // E 투사체 발사 여부
    bool m_bAnimFinished = false;        // 애니메이션 종료 여부
};

NS_END
NS_END