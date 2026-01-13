#pragma once
#include "PlayerState.h"

NS_BEGIN(Client)
NS_BEGIN(Player)
class CPlayerRecallState final : public CPlayerState
{
public:
    CPlayerRecallState(CPlayer* pPlayer);

public:
    virtual CPlayerState* HandleInput() override;
    virtual CPlayerState* Update(_float fTimeDelta) override;
    virtual CPlayerState* Late_Update(_float fTimeDelta) override;
    virtual void Enter() override;

    virtual void Exit() override;

private:
    enum RECALL_PHASE
    {
        PHASE_RECALL,
        PHASE_WINDDOWN,
    };

private:
    RECALL_PHASE m_ePhase = PHASE_RECALL;
    bool m_bIsRecall = false;
};
NS_END
NS_END
