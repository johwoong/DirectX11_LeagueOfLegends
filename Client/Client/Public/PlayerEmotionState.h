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
    void PlayEmotionSound();
    void StopEmotionSound();

    // ? 챔피언/이모션 -> 사운드 파일 매핑
    const TCHAR* GetEmotionSoundFile() const;

private:
    EMOTION_TYPE m_eType;

    TEEMO_PHASE m_eTeemoPhase = TEEMO_PHASE::NONE;
    float m_fPhaseAcc = 0.f;

    float m_fJokeSpamDuration = 1.2f;
};

NS_END
NS_END
