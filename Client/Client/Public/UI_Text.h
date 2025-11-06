#pragma once
#include "UI_Base.h"

// 텍스트 UI
class CUI_Text final : public CUI_Base
{
public:
    CUI_Text(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CUI_Text(const CUI_Text& rhs);
    virtual ~CUI_Text() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual int Update(_float fTimeDelta) override;
    virtual void Late_Update(_float fTimeDelta) override;
    virtual HRESULT Render() override;

public: // 외부 제어 API
    void Set_Text(const wstring& text) { m_strText = text; }
    void Set_Color(const _float4& color) { m_vColor = color; }
    void Set_Scale(_float scale) { m_fScale = scale; }
    void Set_Position(const _float2& pos) { m_fPosition = pos; }

private:
    HRESULT Ready_Components(void* pArg) override;
    HRESULT SetUp_ShaderResources() override; // 사용 안함 → S_OK

private:
    wstring m_strText = L"";
    _float4 m_vColor = _float4(1.f, 1.f, 1.f, 1.f);
    _float  m_fScale = 1.f;

public:
    static CUI_Text* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr) override;
    virtual void Free() override;
};

