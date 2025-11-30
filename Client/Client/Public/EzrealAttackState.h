#pragma once
#include "PlayerState.h"

NS_BEGIN(Client)

NS_BEGIN(Player)
class CEzrealAttackState final : public CPlayerState
{
public:
	CEzrealAttackState(class CPlayer* pPlayer, STATE_ID eStateType, _float fTime = 0.f);

	virtual CPlayerState* HandleInput() override;
	virtual CPlayerState* Update(_float fTimeDelta) override;
	virtual CPlayerState* Late_Update(_float fTimeDelta) override;

	virtual void Enter() override;
	virtual void Exit() override;

private:
	bool m_bProjectileSpawned = false;
};
NS_END
NS_END
