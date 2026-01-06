#pragma once
#include "Projectile.h"

NS_BEGIN(Client)
class CMinionProjectile final : public CProjectile
{
public:
    CMinionProjectile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CMinionProjectile(const CMinionProjectile& rhs);
    virtual ~CMinionProjectile() = default;

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
    virtual HRESULT Ready_Components(void* pArg = nullptr) override;

public:
    static CMinionProjectile* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr) override;
    virtual void Free() override;
};
NS_END
