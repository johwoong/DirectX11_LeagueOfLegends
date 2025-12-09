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
    void Set_Owner(CActor* pOwner) { m_pOwner = pOwner; }
    void Set_Visible(_bool bVisible) { m_bVisible = bVisible; }
    _bool Is_Visible() const { return m_bVisible; }
    void Set_RadiusWorld(float fRadius) { m_fRadiusWorld = fRadius; }
    void Set_Alpha(float fAlpha) { m_fAlpha = fAlpha; }
    void Set_Color(const _float4& vColor) { m_vColor = vColor; }
private:
    _bool Is_OwnerStillExist() const;

private:
    virtual HRESULT Ready_Components(void* pArg) override;
    virtual HRESULT SetUp_ShaderResources() override;
    virtual HRESULT SetUp_ShaderID() override { return S_OK; }

private:
    CActor* m_pOwner = nullptr;
    _bool       m_bVisible = false; // 토글용
    CVIBuffer_Quad* m_pVIBufferCom = nullptr;
    CTexture* m_pTextureCom = nullptr;

private:
    float   m_fRadiusWorld = 0.f;  // 공격 사거리(월드 값)
    _float4 m_vColor = _float4(0.f, 1.f, 1.f, 1.f); // 기본 시안
public:
    static CAttackRange* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr) override;
    virtual void          Free() override;
};

NS_END
