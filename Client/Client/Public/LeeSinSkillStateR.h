#pragma once
#include "PlayerState.h"

NS_BEGIN(Client)
class CLeeSin;
class CActor;

NS_BEGIN(Player)

class CLeeSinSkillStateR final : public CPlayerState
{
public:
    CLeeSinSkillStateR(CPlayer* pPlayer);
    virtual ~CLeeSinSkillStateR() = default;

public:
    virtual CPlayerState* HandleInput() override;
    virtual CPlayerState* Update(_float fDeltaTime) override;
    virtual CPlayerState* Late_Update(_float fDeltaTime) override;

    virtual void Enter() override;
    virtual void Exit() override;

private:
    void Create_RCollider();
    void Update_RCollider();
    void Destroy_RCollider();

    void Do_DragonRage();          // 타격 판정 + 데미지 + 넉백 시작
    void Update_Knockback(_float dt);

private:
    CLeeSin* m_pLeeSin = nullptr;
    CActor* m_pTarget = nullptr;     // 타겟팅 대상(선택된 공격 대상)

    CColider* m_pRCollider = nullptr;
    bool      m_bColliderEventPlaying = false;
    bool      m_bDidHit = false;
    bool      m_bAnimFinished = false;
    float     m_fAcc = 0.f;

    // ===== 히트박스 튜닝 =====
    _float3   m_vRBoxScale = _float3(2.2f, 1.8f, 2.4f);
    float     m_fROffset = 1.0f;   // 전방으로 얼마나 밀지
    float     m_fRYOffset = 0.6f;   // 콜라이더 높이

    // ===== R 데미지/넉백 =====
    int       m_iDamageR = 200;
    float     m_fKnockDuration = 0.35f;
    float     m_fKnockSpeed = 18.f;

    bool      m_bKnockActive = false;
    float     m_fKnockAcc = 0.f;
    _vector   m_vKnockDir = XMVectorSet(0.f, 0.f, 1.f, 0.f);
};

NS_END
NS_END
