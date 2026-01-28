#pragma once
#include "UI_Base.h"
#include "Client_Defines.h"

NS_BEGIN(Client)

class CUI_TextImage final : public CUI_Base
{
public:
    enum class TEXT_TYPE : _uint
    {
        WELCOME_SUMMONERS_RIFT = 0,
        MINION_HAS_SPAWNED,
        END
    };


    struct TEXTIMAGEDESC
    {
        TEXT_TYPE eType = TEXT_TYPE::WELCOME_SUMMONERS_RIFT;
        float fLifeTime = 2.5f;
        bool  bAutoDead = true;

        _float2 vCenterPos = { g_WinSizeX * 0.5f, g_WinSizeY * 0.18f };
        _float2 vSize = { g_WinSizeX * 0.45f, g_WinSizeY * 0.07f };
        float   fAlpha = 1.f;
    };

private:
    CUI_TextImage(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CUI_TextImage(const CUI_TextImage& rhs);
    virtual ~CUI_TextImage() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual int     Update(_float fTimeDelta) override;
    virtual void    Late_Update(_float fTimeDelta) override;
    virtual HRESULT Render() override;

private:
    virtual HRESULT Ready_Components(void* pArg) override;
    HRESULT SetUp_ShaderResources();

private:
    class CTexture* m_pTextureTextCom = nullptr;  // ∏‡∆Æ ≈ÿΩ∫√≥ π≠¿Ω

private:
    TEXTIMAGEDESC m_tDesc{};
    float m_fAcc = 0.f;
    int   m_iTextTexIndex = 0;

    _float2 m_vTextSize = { 0.f, 0.f };
    _float2 m_vTextPos = { 0.f, 0.f };

public:
    static CUI_TextImage* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr) override;
    virtual void Free() override;
};

NS_END
