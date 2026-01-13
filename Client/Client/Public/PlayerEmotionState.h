#pragma once
#include "PlayerState.h"

NS_BEGIN(Client)
NS_BEGIN(Player)

class CPlayerEmotionState final : public CPlayerState
{
private:
    enum class TEEMO_PHASE : uint8_t
    {
        NONE,
        JOKE_IN,
        JOKE_SPAM,
        JOKE_OUT,
        LAUGH,
        LAUGH_TOIDLE,
        TAUNT,
    };
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

    TEEMO_PHASE m_eTeemoPhase = TEEMO_PHASE::NONE;
    float m_fPhaseAcc = 0.f;

    // JOKE_SPAM 재생 시간(원하는 만큼 조절)
    float m_fJokeSpamDuration = 1.2f;
};

NS_END
NS_END
