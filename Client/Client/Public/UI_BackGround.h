#pragma once
#include "UI_Base.h"


class UI_BackGround final : public CUI_Base
{
private:
	UI_BackGround(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	UI_BackGround(const UI_BackGround& rhs);
	virtual ~UI_BackGround() = default;


public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual int Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);
	virtual HRESULT Render();

private:
	virtual HRESULT Ready_Components(void* pArg) override;
	HRESULT SetUp_ShaderResources(); 

	CTexture* m_pTextureCom = nullptr;

public:
	static UI_BackGround* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free() override;
};

