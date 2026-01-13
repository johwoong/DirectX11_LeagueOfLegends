#pragma once
#include "PlayerState.h"

NS_BEGIN(Client)
class CLeeSin;
class CActor;
NS_BEGIN(Player)

class CLeeSinSkillStateW  final : public CPlayerState
{
public:
    // ★ 타겟을 고정해서 받는다 (키 누른 순간의 호버 대상)
    CLeeSinSkillStateW(CPlayer* pPlayer, CActor* pTarget);
    virtual ~CLeeSinSkillStateW() = default;

public:
    virtual CPlayerState* HandleInput() override { return nullptr; }
    virtual CPlayerState* Update(_float dt) override;
    virtual CPlayerState* Late_Update(_float dt) override { return nullptr; }

    virtual void Enter() override;
    virtual void Exit() override;

private:
    void DashToTarget(_float dt);
    bool IsValidAllyMinion(CActor* p) const;

private:
    // 튜닝
    float m_fShieldHP = 80.f;
    float m_fShieldDur = 2.0f;
private:
    bool  m_bSelfCast = false;
    float m_fSelfAcc = 0.f;
    float m_fDashSpeed = 22.f;
    float m_fStopDist = 0.15f;
    bool m_bRecast = false;
    bool  m_bApplied = false;
    bool  m_bFinished = false;
private:
    CPlayer* m_pPlayer = nullptr; 
    CLeeSin* m_pOwner = nullptr;
    CActor* m_pTarget = nullptr;
};

NS_END
NS_END