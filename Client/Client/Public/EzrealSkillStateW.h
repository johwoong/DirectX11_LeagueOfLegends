#pragma once
#include "PlayerState.h"
class CEzrealSkillStateW final : public CPlayerState
{
public:
    enum CAST_MODE
    {
        CAST_IDLE,   // 서 있다가 W (W → IDLEW → IDLE)
        CAST_RUN     // 뛰다가 W (RUNW 안에서 이동 재개)
    };

public:
    // eMode 기본값을 CAST_IDLE로 둠 -> 예전 코드(new CEzrealSkillStateW(m_pOwner))도 그대로 동작
    explicit CEzrealSkillStateW(CPlayer* pPlayer,
        CAST_MODE eMode = CAST_IDLE,
        const _float3* pTargetPos = nullptr);

    virtual CPlayerState* HandleInput() override;
    virtual CPlayerState* Update(_float fTimeDelta) override;
    virtual CPlayerState* Late_Update(_float fTimeDelta) override;

    virtual void Enter() override;
    virtual void Exit() override;

private:
    // 공통
    bool    m_bProjectileSpawned = false;
    _float3 m_vDir = { 0.f,0.f,0.f }; // W 발사 방향

    // IDLE 캐스팅용
    _bool   m_bIdleWPhase = false;

    // RUN 캐스팅용
    CAST_MODE m_eCastMode = CAST_IDLE;
    bool      m_bMoveStarted = false;          // RUNW 안에서 EVENT_INPUT(또는 MOVE_START) 받은 후
    _float3   m_vRunTarget = { 0.f,0.f,0.f }; // 다시 달려갈 목적지
};

