#include "pch.h"
#include "MainApp.h"

#include "GameInstance.h"
#include "Level_Loading.h"
#include "Imgui_Manager.h"
#include "PickingMgr.h"

CMainApp::CMainApp()
	: m_pGameInstance(CGameInstance::GetInstance())
{
	Safe_AddRef(m_pGameInstance);
}

HRESULT CMainApp::Initialize()
{
	GRAPHIC_DESC	Graphic_Desc;
	ZeroMemory(&Graphic_Desc, sizeof(GRAPHIC_DESC));

	Graphic_Desc.hWnd = g_hWnd;
	Graphic_Desc.iWinSizeX = g_WinSizeX;
	Graphic_Desc.iWinSizeY = g_WinSizeY;
	Graphic_Desc.eWindowMode = GRAPHIC_DESC::MODE_WIN;

	if (FAILED(m_pGameInstance->Initialize_Engine(g_hInst, LEVEL_END, Graphic_Desc, &m_pDevice, &m_pContext)))
		return E_FAIL;

	if(FAILED(CImGui_Manager::GetInstance()->Initialize(m_pDevice, m_pContext)))
		return E_FAIL;

	if (Ready_Prototype_Component())
		return E_FAIL;

	if(FAILED(Open_Level(LEVEL_LOGO)))
		return E_FAIL;
	
	

	return S_OK;
}

void CMainApp::Update(_float fTimeDelta)
{
	if (nullptr == m_pGameInstance)
		return;

	m_pGameInstance->Update_Engine(fTimeDelta);


#ifdef _DEBUG
	// 누적 시간 계산 (1초당 FPS 갱신)
	m_fTimeAcc += fTimeDelta;
	m_iNumRender++;

	if (m_fTimeAcc >= 1.0f)
	{
		swprintf_s(m_szFPS, L"FPS : %d", m_iNumRender);
		SetWindowText(g_hWnd, m_szFPS);

		m_fTimeAcc = 0.f;
		m_iNumRender = 0;
	}
#endif

}

HRESULT CMainApp::Render()
{
	m_pGameInstance->Clear_BackBuffer_View(_float4(0.f, 0.f, 1.f, 1.f));
	m_pGameInstance->Clear_DepthStencil_View();

	m_pRenderer->Render_GameObjects();


	// GamePlay 레벨일 때만 IMGUI 렌더
	if (CGameInstance::GetInstance()->Get_CurrentLevelIndex() == LEVEL_GAMEPLAY)
	{
		CImGui_Manager::GetInstance()->Update(0.1f);
		CImGui_Manager::GetInstance()->Render();
	}

	m_pGameInstance->Present();
	return S_OK;
}

HRESULT CMainApp::Open_Level(LEVEL eLevel)
{
	if (nullptr == m_pGameInstance)
		return E_FAIL;

	CLevel_Loading* pLevelLoading = CLevel_Loading::Create(m_pDevice, m_pContext, eLevel);
	if (nullptr == pLevelLoading)
		return E_FAIL;

	m_pGameInstance->Open_Level(LEVEL_LOADING, pLevelLoading);


	return S_OK;
}

HRESULT CMainApp::Ready_Prototype_Component()
{
	if (nullptr == m_pGameInstance)
		return E_FAIL;

	/* For.Prototype_Component_Renderer */
 	if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), m_pRenderer = CRenderer::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/* For.Prototype_Component_VIBuffer_Rect */
	if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_VIBuffer_Rect"), CVIBuffer_Rect::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/* For.Prototype_Component_Transform */
	if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Transform"), CTransform::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/* For.Prototype_Component_Shader_VtxTex*/
	if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxTex"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../../../Bin/ShaderFiles/Shader_VtxTex.hlsl"), VTXTEX_DECLARATION::Elements, VTXTEX_DECLARATION::iNumElements))))
		return E_FAIL;

 	if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxNorTex"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../../../Bin/ShaderFiles/Shader_NorTexTerrianl.hlsl"), VTXNORTEX_DECLARATION::Elements, VTXNORTEX_DECLARATION::iNumElements))))
		return E_FAIL;

	/*For.Prototype_Component_VIBuffer_Terrain*/
	if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_VIBuffer_Terrain"),
		CVIBuffer_Terrian::Create(m_pDevice, m_pContext, 50, 50, 1))))
		return E_FAIL;

	Safe_AddRef(m_pRenderer);
	return S_OK;
}

CMainApp* CMainApp::Create()
{
	CMainApp* pInstance = new CMainApp();

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CMainApp");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CMainApp::Free()
{
	Safe_Release(m_pRenderer);

	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);

	Safe_Release(m_pGameInstance);

	CGameInstance::Release_Engine();


	CModelManager::GetInstance()->DestroyInstance();
	CPickingMgr::GetInstance()->DestroyInstance();
	CImGui_Manager::GetInstance()->DestroyInstance();
	CTerrianManager::GetInstance()->DestroyInstance();
}
