#pragma once

#include "Client_Defines.h"
#include "Base.h"

NS_BEGIN(Client)

class CLoader final : public CBase
{
private:
	CLoader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CLoader() = default;

public:
	HRESULT Initialize(LEVEL eLoadingLevelID);
	HRESULT Loading();

	_bool isFinished() const {
		return m_isFinished;
	}

	void Output() {
		SetWindowText(g_hWnd, m_szFPS);
	}

private:
	ID3D11Device*			m_pDevice = { nullptr };
	ID3D11DeviceContext*	m_pContext = { nullptr };
	LEVEL					m_eLoadingLevelID = { LEVEL::END };

private:
	HANDLE				m_hThread = {};
	CRITICAL_SECTION	m_CriticalSection = {};

	_bool				m_isFinished = { false };
	_tchar				m_szFPS[MAX_PATH] = {};

private:
	HRESULT Loading_For_Logo();
	HRESULT Loading_For_GamePlay();

public:
	static CLoader* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eLoadingLevelID);
	virtual void Free() override;

};

NS_END