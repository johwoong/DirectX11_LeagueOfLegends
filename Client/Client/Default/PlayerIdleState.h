#pragma once

#include "PlayerState.h"
NS_BEGIN(Client)
NS_BEGIN(Player)
class CIdleState final : public CPlayerState
{
public:
	enum IDLETYPE {IDLE_MAIN, IDLE_END};

public:
	CIdleState(class CPlayer* pPlayer, IDLETYPE eIdleType);

	virtual CPlayerState* HandleInput() override;
	virtual CPlayerState* Update(_float fTimeDelta) override;
	virtual CPlayerState* Late_Update(_float fTimeDelta) override;
	 
	virtual void Enter() override;
	virtual void Exit() override;

private:
	IDLETYPE m_eIdleType = IDLE_END;
	
};
NS_END
NS_END