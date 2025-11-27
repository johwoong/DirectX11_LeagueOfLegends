#pragma once
#include "UI_Base.h"
#include "Client_Defines.h"
#include "CursorManager.h"
class CCursor_UI final : public CUI_Base
{
private:
    CCursor_UI(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CCursor_UI(const CCursor_UI& rhs);
    virtual ~CCursor_UI() = default;

public:
    virtual HRESULT Initialize_Prototype();
    virtual HRESULT Initialize(void* pArg);
    virtual int    Update(_float fTimeDelta);
    virtual void   Late_Update(_float fTimeDelta);
    virtual HRESULT Render();

private:
    virtual HRESULT Ready_Components(void* pArg) override;
    HRESULT SetUp_ShaderResources();

private:
    // 커서 전용 텍스처 두 개
    class CTexture* m_pTexNormal = nullptr;
    class CTexture* m_pTexAttack = nullptr;

public:
    static CCursor_UI* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr);
    virtual void Free() override;
};

