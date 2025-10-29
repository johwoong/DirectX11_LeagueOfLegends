
#include "Loader.h"

#include "GameInstance.h"

// UI
#include "UI_BackGround.h"

#include "Terrian.h"
#include "CCamera_Dynamic.h"


CLoader::CLoader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pGameInstnace(CGameInstance::GetInstance())
	, m_pDevice { pDevice }
	, m_pContext { pContext }
{
	Safe_AddRef(m_pGameInstnace);
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

unsigned int APIENTRY ThreadMain(void* pArg)
{
	CLoader* pLoader = static_cast<CLoader*>(pArg);

	
	EnterCriticalSection(&pLoader->Get_CriticalSection());

	switch (pLoader->Get_NextLevelID())
	{
	case LEVEL_LOGO:
		pLoader->Loading_For_Logo();
		break;
	case LEVEL_GAMEPLAY:
		pLoader->Loading_For_GamePlay();
		break;
	}

 	LeaveCriticalSection(&pLoader->Get_CriticalSection());

	return 0;
}

HRESULT CLoader::Initialize(LEVEL eNextLevel)
{
	CoInitializeEx(nullptr, 0);

	m_eNextLevel = eNextLevel;

	InitializeCriticalSection(&m_CriticalSection);

	m_hThread = (HANDLE)_beginthreadex(nullptr, 0, ThreadMain, this, 0, nullptr);
	if (0 == m_hThread)
		return E_FAIL;

	return S_OK;
}


HRESULT CLoader::Loading_For_Logo()
{	
 	if (m_pGameInstnace == nullptr)
		return E_FAIL;

	lstrcpy(m_szLoadingText, TEXT("텍스쳐 로딩 중 "));


	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_LOGO, TEXT("Prototype_Component_Texture_Logo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Logo/BackGround.dds")))))
		return E_FAIL;


	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_UI_BackGround_Logo"),
		CUI_BackGround::Create(m_pDevice, m_pContext))))
		return E_FAIL;


	lstrcpy(m_szLoadingText, TEXT("로딩이 완료되었슴니다."));
	m_isFinished = true;
	return S_OK;
}

HRESULT CLoader::Loading_For_GamePlay()
{
	lstrcpy(m_szLoadingText, TEXT("로딩이 완료되었슴니다."));

#pragma region 셰이더 로딩
	lstrcpy(m_szLoadingText, TEXT("원본 셰이더 로딩 중"));
	// For Prototype Component Shader VtxNorTex
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxNorTex"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../../../Bin/ShaderFiles/Shader_VtxNorTex.hlsl"), VTXNORTEX_DECLARATION::Elements, VTXNORTEX_DECLARATION::iNumElements))))
		return E_FAIL;
#pragma region 셰이더 로딩



#pragma region 원본객체 로딩
	lstrcpy(m_szLoadingText, TEXT("원본 객체 로딩 중"));
	// For Prototype Object Terrian 
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Terrian"),
		CTerrian::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	// For Prototype Object CameraDynamic 
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Camera_Dynamic"),
		CCamera_Dynamic::Create(m_pDevice, m_pContext))))
		return E_FAIL;

#pragma endregion

#pragma region

	lstrcpy(m_szLoadingText, TEXT("버퍼 로딩 중"));

	HANDLE hFile = 0;
	_ulong dwByte = 0;
	_uint iNum = 0;
	hFile = CreateFile(TEXT("../../../Bin/Bin_Data/NonAnim/Map/MapTest.dat"), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	ReadFile(hFile, &(iNum), sizeof(_uint), &dwByte, nullptr);
	if(FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Summoners_Rift"), CVIBuffer_Terrian::Create(m_pDevice, m_pContext, hFile, dwByte))))
		return E_FAIL;

#pragma endregion 

	m_isFinished = true;

	return S_OK;
}

CLoader* CLoader::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eLoadingLevelID)
{
	CLoader* pInstance = new CLoader(pDevice, pContext);

	if (FAILED(pInstance->Initialize(eLoadingLevelID)))
	{
		MSG_BOX("Failed to Created : CLoader");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CLoader::Free()
{
	WaitForSingleObject(m_hThread, INFINITE);

	CloseHandle(m_hThread);

	DeleteCriticalSection(&m_CriticalSection);

	Safe_Release(m_pGameInstnace);



}
