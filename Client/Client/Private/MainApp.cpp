#include "MainApp.h"
#include "GameInstance.h"

#include "Level_Loading.h"
#include "PickingManager.h"
#include "PlayerManager.h"
#include "PoolManager.h"
#include "CursorManager.h"
#include "Collision_Manager.h"

#include "UI_BackGround.h"
#include "UI_ChampionImage.h"
#include "UI_LoadingBar.h"
#include "UI_Text.h"
#include "Cursor_UI.h"

USING(Client)

CMainApp::CMainApp()
	: m_pGameInstance { CGameInstance::GetInstance()}
	, m_pUIManager(CUI_Manager::GetInstance())
{
	Safe_AddRef(m_pUIManager);
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

	if (FAILED(m_pGameInstance->Add_Fonts(m_pDevice, m_pContext, TEXT("Font_Nexon"), TEXT("../../../Bin/Resources/Fonts/dejavusans.spritefont"))))
		return E_FAIL;

	CCursorManager::GetInstance()->Initialize(g_hWnd);
	m_pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_UI_Cursor"),
		LEVEL_STATIC, TEXT("Layer_UI"));
	return S_OK;
}

void CMainApp::Update(_float fTimeDelta)
{
	if (nullptr == m_pGameInstance)
		return;

	m_pUIManager->Update_UI(fTimeDelta);

	/* 내 게임의 반복적인 갱신  */
	m_pGameInstance->Update_Engine(fTimeDelta);
	CPickingManager::GetInstance()->Picking();
	CCursorManager::GetInstance()->Update();
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


	// 로딩 관련
	if (FAILED(m_pGameInstance->Add_Prototype(TEXT("Prototype_GameObject_UI_BackGround"),
		CUI_BackGround::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(TEXT("Prototype_GameObject_UI_ChampionImage"),
		CUI_ChampionImage::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(TEXT("Prototype_GameObject_UI_LoadingBar"),
		CUI_LoadingBar::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(TEXT("Prototype_GameObject_UI_Text"),
		CUI_Text::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(TEXT("Prototype_GameObject_UI_Cursor"),
		CCursor_UI::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Texture_Logo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/Logo/BackGround%d.dds"), 2))))
		return E_FAIL;
 	if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Texture_ChampionImage"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/Loading/ChampionImage/ChampionImage%d.dds"), 10))))
		return E_FAIL;
	if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Texture_Frame"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/Loading/Frame/LoadingFrame.dds")))))
		return E_FAIL;
	if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Texture_Loading_Bar"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/Loading/LoadingBar/LoadingBar.dds")))))
		return E_FAIL;
	if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Texture_Cursor_Normal"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Cursor/hover_precise.png")))))
		return E_FAIL;
	if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Texture_Cursor_Attack"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Cursor/hover_enemy_precise.png")))))
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
	CPoolManager::GetInstance()->Clear_AllPooling_Layer();
	Safe_Release(m_pRenderer);
	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);


	CPoolManager::GetInstance()->DestroyInstance();
	CPlayerManager::GetInstance()->DestroyInstance();
	CPickingManager::GetInstance()->DestroyInstance();
	CCursorManager::GetInstance()->DestroyInstance();
	CUI_Manager::GetInstance()->DestroyInstance();

	Safe_Release(m_pGameInstance);
	/* 내 멤버를 정리한다. */
	CGameInstance::Release_Engine();
	Safe_Release(m_pUIManager);
	CCollision_Manager::GetInstance()->DestroyInstance();
}
