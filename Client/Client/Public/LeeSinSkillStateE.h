#pragma once
#include "PlayerState.h"

NS_BEGIN(Client)
class CLeeSin;
class CActor;
NS_BEGIN(Player)

class CLeeSinSkillStateE final : public CPlayerState
{
public:
    enum class PHASE { CAST_E1, CAST_E2 };

public:
    CLeeSinSkillStateE(CPlayer* pPlayer);
    virtual ~CLeeSinSkillStateE() = default;

public:
    virtual CPlayerState* HandleInput() override;
    virtual CPlayerState* Update(_float fDeltaTime) override;
    virtual CPlayerState* Late_Update(_float fDeltaTime) override;

    virtual void Enter() override;
    virtual void Exit() override;

private:
    void Do_Tempest();   // E1: AABB 광역 데미지 + 표식 저장
    void Do_Cripple();   // E2: 표식 대상만 데미지/슬로우

private:
    void Create_E1Collider();
    void Update_E1Collider();
    void Destroy_E1Collider();

private:
    CLeeSin* m_pLeeSin = nullptr;

    // ★ E1에만 쓰는 콜라이더 (AttackState처럼)
    CColider* m_pE1Collider = nullptr;
    bool      m_bColliderEventPlaying = false;

    PHASE  m_ePhase = PHASE::CAST_E1;
    float  m_fAcc = 0.f;
    bool   m_bDidHit = false;
    bool   m_bAnimFinished = false;

    // ★ 원(radius) 계산 안 함. 그냥 AABB 크기로 조절해
    _float3 m_vE1BoxScale = _float3(3.5f, 1.5f, 3.5f); // 여기만 튜닝
    float   m_fE1Offset = 0.0f; // 0이면 발밑 중심, 앞으로 밀고싶으면 0.8f 같은 값

    int    m_iDamageE1 = 80;
    int    m_iDamageE2 = 40; // 원하면 0 + 슬로우만
  
};

NS_END
NS_END
