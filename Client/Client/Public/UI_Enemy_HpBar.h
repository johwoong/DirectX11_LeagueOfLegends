#pragma once
#include "Actor.h"

NS_BEGIN(Engine)
class CTexture;
class CVIBuffer_Rect;
NS_END

NS_BEGIN(Client)
class CEnemy;  

class CEnemy_HpBar final : public CActor
{
private:
    CEnemy_HpBar(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CEnemy_HpBar(const CEnemy_HpBar& rhs);
    virtual ~CEnemy_HpBar() = default;

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
    void Set_Owner(CEnemy* pOwner);
private:
    CEnemy* m_pOwner = nullptr;

    // Enemy_HpBar.h
    float m_fMaxWidth = 2.5f;   // 기존 1.f → 더 크게
    float m_fHeight = 0.3f;   // 기존 0.2f → 조금 더 두껍게
    float m_fOffsetY = 3.0f;   // 머리 위 높이는 그대로 쓰거나 취향대로
    float m_fHpRatio = 0.f;
    float   m_fMaxHp = 1.f;   // ★ 추가: 처음 한 번 저장해두는 MaxHp
    CVIBuffer_Rect* m_pVIBufferCom = nullptr;
    CTexture* m_pTextureCom = nullptr;        // 실제 HP 바 텍스처
    CTexture* m_pFrameTextureCom = nullptr;   // 프레임 텍스처


public:
    static CEnemy_HpBar* Create(ID3D11Device*, ID3D11DeviceContext*);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void Free() override;
};

NS_END