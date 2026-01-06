#pragma once
#include "Projectile.h"

NS_BEGIN(Client)

class CVayneProjectile final : public CProjectile
{
public:
    enum TYPE
    {
        BASE = 0,        // 평타
        Q_EMPOWER = 1,   // Q 강화 평타
        E_CONDEMN = 2    // ?? 베인 E
    };
public:
    CVayneProjectile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CVayneProjectile(const CVayneProjectile& rhs);
    virtual ~CVayneProjectile() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual int     Update(_float fTimeDelta) override;
    virtual void    Late_Update(_float fTimeDelta) override;
    virtual void    Collision_Check() override;

    void Dead_Effect();
    void Spawn_Effect(CActor* pHitObj);

protected:
    virtual HRESULT Ready_Components(void* pArg = nullptr) override;
    virtual void    ReUse_Setting(void* pArg) override;

private:
    void Update_Projectile(_float fTimeDelta);

private:
    bool Is_BlueTarget(CActor* pActor) const;

private:
    _vector m_vStartPos = XMVectorZero();
    float   m_fMaxDistance = 0.f;
private:
    bool m_bHitOnce = false;   // ? 히트 1회 보장용
public:
    static CVayneProjectile* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr) override;
    virtual void Free() override;
};

NS_END
