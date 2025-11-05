#pragma once
#include "UI_Base.h"
class CUI_ChampionImage final : public CUI_Base
{
public:
	enum CHAMPION
	{
		EZREAL, RENEKTON, CHAMPION_END
	};

private:
	CUI_ChampionImage(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CUI_ChampionImage(const CUI_ChampionImage& rhs);
	virtual ~CUI_ChampionImage() = default;

public:
	void Set_ChampionIdx(CHAMPION eChampion) { m_eChampion = eChampion; }

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual int Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);
	virtual HRESULT Render();

private:
	CHAMPION m_eChampion = EZREAL;
	CTexture* m_pFrameTextureCom = { nullptr };

private:
	_float2 m_fFrameSize = { 655.f, 620.f };  // 프레임 전용 크기

private:
	virtual HRESULT Ready_Components(void* pArg) override;
	HRESULT SetUp_ShaderResources(); /* 셰이더 전역변수에 값을 전달한다. */

public:
	static CUI_ChampionImage* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free() override;
};

