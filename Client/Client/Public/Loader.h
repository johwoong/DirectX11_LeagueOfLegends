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
	HRESULT Initialize(LEVEL eNextLevel);
	LEVEL Get_NextLevelID() const {
		return m_eNextLevel;
	}

	CRITICAL_SECTION& Get_CriticalSection() {
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
	_float Get_Progress() const { return m_fProgress; }


public:
	HRESULT Loading_For_Logo();
	HRESULT Loading_For_Prtotype();
	HRESULT Loading_For_GamePlay();

private:
	LEVEL					m_eNextLevel = LEVEL_END;
	_bool					m_isFinished = false;
	_tchar					m_szLoadingText[MAX_PATH] = TEXT("");

private:
	ID3D11Device* m_pDevice				=			{ nullptr };	
	ID3D11DeviceContext* m_pContext		=			{ nullptr };
	CGameInstance* m_pGameInstnace		=			{ nullptr };

private:
	_float m_fProgress = 0.f;

private:
	HANDLE					m_hThread = 0;
	CRITICAL_SECTION		m_CriticalSection;

public:
	static CLoader* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eLoadingLevelID);
	virtual void Free() override;
};

NS_END