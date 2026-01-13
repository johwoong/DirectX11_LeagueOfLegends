#pragma once
#include "PlayerState.h"

NS_BEGIN(Client)
NS_BEGIN(Player)

class CTemooSkillStateW final : public CPlayerState
{
public:
    CTemooSkillStateW(class CPlayer* pPlayer);

public:
    virtual CPlayerState* HandleInput() override;
    virtual CPlayerState* Update(_float fTimeDelta) override;
    virtual CPlayerState* Late_Update(_float fTimeDelta) override;

    virtual void Enter() override;
    virtual void Exit() override;

private:
    void Apply_WBuff_And_SpawnFX();
    void Spawn_TemooW_Effect(); // ★ Temoo_W.dat (follow)

private:
    bool  m_bApplied = false;

    _float m_fDuration = 3.f;     // W 지속시간
    _float m_fSpeedMul = 1.35f;   // 이속 배율
};

NS_END
NS_END

