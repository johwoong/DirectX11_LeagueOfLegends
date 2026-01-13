#pragma once
#include "PlayerState.h"

NS_BEGIN(Client)

NS_BEGIN(Player)
class CLeeSinAttackState final : public CPlayerState
{
public:
    CLeeSinAttackState(class CPlayer* pPlayer);

    virtual CPlayerState* HandleInput() override;
    virtual CPlayerState* Update(_float fTimeDelta) override;
    virtual CPlayerState* Late_Update(_float fTimeDelta) override;

    virtual void Enter() override;
    virtual void Exit() override;

private:
    // 콤보 관리
    bool  m_bLeftAttack = true;     // true = Left, false = Right
    float m_fComboTimer = 0.f;
    float m_fComboLimit = 0.45f;    // 이 시간 안에 다음 공격 입력 시 연계
private:
    // 공격용 충돌체
    CColider* m_pAttackCollider = nullptr;
    _matrix   m_ColliderMatrix{};
    bool      m_bDamageApplied = false;
};
NS_END
NS_END
