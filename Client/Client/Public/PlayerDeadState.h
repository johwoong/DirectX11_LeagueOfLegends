#pragma once
#include "PlayerState.h"

NS_BEGIN(Client)
NS_BEGIN(Player)
class CPlayerDeadState final : public CPlayerState
{
public:
    CPlayerDeadState(CPlayer* pPlayer);

public:
    virtual CPlayerState* HandleInput() override;
    virtual CPlayerState* Update(_float fTimeDelta) override;
    virtual CPlayerState* Late_Update(_float fTimeDelta) override;
    virtual void Enter() override;
    virtual void Exit() override;

private:
    enum DEAD_PHASE
    {
        PHASE_DEATH_ANIM,
        PHASE_WAIT_RESPAWN,
        PHASE_SPAWN_ANIM,
    };

private:
    DEAD_PHASE m_ePhase = PHASE_DEATH_ANIM;
    float      m_fWaitAcc = 0.f;

private:
    void Start_DeathAnim();
    void Do_Respawn();
    void Start_SpawnAnim();
};
NS_END
NS_END