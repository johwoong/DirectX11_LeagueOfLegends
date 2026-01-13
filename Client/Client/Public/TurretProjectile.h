#pragma once
#include "Projectile.h"

NS_BEGIN(Client)
class CTurretProjectile final : public CProjectile
{
public:
    CTurretProjectile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CTurretProjectile(const CTurretProjectile& rhs);
    virtual ~CTurretProjectile() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual int Update(_float fTimeDelta) override;
    virtual void Late_Update(_float fTimeDelta) override;
    virtual void Collision_Check() override;

private:
    void Update_Projectile(_float fTimeDelta);
    void Dead_Effect();
    void Spawn_HitEffect(CActor* pHit);
protected:
    virtual void ReUse_Setting(void* pArg) override;
protected:
    virtual HRESULT Ready_Components(void* pArg = nullptr) override;

public:
    static CTurretProjectile* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr) override;
    virtual void Free() override;
};
NS_END
