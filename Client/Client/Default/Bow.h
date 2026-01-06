#pragma once
#include "Actor.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CModel;
NS_END

NS_BEGIN(Client)
class CBow final : public CActor
{
public:
    enum ANIM { DEATH, IDLE, IDLE_END };

    struct BOWDESC
    {
        CGameObject* pOwner = nullptr;
        _vector      vPos = XMVectorZero();
        _vector      vDir = XMVectorSet(0, 0, 1, 0);
    };


private:
    CBow(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CBow(const CBow& rhs);
    virtual ~CBow() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual int Update(_float fTimeDelta) override;
    virtual void Late_Update(_float fTimeDelta) override;
    virtual HRESULT Render() override;
    virtual HRESULT Render_Glow() override;
    virtual HRESULT Render_ShadowDepth() override;

public:
    CModel* Get_Model() { return m_pModelCom; }
    virtual void Ready_Animation_Event();

private:
    virtual HRESULT Ready_Components(void* pArg = nullptr) override;
    virtual HRESULT SetUp_ShaderResources() override;

private:
    CModel* m_pModelCom = nullptr;
    BOWDESC  m_tDesc{};

private:
    _bool  m_bSpawned = false;
    _float m_fLifeAcc = 0.f; 
    bool     m_bProjectileSpawned = false;

public:
    static CBow* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr) override;
    virtual void Free() override;
};
NS_END
