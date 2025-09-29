#include "Level_Manager.h"
#include "Level.h"
#include "GameInstance.h"

CLevel_Manager::CLevel_Manager()
	: m_pGameInstance { CGameInstance::GetInstance() }
{
	Safe_AddRef(m_pGameInstance); 	
}

HRESULT CLevel_Manager::Change_Level(_uint iCurrentLevelID, CLevel* pNewLevel)
{
	Safe_Release(m_pCurrentLevel);

	if (nullptr != m_pCurrentLevel)
		m_pGameInstance->Clear(m_iCurrentLevelID);

	m_pCurrentLevel = pNewLevel;	

	m_iCurrentLevelID = iCurrentLevelID;

	return S_OK;
}

void CLevel_Manager::Update(_float fTimeDelta)
{
	m_pCurrentLevel->Update(fTimeDelta);

}

HRESULT CLevel_Manager::Render()
{
	return m_pCurrentLevel->Render();
}

CLevel_Manager* CLevel_Manager::Create()
{
	return new CLevel_Manager();
}

void CLevel_Manager::Free()
{
	__super::Free();

	Safe_Release(m_pGameInstance);
	Safe_Release(m_pCurrentLevel);
}
