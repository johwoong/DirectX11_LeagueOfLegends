#pragma once
#include "Actor.h"


NS_BEGIN(Engine)
class CTexture;
class CVIBuffer_Quad;
NS_END

NS_BEGIN(Client)
class CChampion;

class CAttackRange final : public CActor
{
private:
    CAttackRange(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CAttackRange(const CAttackRange& rhs);
    virtual ~CAttackRange() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual int     Update(_float fTimeDelta) override;
    virtual void    Late_Update(_float fTimeDelta) override;
    virtual HRESULT Render() override;

public:
    void Set_Owner(CChampion* pOwner) { m_pOwner = pOwner; }
    void Set_Visible(_bool bVisible) { m_bVisible = bVisible; }
    _bool Is_Visible() const { return m_bVisible; }

private:
    virtual HRESULT Ready_Components(void* pArg) override;
    virtual HRESULT SetUp_ShaderResources() override;
    virtual HRESULT SetUp_ShaderID() override { return S_OK; }

private:
    CChampion* m_pOwner = nullptr;
    _bool       m_bVisible = false; // Åä±Û¿ë
    CVIBuffer_Quad* m_pVIBufferCom = nullptr;
    CTexture* m_pTextureCom = nullptr;
public:
    static CAttackRange* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr) override;
    virtual void          Free() override;
};

NS_END
