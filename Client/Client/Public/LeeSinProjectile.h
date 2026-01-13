#pragma once
#include "Projectile.h"

NS_BEGIN(Client)
class CLeeSinProjectile final : public CProjectile
{
public:
    CLeeSinProjectile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CLeeSinProjectile(const CLeeSinProjectile& rhs);
    virtual ~CLeeSinProjectile() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual int Update(_float fTimeDelta) override;
    virtual void Late_Update(_float fTimeDelta) override;
    virtual void Collision_Check() override;

private:
    void Update_Projectile(_float fTimeDelta);
    void Align_ToDir(_vector vDir);
    void Spawn_FlyEffect();
    void Spawn_HitEffect(CActor* pHit);
    void Dead_Effect();

protected:
    virtual void ReUse_Setting(void* pArg) override;
    virtual HRESULT Ready_Components(void* pArg = nullptr) override;

private:
    bool    m_bHit = false;
    _vector m_vStartPos = XMVectorZero();
    _float  m_fMaxDistance = 12.f;      // Q1 사거리(XZ)
    _float  m_fMarkDuration = 3.f;      // 표식 지속

    // ===== FX (Ezreal처럼) =====
    vector<CEffect*> m_pEffects;

public:
    static CLeeSinProjectile* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr) override;
    virtual void Free() override;
};
NS_END
