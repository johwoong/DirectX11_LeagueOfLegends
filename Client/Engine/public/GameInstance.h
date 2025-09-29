#pragma once

/* 1. ���� �Ŵ��� Ŭ������ ��� �ִ�. */
/* 2. ���� �Ŵ��� Ŭ������ �Լ��� ȣ���� ������. */
/* 3. �� ������ ������ �˾Ƽ� �Ŵ��� �����Ѵ�. */
/* 4. �˾Ƽ� �����̺�����. */
/* 4. �˾Ƽ� �������Ѵ�. */
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
	HRESULT Clear(_uint iLevelID); /* ������������ �ڿ��� ������. */
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