#include "Level_Loading.h"

#include "Loader.h"
#include "Level_Logo.h"
#include "Level_GamePlay.h"

#include "GameInstance.h"

CLevel_Loading::CLevel_Loading(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CLevel { pDevice, pContext } 
{
}

HRESULT CLevel_Loading::Initialize(LEVEL eNextLevelID)
{
	if (FAILED(__super::Initialize()))
		return E_FAIL;

	m_eNextLevelID = eNextLevelID;

	/* 자원을 준비하는 동안 로딩 장면을 렌더링 해준다.(메인스레드) */
	if (FAILED(Ready_Layer_UI()))
		return E_FAIL;

	/* 다음레벨에 대한 자원들을 준비한다.(추가스레드) */
	m_pLoader = CLoader::Create(m_pDevice, m_pContext, m_eNextLevelID);
	if (nullptr == m_pLoader)
		return E_FAIL;

    return S_OK;
}

void CLevel_Loading::Update(_float fTimeDelta)
{
	if (true == m_pLoader->isFinished())
	{
		CLevel* pNewLevel = { nullptr };

		switch (m_eNextLevelID)
		{
			case LEVEL::LEVEL_LOGO:
				pNewLevel = CLevel_Logo::Create(m_pDevice, m_pContext);
				break;
			case LEVEL::LEVEL_GAMEPLAY:
				pNewLevel = CLevel_GamePlay::Create(m_pDevice, m_pContext);
				break;
		}

		if (nullptr == pNewLevel)
			return;


		if(FAILED(m_pGameInstance->Open_Level(m_eNextLevelID, pNewLevel)))
			return;

	}	
}

void CLevel_Loading::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);
	SetWindowText(g_hWnd, TEXT("로딩"));
}

HRESULT CLevel_Loading::Ready_Layer_UI()
{
	return S_OK;
}

CLevel_Loading* CLevel_Loading::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eNextLevelID)
{
	CLevel_Loading* pInstance = new CLevel_Loading(pDevice, pContext);

	if (FAILED(pInstance->Initialize(eNextLevelID)))
	{
		MSG_BOX("Failed to Created : CLevel_Loading");
		Safe_Release(pInstance);
	}
	return pInstance;
}


void CLevel_Loading::Free()
{
    __super::Free();

	Safe_Release(m_pLoader);

}
