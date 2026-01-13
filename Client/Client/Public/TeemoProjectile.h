#pragma once
#include "Projectile.h"

NS_BEGIN(Client)
class CTemooProjectile final : public CProjectile
{
public:
    enum TYPE { BASE, Q_BLIND};

public:
    CTemooProjectile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CTemooProjectile(const CTemooProjectile& rhs);
    virtual ~CTemooProjectile() = default;

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
    // 직선 사거리 체크용(현재 BASE는 타겟 추적이라 보통 0으로 둠)
    _vector m_vStartPos = XMVectorZero();
    float   m_fMaxDistance = 0.f;

private:
    bool Is_RedTarget(CActor* pActor) const;

public:
    static CTemooProjectile* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr) override;
    virtual void Free() override;
};
NS_END
