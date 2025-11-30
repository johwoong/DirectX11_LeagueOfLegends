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
    bool m_bProjectileSpawned = false;   // 이벤트 기반 발사
    _float3 m_vDir = { 0,0,0 };          // 마우스 방향
    _vector m_vDashTarget;
    bool m_bTeleported = false;
};

