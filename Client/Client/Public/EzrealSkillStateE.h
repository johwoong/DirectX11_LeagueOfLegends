#pragma once
#include "PlayerState.h"
class CEzrealSkillStateE : public CPlayerState
{
public:
    CEzrealSkillStateE(CPlayer* pPlayer);

    virtual CPlayerState* HandleInput() override;
    virtual CPlayerState* Update(_float fTimeDelta) override;
    virtual CPlayerState* Late_Update(_float fTimeDelta) override;

    virtual void Enter() override;
    virtual void Exit() override;

private:
    _float3 m_vDir = { 0,0,0 };   // 대쉬 방향 (XZ)
    _vector m_vDashTarget = {};   // 도착 위치
    bool    m_bTeleported = false;
};

