// =======================
// Nexus.h
// =======================
#pragma once
#include "Building.h"
#include <vector>

NS_BEGIN(Client)

class CEffect;

class CNexus final : public CBuilding
{
public:
    enum STATE { DEATH, IDLE, SPAWN };

private:
    CNexus(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CNexus(const CNexus& rhs);
    virtual ~CNexus() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual int     Update(_float fTimeDelta) override;
    virtual void    Late_Update(_float fTimeDelta) override;

public: /* For.State */
    virtual _bool   Is_AnimationLoop(_uint eAnimId);
    void            Update_State(_float fTimeDelta);
    void            LateUpdate_State(_float fTimeDelta);

public:
    virtual HPBARDESC Get_HpBarDesc() const override;

public:
    virtual _int Take_Damage(int iDamage, CActor* DamageCauser) override;
    bool IsDestroyed() const { return m_bDestroyed; }

    void Tick_GameOverSequence(_float fUnscaledDelta);

private:
    virtual HRESULT Ready_Components(void* pArg) override;

private:
    void Begin_Destroy();  
    bool IsRedTeam() const;  

private:
    _uint m_eState = IDLE;

    bool m_bDestroyed = false;
    bool m_bDeathFxPlayed = false;
    vector<CEffect*> m_vecDeathFx;

public:
    static CNexus* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr) override;
    virtual void Free() override;
};

NS_END


