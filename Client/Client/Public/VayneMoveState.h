#pragma once

#include "VayneState.h"

NS_BEGIN(Client)
NS_BEGIN(Vayne)
class CMoveState final  :public CVayneState
{
public:
    CMoveState(class CVayne* pVayne);

    virtual CVayneState* AI_Behaviour(_float fTimeDelta) override;
    virtual CVayneState* Update(_float fTimeDelta) override;
    virtual CVayneState* LateUpdate(_float fTimeDelta) override;

    virtual void Enter() override;
    virtual void Exit() override;

private:
    _float m_fMoveTimeAcc = 0.f;
    _float m_fMoveTime = 0.f;
    _float3 m_vLastFollowPos = { 0.f, 0.f, 0.f };
    bool    m_bHasFollowPos = false;
};
NS_END
NS_END

