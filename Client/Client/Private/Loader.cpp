
#include "Loader.h"

#include "GameInstance.h"

// UI
#include "UI_BackGround.h"

CLoader::CLoader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice { pDevice }
	, m_pContext { pContext }
	, m_pGameInstance(CGameInstance::GetInstance())
{
	Safe_AddRef(m_pGameInstance);
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

unsigned int APIENTRY ThreadMain(void* pArg)
{
	CLoader* pLoader = static_cast<CLoader*>(pArg);

	if (FAILED(pLoader->Loading()))
		return 1;

	return 0;
}

HRESULT CLoader::Initialize(LEVEL eLoadingLevelID)
{
	m_eLoadingLevelID = eLoadingLevelID;

	InitializeCriticalSection(&m_CriticalSection);

	m_hThread = (HANDLE)_beginthreadex(nullptr, 0, ThreadMain, this, 0, nullptr);
	if (0 == m_hThread)
		return E_FAIL;


	return S_OK;
}

HRESULT CLoader::Loading()
{
	EnterCriticalSection(&m_CriticalSection);

	CoInitializeEx(nullptr, 0);

	HRESULT		hr = {};

	switch (m_eLoadingLevelID)
	{
	case LEVEL::LEVEL_LOGO:
		hr = Loading_For_Logo();
		break;
	case LEVEL::LEVEL_GAMEPLAY:
		hr = Loading_For_GamePlay();
		break;
	}

	LeaveCriticalSection(&m_CriticalSection);

	if (FAILED(hr))
		return E_FAIL;



	return S_OK;
}


HRESULT CLoader::Loading_For_Logo()
{	
	if (m_pGameInstance == nullptr)
		return E_FAIL;

	lstrcpy(m_szFPS, TEXT("텍스쳐 로딩 중 "));


	if (FAILED(m_pGameInstance->Add_Prototype(LEVEL_LOGO, TEXT("Prototype_Component_Texture_Logo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Logo/BackGround.dds")))))
		return E_FAIL;


	if (FAILED(m_pGameInstance->Add_Prototype(TEXT("Prototype_GameObject_UI_BackGround_Logo"),
		CUI_BackGround::Create(m_pDevice, m_pContext))))
		return E_FAIL;


	lstrcpy(m_szFPS, TEXT("로딩이 완료되었슴니다."));
	m_isFinished = true;
	return S_OK;
}

HRESULT CLoader::Loading_For_GamePlay()
{
	lstrcpy(m_szFPS, TEXT("로딩이 완료되었슴니다."));
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
	__super::Free();

	WaitForSingleObject(m_hThread, INFINITE);

	DeleteCriticalSection(&m_CriticalSection);

	CloseHandle(m_hThread);

	Safe_Release(m_pGameInstance);
	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);


}
