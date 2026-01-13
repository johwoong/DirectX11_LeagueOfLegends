#pragma once
#include "PlayerState.h"

NS_BEGIN(Client)

NS_BEGIN(Player)
class CTemooAttackState final : public CPlayerState
{
public:
    CTemooAttackState(class CPlayer* pPlayer, STATE_ID eStateType, _float fTime = 0.f);

    virtual CPlayerState* HandleInput() override;
    virtual CPlayerState* Update(_float fTimeDelta) override;
    virtual CPlayerState* Late_Update(_float fTimeDelta) override;

    virtual void Enter() override;
    virtual void Exit() override;

private:
    void Begin_AttackCycle();
    void Spawn_TemooProjectile(class CActor* pTarget);

private:
    bool m_bProjectileSpawned = false;

private:
    enum class PHASE : uint8_t { ATTACK, TOIDLE };
    PHASE m_ePhase = PHASE::ATTACK;
    _uint m_iAttackAnim = 0;
};
NS_END
NS_END
