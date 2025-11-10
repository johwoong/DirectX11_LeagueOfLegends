#pragma once
#include "PlayerState.h"

NS_BEGIN(Client)
NS_BEGIN(Player)
class CRunState final : public CPlayerState
{
public:
	CRunState(class CPlayer* pPlayer, DIRID eDir);
	virtual CPlayerState* HandleInput() override;
	virtual CPlayerState* Update(_float fTimeDelta) override;
	virtual CPlayerState* Late_Update(_float fTimeDelta);

	virtual void Enter() override;
	virtual void Exit() override;

private:
	void Move(_float fTimeDelta);

private:
	DIRID m_eDirection = DIRID::DIR_END;
	_float3 m_vTargetPos = { 0.f, 0.f, 0.f };
	STATETYPE m_eStateType = STATETYPE_START;
};
NS_END
NS_END

