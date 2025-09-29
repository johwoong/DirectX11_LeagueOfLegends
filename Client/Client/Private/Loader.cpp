#include "Loader.h"

CLoader::CLoader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice { pDevice }
	, m_pContext { pContext }
{
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

_uint APIENTRY ThreadMain(void* pArg)
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
	case LEVEL::LOGO:
		hr = Loading_For_Logo();
		break;
	case LEVEL::GAMEPLAY:
		hr = Loading_For_GamePlay();
		break;
	}

	if (FAILED(hr))
		return E_FAIL;

	LeaveCriticalSection(&m_CriticalSection);

	return S_OK;
}

HRESULT CLoader::Loading_For_Logo()
{	
	lstrcpy(m_szFPS, TEXT("텍스쳐를 로딩 중 입니다."));
	for (size_t i = 0; i < 888899999; i++)
	{
		int a = 10;
	}
	lstrcpy(m_szFPS, TEXT("모델을(를) 로딩 중 입니다."));
	for (size_t i = 0; i < 88899999; i++)
	{
		int a = 10;
	}
	lstrcpy(m_szFPS, TEXT("ㅅㅖ이더을(를) 로딩 중 입니다."));
	for (size_t i = 0; i < 88899999; i++)
	{
		int a = 10;
	}
	lstrcpy(m_szFPS, TEXT("객체원형을(를) 로딩 중 입니다."));
	for (size_t i = 0; i < 88899999; i++)
	{
		int a = 10;
	}

	lstrcpy(m_szFPS, TEXT("로딩이 완료되었슴니다."));

	m_isFinished = true;

	return S_OK;
}

HRESULT CLoader::Loading_For_GamePlay()
{
	lstrcpy(m_szFPS, TEXT("텍스쳐를 로딩 중 입니다."));
	for (size_t i = 0; i < 88899999; i++)
	{
		int a = 10;
	}
	lstrcpy(m_szFPS, TEXT("모델을(를) 로딩 중 입니다."));
	for (size_t i = 0; i < 88889999; i++)
	{
		int a = 10;
	}
	lstrcpy(m_szFPS, TEXT("ㅅㅖ이더을(를) 로딩 중 입니다."));
	for (size_t i = 0; i < 88889999; i++)
	{
		int a = 10;
	}
	lstrcpy(m_szFPS, TEXT("객체원형을(를) 로딩 중 입니다."));
	for (size_t i = 0; i < 88899999; i++)
	{
		int a = 10;
	}

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

	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);


}
