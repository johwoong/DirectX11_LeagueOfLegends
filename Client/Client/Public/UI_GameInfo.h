#pragma once
#include "UI_Base.h"
class CUI_GameInfo final : public CUI_Base
{
private:
    CUI_GameInfo(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CUI_GameInfo(const CUI_GameInfo& rhs);
    virtual ~CUI_GameInfo() = default;

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
    static CUI_GameInfo* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr);
    virtual void Free() override;
};

