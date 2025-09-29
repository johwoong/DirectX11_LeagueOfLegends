#pragma once

/* 1. 여러 매니져 클래스를 들고 있다. */
/* 2. 여러 매니져 클래스의 함수를 호출해 ㅈ눈다. */
/* 3. 내 게임이 끝날때 알아서 매니져 정리한다. */
/* 4. 알아서 업데이틓나다. */
/* 4. 알아서 렌더링한다. */
#include "Base.h"

NS_BEGIN(Engine)

class ENGINE_DLL CGameInstance final : public CBase
{
	DECLARE_SINGLETON(CGameInstance)

private:
	CGameInstance();
	virtual ~CGameInstance() = default;

#pragma region Engine
public:	
	HRESULT Initialize_Engine(const ENGINE_DESC& EngineDesc, ID3D11Device** ppDevice, ID3D11DeviceContext** ppContext);
	void Update_Engine(_float fTimeDelta);
	HRESULT Draw_Begin(const _float4* pClearColor);
	HRESULT Draw();
	HRESULT Draw_End();
	HRESULT Clear(_uint iLevelID); /* 정해진레벨의 자원을 날린다. */
#pragma endregion

	
		

#pragma region TIMER_MANAGER
public:
	_float			Get_TimeDelta(const _tchar* pTimerTag);
	HRESULT			Add_Timer(const _tchar* pTimerTag);
	void			Compute_TimeDelta(const _tchar* pTimerTag);
#pragma endregion

#pragma region LEVEL_MANAGER
	HRESULT Change_Level(_uint iCurrentLevelID, class CLevel* pNewLevel);
#pragma endregion



private:
	class CGraphic_Device*			m_pGraphic_Device = { nullptr };
	class CLevel_Manager*			m_pLevel_Manager = { nullptr };
	class CObject_Manager*			m_pObject_Manager = { nullptr };
	class CTimer_Manager*			m_pTimer_Manager = { nullptr };
	class CRenderer*				m_pRenderer = { nullptr };


public:
	virtual void Free() override;

};

NS_END