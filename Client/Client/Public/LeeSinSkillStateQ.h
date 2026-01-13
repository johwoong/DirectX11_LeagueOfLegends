#pragma once
#include "PlayerState.h"

NS_BEGIN(Client)
class CLeeSin;
class CActor;
NS_BEGIN(Player)

class CLeeSinSkillStateQ final : public CPlayerState
{
public:
    enum class PHASE { CAST_Q1, DASH_Q2, TRANS_HIT };

public:
    CLeeSinSkillStateQ(CPlayer* pPlayer);
    virtual ~CLeeSinSkillStateQ() = default;

public:
    virtual CPlayerState* HandleInput() override;
    virtual CPlayerState* Update(_float fDeltaTime) override;
    virtual CPlayerState* Late_Update(_float fDeltaTime) override;

    virtual void Enter() override;
    virtual void Exit() override;

private:
    void Enter_CastQ1();
    void Enter_DashQ2();
    void Enter_Trans();

    void Update_CastQ1(_float dt);
    void Update_DashQ2(_float dt);
    void Update_Trans(_float dt);

    void Spawn_QWaveProjectile();  // Q1 투사체

private:
    CLeeSin* m_pOwner = nullptr;
    CActor* m_pTarget = nullptr;

    PHASE m_ePhase = PHASE::CAST_Q1;

    _float3 m_vDir = { 0.f,0.f,0.f };  // Q1 발사 방향(Enter에서 세팅)
    bool    m_bProjectileSpawned = false;
    bool    m_bHitApplied = false;

    int     m_iDashType = 0;           // 0 short, 1 mid, 2 long
    float   m_fDashSpeed = 22.f;       // 튜닝
    float   m_fQ2StopDist = 1.05f;      // 타겟과 유지할 거리(튜닝)
    bool    m_bQ2HitboxSpawned = false; // TRANS에서 히트박스 1회 스폰

    // 공격처럼 콜라이더 재사용
    CColider* m_pQ2Collider = nullptr;
    bool      m_bQ2DamageApplied = false;

    // 튜닝
    float     m_fQ2HitOffset = 1.15f;              // 리신 앞쪽에 콜라이더 두는 거리
    _float3   m_vQ2HitScale = _float3(1.6f, 1.2f, 1.6f); // 콜라이더 크기(공격이랑 비슷하게)

private:
    bool m_bForceExitIdle = false;   // 대시 중 타겟이 죽거나 사라지면 강제 종료
    bool m_bQ2Locked = false;        // Q2 대시 시작하면 표식 검사 끊고 타겟 LOCK
};

NS_END
NS_END


