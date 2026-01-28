#pragma once
#include "PlayerState.h"

NS_BEGIN(Client)
NS_BEGIN(Player)
class CRunState final : public CPlayerState
{
public:
	CRunState(class CPlayer* pPlayer);
	virtual CPlayerState* HandleInput() override;
	virtual CPlayerState* Update(_float fTimeDelta) override;
	virtual CPlayerState* Late_Update(_float fTimeDelta);

	virtual void Enter() override;
	virtual void Exit() override;

private:
	void Move(_float fTimeDelta);

private:
	_float3 m_vTargetPos = { 0.f, 0.f, 0.f };
	STATETYPE m_eStateType = STATETYPE_START;
private: // ? Run ´ë»ç¿ë
	float m_fRunNoTalkAcc = 0.f;
	float m_fRunTalkTimer = 0.f;
};
NS_END
NS_END

