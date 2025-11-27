#pragma once

#include "Client_Defines.h"
#include "Base.h"
#include "UI_Manager.h"

NS_BEGIN(Engine)
class CGameInstance;
class CRenderer;
NS_END

NS_BEGIN(Client)

class CMainApp final : public CBase
{
private:
	CMainApp();
	virtual ~CMainApp() = default;

public:
	/* 게임 초기화작업 */
	HRESULT Initialize();
	void Update(_float fTimeDelta);
	HRESULT Render();

private:
	ID3D11Device* m_pDevice =				{ nullptr };
	ID3D11DeviceContext* m_pContext =		{ nullptr };
	CGameInstance* m_pGameInstance =		{ nullptr };
	CRenderer* m_pRenderer =				{ nullptr };
	
private:
	CUI_Manager* m_pUIManager	=			{ nullptr };


private:
	_uint					m_iNumRender = 0;
	_tchar					m_szFPS[MAX_PATH] = TEXT("");
	_float					m_fTimeAcc = 0.f;

private:
	HRESULT Start_Level(LEVEL eLevelID);
	HRESULT Ready_Prototype_Component();

public:
	static CMainApp* Create();
	virtual void Free() override;
};

NS_END

