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
	/* �� ������ �ʱ�ȭ�۾��� ���� */
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
	/* �� ������ �ݺ����� ����  */
	m_pGameInstance->Update_Engine(fTimeDelta);
}

HRESULT CMainApp::Render()
{
	/* �� ������ �ݺ����� ����  */
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

	/* ���ξ��� �Ϻ��� ����ϱ� ���� ��Ÿ �ʱ�ȭ�۾��� �����Ѵ�. */
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

	/* �� ����� �����Ѵ�. */
	Safe_Release(m_pGameInstance);
}
