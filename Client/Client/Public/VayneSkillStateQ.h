#pragma once
#include "VayneState.h"

NS_BEGIN(Client)
NS_BEGIN(Vayne)

class CVayneSkillStateQ : public CVayneState
{
public:
    CVayneSkillStateQ(class CVayne* pVayne);

public:
    virtual CVayneState* AI_Behaviour(_float fTimeDelta) override;
    virtual CVayneState* Update(_float fTimeDelta) override;
    virtual CVayneState* LateUpdate(_float fTimeDelta) override;

    virtual void Enter() override;
    virtual void Exit()  override;

private:
    _bool m_bProjectileSpawned = false;
    _bool m_bIsAnimFinished = false;
    private:
    vector<CEffect*> m_QFxList;   // ?? 이게 정답
    bool m_bQFxSpawned = false;
private:
    float m_fQMinTime = 0.f;
};

NS_END
NS_END