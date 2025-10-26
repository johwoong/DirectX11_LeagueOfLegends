#pragma once

#include "Client_Defines.h"
#include "Base.h"

NS_BEGIN(Engine)
class CGameInstance;
NS_END

NS_BEGIN(Client)
class CLoader final : public CBase
{
private:
	CLoader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CLoader() = default;

public:
	LEVEL Get_LoadingID() const {
		return m_eLoadingLevelID;
	}

	CRITICAL_SECTION Get_CriticalSection() {
		return m_CriticalSection;
	}

	_bool Get_Finished() const {
		return m_isFinished;
	}

	const _tchar* Get_LoadingText() const {
		return m_szLoadingText;
	}

	void Set_Finished(_bool tof) { m_isFinished = tof; }

public:
	HRESULT Initialize(LEVEL eLoadingLevelID);
	HRESULT Loading();

	_bool isFinished() const {
		return m_isFinished;
	}


private:
	ID3D11Device*			m_pDevice = { nullptr };
	ID3D11DeviceContext*	m_pContext = { nullptr };
	LEVEL					m_eLoadingLevelID = { LEVEL::LEVEL_END };
	_tchar					m_szLoadingText[MAX_PATH] = TEXT("");
	_bool					m_isFinished = { false };
	CGameInstance*			m_pGameInstance = { nullptr };

private:
	HANDLE				m_hThread = {};
	CRITICAL_SECTION	m_CriticalSection = {};


private:
	HRESULT Loading_For_Logo();
	HRESULT Loading_For_GamePlay();

public:
	static CLoader* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eLoadingLevelID);
	virtual void Free() override;

};

NS_END