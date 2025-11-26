#pragma once
#include "PlayerState.h"

NS_BEGIN(Client)
NS_BEGIN(Player)
class cPlayerChaseState final : public CPlayerState
{
public:
	cPlayerChaseState(class CPlayer* pPlayer, STATE_ID eStateType, STATE_ID eNextStateType);

	virtual CPlayerState* HandleInput() override;
	virtual CPlayerState* Update(_float fTimeDelta) override;
	virtual CPlayerState* Late_Update(_float fTimeDelta) override;

	virtual void Enter() override;
	virtual void Exit() override;

private:
	CActor* m_pTarget = nullptr;
	STATE_ID m_eNextState = STATE_END;
};
NS_END
NS_END

