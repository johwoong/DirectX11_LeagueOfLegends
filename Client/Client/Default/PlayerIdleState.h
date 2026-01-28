#pragma once
#include "PlayerState.h"

NS_BEGIN(Client)
NS_BEGIN(Player)

class CIdleState final : public CPlayerState
{
public:
    enum IDLETYPE { IDLE_MAIN, IDLE_END };

public:
    CIdleState(class CPlayer* pPlayer, IDLETYPE eIdleType);

    virtual CPlayerState* HandleInput() override;
    virtual CPlayerState* Update(_float fTimeDelta) override;
    virtual CPlayerState* Late_Update(_float fTimeDelta) override;

    virtual void Enter() override;
    virtual void Exit() override;

private:
    IDLETYPE m_eIdleType = IDLE_END;
    vector<_uint> m_vecIdleAnim;

private: // ? 대사 재생용
    float m_fTalkTimer = 0.f;     // 다음 체크까지 남은 시간
    float m_fNoTalkAcc = 0.f;     // Idle 진입 후 말 금지 누적 (게이트)
};

NS_END
NS_END