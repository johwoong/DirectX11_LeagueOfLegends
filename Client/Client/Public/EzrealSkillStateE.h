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
    enum class E_BLINK_FX { CAST, ARRIVE };
    void Spawn_E_BlinkFx(_vector vPos, E_BLINK_FX eType);

private:
    _float3 m_vDir = { 0,0,0 };   // 대쉬 방향 (XZ)
    _vector m_vDashTarget = {};   // 도착 위치
    bool    m_bTeleported = false;
    _vector m_vCastPos = XMVectorZero();  // 시전 위치 저장
    bool   m_bCastFxSpawned = false;
    bool   m_bArriveFxSpawned = false;
};

