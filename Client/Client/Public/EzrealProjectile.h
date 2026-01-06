#pragma once
#include "Projectile.h"

NS_BEGIN(Client)
class CEzrealProjectile final : public CProjectile
{
public:
    enum TYPE { BASE, Q, W, R };

public:
    CEzrealProjectile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CEzrealProjectile(const CEzrealProjectile& rhs);
    virtual ~CEzrealProjectile() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual int Update(_float fTimeDelta) override;
    virtual void Late_Update(_float fTimeDelta) override;
    virtual void Collision_Check() override;

    void Dead_Effect();
    void Spawn_Effect(CActor* pHitObj);

protected:
    virtual HRESULT Ready_Components(void* pArg = nullptr) override;

private:
    void Update_Projectile(_float fTimeDelta);
protected:
    virtual void ReUse_Setting(void* pArg) override;
private:
    // 직선 사거리 체크용
    _vector m_vStartPos = XMVectorZero();
    float   m_fMaxDistance = 0.f;

private: // R 피격 체크
    static const int MAX_R_HIT = 64;   
    CActor* m_RHitList[MAX_R_HIT] = {};
    int     m_iRHitCount = 0;

    bool HasHitTarget(CActor* pActor) const;
    void RegisterHitTarget(CActor* pActor);

private:
    // ? "무조건 레드만 맞는다" 필터
    bool Is_RedTarget(CActor* pActor) const;

public:
    static CEzrealProjectile* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr) override;
    virtual void Free() override;
};
NS_END
