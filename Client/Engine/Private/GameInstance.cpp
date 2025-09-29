#include "GameInstance.h"

#include "Graphic_Device.h"
#include "Timer_Manager.h"
#include "Level_Manager.h"

IMPLEMENT_SINGLETON(CGameInstance)

CGameInstance::CGameInstance()
{

}
#pragma region Engine
HRESULT CGameInstance::Initialize_Engine(const ENGINE_DESC& EngineDesc, ID3D11Device** ppDevice, ID3D11DeviceContext** ppContext)
{
	/* 그래픽 디바이스 초기화 */
	m_pGraphic_Device = CGraphic_Device::Create(EngineDesc.hWnd, EngineDesc.eWindowMode, EngineDesc.iWinSizeX, EngineDesc.iWinSizeY, ppDevice, ppContext);
	if (nullptr == m_pGraphic_Device)
		return E_FAIL;

	m_pLevel_Manager = CLevel_Manager::Create();
	if (nullptr == m_pLevel_Manager)
		return E_FAIL;

	/* 인풋 디바이스 초기화 */
	/* 사운드  디바이스 초기화 */
	/* 타이머 매니져 초기화 */
	m_pTimer_Manager = CTimer_Manager::Create();
	if (nullptr == m_pTimer_Manager)
		return FALSE;
	/* 프로토타입매니져  초기화 */
	/* 오브젝트 매니져 초기화 */

	return S_OK;
}
void CGameInstance::Update_Engine(_float fTimeDelta)
{

	m_pLevel_Manager->Update(fTimeDelta);
}

HRESULT CGameInstance::Draw_Begin(const _float4* pClearColor)
{		
	if (FAILED(m_pGraphic_Device->Clear_BackBuffer_View(pClearColor)))
		return E_FAIL;

	if (FAILED(m_pGraphic_Device->Clear_DepthStencil_View()))
		return E_FAIL;

	return S_OK;
}
HRESULT CGameInstance::Draw()
{
	// m_pRenderer->Draw();

	m_pLevel_Manager->Render();

	return S_OK;
}
HRESULT CGameInstance::Draw_End()
{
	m_pGraphic_Device->Present();

	return S_OK;
}
HRESULT CGameInstance::Clear(_uint iLevelID)
{
	/* iLevelID용 자원을 삭제하낟. */


	return S_OK;
}
#pragma endregion

#pragma region TIMER_MANAGER
_float CGameInstance::Get_TimeDelta(const _tchar* pTimerTag)
{
	return m_pTimer_Manager->Get_TimeDelta(pTimerTag);
}

HRESULT CGameInstance::Add_Timer(const _tchar* pTimerTag)
{
	return m_pTimer_Manager->Add_Timer(pTimerTag);
}

void CGameInstance::Compute_TimeDelta(const _tchar* pTimerTag)
{
	m_pTimer_Manager->Compute_TimeDelta(pTimerTag);
}

#pragma endregion

#pragma region LEVEL_MANAGER

HRESULT CGameInstance::Change_Level(_uint iCurrentLevelID, CLevel* pNewLevel)
{
	return m_pLevel_Manager->Change_Level(iCurrentLevelID, pNewLevel);
}

#pragma endregion

void CGameInstance::Free()
{
	__super::Free();
	

	Safe_Release(m_pTimer_Manager);
	Safe_Release(m_pLevel_Manager);
	Safe_Release(m_pGraphic_Device);
	

}