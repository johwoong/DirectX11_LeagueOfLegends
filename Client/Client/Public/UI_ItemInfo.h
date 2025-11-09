#pragma once
#include "UI_Base.h"
class CUI_ItemInfo final : public CUI_Base
{
private:
    CUI_ItemInfo(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CUI_ItemInfo(const CUI_ItemInfo& rhs);
    virtual ~CUI_ItemInfo() = default;

public:
    virtual HRESULT Initialize_Prototype();
    virtual HRESULT Initialize(void* pArg);
    virtual int Update(_float fTimeDelta);
    virtual void Late_Update(_float fTimeDelta);
    virtual HRESULT Render();

private:
    virtual HRESULT Ready_Components(void* pArg) override;
    HRESULT SetUp_ShaderResources();


public:
    static CUI_ItemInfo* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr);
    virtual void Free() override;
};

