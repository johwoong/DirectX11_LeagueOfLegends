#include "MainApp.h"
#include "GameInstance.h"

#include "Level_Loading.h"
#include "PickingManager.h"
#include "PlayerManager.h"

USING(Client)

CMainApp::CMainApp()
	: m_pGameInstance { CGameInstance::GetInstance()}
{
	Safe_AddRef(m_pGameInstance);
}

HRESULT CMainApp::Initialize()
{
	if (::AllocConsole() == TRUE)
	{
		FILE* nfp[3];
		freopen_s(nfp + 0, "CONOUT$", "rb", stdin);
		freopen_s(nfp + 1, "CONOUT$", "wb", stdout);
		freopen_s(nfp + 2, "CONOUT$", "wb", stderr);
		std::ios::sync_with_stdio();
	}


	srand(unsigned int(time(NULL)));

	/* 내 게임의 초기화작업을 수행 */
	GRAPHIC_DESC		EngineDesc{};
	EngineDesc.hWnd = g_hWnd;
	EngineDesc.eWindowMode = GRAPHIC_DESC::MODE_WIN;
	EngineDesc.iWinSizeX = g_WinSizeX;
	EngineDesc.iWinSizeY = g_WinSizeY;

	if (FAILED(m_pGameInstance->Initialize_Engine(g_hInst, LEVEL_END, EngineDesc, &m_pDevice, &m_pContext)))
		return E_FAIL;

	if (FAILED(Ready_Prototype_Component()))
		return E_FAIL;

	if (FAILED(Start_Level(LEVEL::LEVEL_LOGO)))
		return E_FAIL;


	return S_OK;
}

void CMainApp::Update(_float fTimeDelta)
{
	if (nullptr == m_pGameInstance)
		return;
	/* 내 게임의 반복적인 갱신  */
	m_pGameInstance->Update_Engine(fTimeDelta);

	//#ifdef _DEBUG
	m_fTimeAcc += fTimeDelta;
	//#endif // _DEBUG
}

HRESULT CMainApp::Render()
{
	m_pGameInstance->Clear_BackBuffer_View(_float4(0.f, 0.f, 1.f, 1.f));
	m_pGameInstance->Clear_DepthStencil_View();

	m_pRenderer->Render_GameObjects();

	++m_iNumRender;
	if (m_fTimeAcc > 1.0f)
	{
		wchar_t szText[128];
		swprintf_s(szText, L"FPS : %d", m_iNumRender);

		SetWindowText(g_hWnd, szText);

		m_fTimeAcc = 0.f;
		m_iNumRender = 0;
	}

	m_pGameInstance->Present();

	return S_OK;
}

HRESULT CMainApp::Start_Level(LEVEL eLevelID)
{
	if (nullptr == m_pGameInstance)
		return E_FAIL;

 	CLevel_Loading* pLevel_Loading = CLevel_Loading::Create(m_pDevice, m_pContext, eLevelID);
	if (nullptr == pLevel_Loading)
		return E_FAIL;

	m_pGameInstance->Open_Level(LEVEL_LOADING, pLevel_Loading);

	return S_OK;
}

HRESULT CMainApp::Ready_Prototype_Component()
{
 	if (nullptr == m_pGameInstance)
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), m_pRenderer = CRenderer::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_VIBuffer_Rect"), CVIBuffer_Rect::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Transform"), CTransform::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxTex"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../../../Bin/ShaderFiles/Shader_VtxTex.hlsl"), VTXTEX_DECLARATION::Elements, VTXTEX_DECLARATION::iNumElements))))
		return E_FAIL;


	if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Shader_UI"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Shaderfiles/Shader_UI.hlsl"), VTXTEX_DECLARATION::Elements, VTXTEX_DECLARATION::iNumElements))))
		return E_FAIL;


	Safe_AddRef(m_pRenderer);

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

	Safe_Release(m_pRenderer);
	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);


	/* 내 멤버를 정리한다. */
	Safe_Release(m_pGameInstance);
	CPickingManager::GetInstance()->DestroyInstance();
	CPlayerManager::GetInstance()->DestroyInstance();
	CGameInstance::Release_Engine();
}
