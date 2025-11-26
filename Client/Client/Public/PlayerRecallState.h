#pragma once
#include "PlayerState.h"

NS_BEGIN(Client)
NS_BEGIN(Player)
class CPlayerRecallState final : public CPlayerState
{
public:
	CPlayerRecallState(class CPlayer* pPlayer, STATE_ID eStateID);

	virtual CPlayerState* HandleInput(void) override;
	virtual CPlayerState* Update(_float fTimeDelta) override;
	virtual CPlayerState* Late_Update(_float fTimeDelta) override;

	virtual void Enter(void) override;
	virtual void Exit(void) override;
};
NS_END
NS_END
