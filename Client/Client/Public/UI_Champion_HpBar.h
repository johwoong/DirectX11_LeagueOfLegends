#pragma once
#include "Actor.h"

NS_BEGIN(Engine)
class CTexture;
class CVIBuffer_Rect;
NS_END

NS_BEGIN(Client)

class CChampion_HpBar final : public CActor
{
private:
    CChampion_HpBar(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CChampion_HpBar(const CChampion_HpBar& rhs);
    virtual ~CChampion_HpBar() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual int     Update(_float fDelta) override;
    virtual void    Late_Update(_float fDelta) override;
    virtual HRESULT Render() override;

private:
    virtual HRESULT Ready_Components(void* pArg) override;
    virtual HRESULT SetUp_ShaderResources() override;

private:
    void Update_Position_And_Scale();
public:
    void Set_Owner(CActor* pOwner);
    virtual void ReUse_Setting(void* pArg) override;
    void ReturnToPool();
private:
    CActor* m_pOwner = nullptr;

    float m_fMaxWidth = 0.8f;
    float m_fHeight = 0.08f;
    float m_fOffsetY = 2.0f;
    float m_fHpRatio = 0.f;

    // HPBar 전용 컴포넌트
    CVIBuffer_Rect* m_pVIBufferCom = nullptr;
    CTexture* m_pTextureCom = nullptr;
    CTexture* m_pFrameTextureCom = nullptr;

public:
    static CChampion_HpBar* Create(ID3D11Device*, ID3D11DeviceContext*);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void Free() override;
};

NS_END