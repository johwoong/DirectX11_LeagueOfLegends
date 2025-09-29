#include "MainApp.h"
#include "GameInstance.h"

#include "Level_Loading.h"

USING(Client)

CMainApp::CMainApp()
	: m_pGameInstance { CGameInstance::GetInstance()}
{
	Safe_AddRef(m_pGameInstance);
}

HRESULT CMainApp::Initialize()
{
	/* 내 게임의 초기화작업을 수행 */
	ENGINE_DESC		EngineDesc{};
	EngineDesc.hWnd = g_hWnd;
	EngineDesc.eWindowMode = WINMODE::WIN;
	EngineDesc.iWinSizeX = g_iWinSizeX;
	EngineDesc.iWinSizeY = g_iWinSizeY;

	if (FAILED(m_pGameInstance->Initialize_Engine(EngineDesc, &m_pDevice, &m_pContext)))
		return E_FAIL;

	if (FAILED(Start_Level(LEVEL::LOGO)))
		return E_FAIL;


	return S_OK;
}

void CMainApp::Update(_float fTimeDelta)
{
	/* 내 게임의 반복적인 갱신  */
	m_pGameInstance->Update_Engine(fTimeDelta);
}

HRESULT CMainApp::Render()
{
	/* 내 게임의 반복적인 렌더  */
	_float4		vClearColor = _float4(0.f, 0.f, 1.f, 1.f);

	m_pGameInstance->Draw_Begin(&vClearColor);

	m_pGameInstance->Draw();

	m_pGameInstance->Draw_End();

	return S_OK;
}

HRESULT CMainApp::Start_Level(LEVEL eLevelID)
{
	if (FAILED(m_pGameInstance->Change_Level(ENUM_TO_UINT(LEVEL::LOADING), CLevel_Loading::Create(m_pDevice, m_pContext, eLevelID))))
		return E_FAIL;

	return S_OK;
}

CMainApp* CMainApp::Create()
{
	CMainApp* pMainApp = new CMainApp();

	/* 메인앱을 완벽히 사용하기 위한 기타 초기화작업을 수행한다. */
	if (FAILED(pMainApp->Initialize()))
	{
		MSG_BOX("Failed to Created : CMainApp");
		Safe_Release(pMainApp);
	}
	return pMainApp;
}

void CMainApp::Free()
{
	__super::Free();

	/* 내 멤버를 정리한다. */
	Safe_Release(m_pGameInstance);
}
