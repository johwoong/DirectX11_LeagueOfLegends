#pragma once
#include "PlayerState.h"

NS_BEGIN(Client)
NS_BEGIN(Player)

class CPlayerEmotionState final : public CPlayerState
{
public:
    enum EMOTION_TYPE { EMOTE_JOKE, EMOTE_LAUGH, EMOTE_TAUNT };

public:
    CPlayerEmotionState(class CPlayer* pPlayer, EMOTION_TYPE eType);

    virtual CPlayerState* HandleInput() override;
    virtual CPlayerState* Update(_float fTimeDelta) override;
    virtual CPlayerState* Late_Update(_float fTimeDelta) override;

    virtual void Enter() override;
    virtual void Exit() override;

private:
    EMOTION_TYPE m_eType;
};

NS_END
NS_END
