#pragma once
#include "UI_Base.h"
class CUI_ChampionIcon final : public CUI_Base
{
private:
    CUI_ChampionIcon(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CUI_ChampionIcon(const CUI_ChampionIcon& rhs);
    virtual ~CUI_ChampionIcon() = default;

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
    void Set_Icon(const wstring& IconTag);

public:
    static CUI_ChampionIcon* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr);
    virtual void Free() override;
};

