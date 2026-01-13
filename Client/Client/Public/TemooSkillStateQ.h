#pragma once
#include "PlayerState.h"

NS_BEGIN(Client)

NS_BEGIN(Player)

class CTemooSkillStateQ final : public CPlayerState
{
public:
	CTemooSkillStateQ(class CPlayer* pPlayer);

public:
	virtual CPlayerState* HandleInput() override;
	virtual CPlayerState* Update(_float fTimeDelta) override;
	virtual CPlayerState* Late_Update(_float fTimeDelta) override;

	virtual void Enter() override;
	virtual void Exit() override;

private:
	void Spawn_TemooQProjectile(class CActor* pTarget);

private:
	bool m_bProjectileSpawned = false;

private:
	enum class PHASE : uint8_t { ATTACK, TOIDLE };
	PHASE m_ePhase = PHASE::ATTACK;

	_uint m_iAttackAnim = 0; // Q æ÷¥œ ¿Œµ¶Ω∫
};

NS_END
NS_END
