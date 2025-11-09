#pragma once
#include "UI_Base.h"
#include "UI_ChampionIcon.h"
class CUI_ExpBar final : public CUI_Base
{
private:
    CUI_ExpBar(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CUI_ExpBar(const CUI_ExpBar& rhs);
    virtual ~CUI_ExpBar() = default;

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
    void Set_ChampionIcon(const wstring& IconTag); // ? 아이콘 변경함수

private:
    CUI_ChampionIcon* m_pChampionIcon = nullptr; // ? 아이콘 UI 보관


public:
    static CUI_ExpBar* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr);
    virtual void Free() override;
};

