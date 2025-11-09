#pragma once
#include "UI_Base.h"
class CUI_MiniMap :
    public CUI_Base
{
private:
    CUI_MiniMap(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CUI_MiniMap(const CUI_MiniMap& rhs);
    virtual ~CUI_MiniMap() = default;

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
    static CUI_MiniMap* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr);
    virtual void Free() override;
};

