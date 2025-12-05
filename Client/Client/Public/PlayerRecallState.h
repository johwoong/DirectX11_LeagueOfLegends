#pragma once
#include "PlayerState.h"

NS_BEGIN(Client)
NS_BEGIN(Player)
class CPlayerRecallState final : public CPlayerState
{

public:
	CPlayerRecallState(class CPlayer* pPlayer);

	virtual CPlayerState* HandleInput() override;
	virtual CPlayerState* Update(_float fTimeDelta) override;
	virtual CPlayerState* Late_Update(_float fTimeDelta) override;

	virtual void Enter() override;
	virtual void Exit() override;
	
private:
	_bool m_bIsRecall = false;
};
NS_END
NS_END
