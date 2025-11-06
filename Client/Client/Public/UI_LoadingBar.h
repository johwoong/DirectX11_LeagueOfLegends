#pragma once
#include "UI_Base.h"
#include "Loader.h"
#include "UI_Text.h"


class CUI_LoadingBar final : public CUI_Base
{
public:
    CUI_LoadingBar(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CUI_LoadingBar(const CUI_LoadingBar& rhs);
    virtual ~CUI_LoadingBar() = default;

public:
    virtual HRESULT Initialize_Prototype();
    virtual HRESULT Initialize(void* pArg);
    virtual int Update(_float fTimeDelta);
    virtual void Late_Update(_float fTimeDelta);
    virtual HRESULT Render();

private:
    HRESULT Ready_Components(void* pArg) override;
    HRESULT SetUp_ShaderResources() override;

private:
    CLoader* m_pLoader = nullptr;
    CUI_Text* m_pPercentText = nullptr; // 진행률 표시 텍스트

    float m_fProgress = 0.f;
    float m_fMaxWidth = 600.f;
    float m_fHeight = 20.f;
    float m_fCurrentWidth = 0.f;

    float m_fAnchorLeftX = 0.f; // ← 고정된 왼쪽 기준점
    float m_fAnchorY = 0.f; // ← Y 고정

public:
    static CUI_LoadingBar* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr);
    virtual void Free() override;
};

