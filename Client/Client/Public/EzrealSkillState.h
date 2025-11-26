#pragma once
#include "PlayerState.h"


NS_BEGIN(Client)

NS_BEGIN(Player)
class CEzrealSkillState final : public CPlayerState
{
public:
	CEzrealSkillState(class CPlayer* pPlayer, STATE_ID eStateType);

	virtual CPlayerState* HandleInput() override;
	virtual CPlayerState* Update(_float fTimeDelta) override;
	virtual CPlayerState* Late_Update(_float fTimeDelta) override;

	virtual void Enter() override;
	virtual void Exit() override;
};
NS_END
NS_END

