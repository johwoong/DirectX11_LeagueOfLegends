#pragma once
#include "Actor.h"
#include "GameInstance.h"

class CProjectile final : public CActor
{
public:
    enum PROJ_TYPE
    {
        PROJ_TARGET,
        PROJ_LINEAR,
        PROJ_AREA,
    };

    // ★ 어떤 스킬에서 나온 투사체인지
    enum SKILL_TAG
    {
        SKILL_NONE,
        SKILL_EZREAL_Q,
        SKILL_EZREAL_W,
        SKILL_EZREAL_R,
    };

    struct PROJECTILEDESC
    {
        PROJ_TYPE eType = PROJ_TARGET;

        _vector vStartPos;

        // === 공통 ===
        float fSpeed = 20.f;
        float fLifeTime = 3.f;
        float fDamage = 50.f;

        // === 타겟추적형 전용 ===
        CGameObject* pTarget = nullptr;

        // === 직선형 전용 ===
        _float3 vDir = { 0,0,0 };
        float   fMaxDistance = 600.f;

        // === 기타 ===
        CActor* pOwner = nullptr;

        // === 콜라이더 커스텀 설정 ===
        _bool   bUseCustomCollider = false;
        _float3 vColliderScale = { 0.f, 0.f, 0.f };
        float   fColliderRadius = 0.f;
        _float3 vColliderRotation = { 0.f, 0.f, 0.f };
        _float3 vColliderOffset = { 0.f, 0.f, 0.f };

        // ★ 추가: 이 투사체가 어떤 스킬에서 나왔는지
        SKILL_TAG eSkillTag = SKILL_NONE;
    };

private:
    CProjectile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CProjectile(const CProjectile& rhs);
    virtual ~CProjectile() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;

    virtual int Update(_float fDeltaTime) override;
    virtual void Late_Update(_float fDeltaTime) override;
    virtual HRESULT Render() override;

private:
    PROJECTILEDESC  m_Desc;
    float           m_fAccTime = 0.f;
    float           m_fTraveled = 0.f;

private:
    vector<CActor*> m_HitActors;   // R 등 관통형 광역 스킬용
private:
    HRESULT Ready_Components(void* pArg) override;
    HRESULT SetUp_ShaderResources() override { return S_OK; }

    void Move_Target(_float fDeltaTime);
    void Move_Linear(_float fDeltaTime);

    void Update_Collider_Size_ByType();
    void Check_Hit();

public:
    static CProjectile* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void Free() override;
};

