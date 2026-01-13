#pragma once
#include "UI_Base.h"
#include "Client_Defines.h"
class CEnding_UI final : public CUI_Base
{
private:
    CEnding_UI(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CEnding_UI(const CEnding_UI& rhs);
    virtual ~CEnding_UI() = default;

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
    // UI_Base가 가진 m_pTextureCom = "배경(eog_base)"
    class CTexture* m_pTextureTextCom = nullptr;  // "승리/문구(Texture.dds)"
private:
    _float2 m_vTextSize = { 0.f, 0.f };
    _float2 m_vTextPos = { 0.f, 0.f };
public:
    static CEnding_UI* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr) override;
    virtual void Free() override;
};

