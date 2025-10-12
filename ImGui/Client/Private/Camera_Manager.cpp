#include "pch.h"
#include "Camera_Manager.h"
#include "GameInstance.h"
#include "Camera_Dynamic.h"


IMPLEMENT_SINGLETON(CCamera_Manager)

CCamera_Manager::CCamera_Manager()
{
}

HRESULT CCamera_Manager::Add_Camera(CAM_MODE eMode)
{
	CGameInstance* pGameInstance = CGameInstance::GetInstance();
	pGameInstance->AddRef();
	
	switch (eMode)
	{
	case DYNAMIC:
		if (FAILED(pGameInstance->Add_GameObject(
			TEXT("Prototype_GameObject_Camera_Dynamic"),
			LEVEL_GAMEPLAY, TEXT("Layer_Camera"), nullptr)))
		{
			pGameInstance->Release();
			return E_FAIL;
		}
		break;
	}
	pGameInstance->Release();
	return S_OK;
}

void CCamera_Manager::Set_CamMode(CAM_MODE eMode)
{
	CGameInstance* pGameInstance = CGameInstance::GetInstance();

	CCamera* pCamera = dynamic_cast<CCamera*>(pGameInstance->Get_Object(LEVEL_GAMEPLAY, TEXT("Layer_Camera"), eMode));

	if (!pCamera)
		return;

	m_eCamMode = eMode;
	m_pCurrentCamera = pCamera;
}


void CCamera_Manager::Clear_Cameras()
{
	Safe_Release(m_pCurrentCamera);
	m_pCurrentCamera = nullptr;
}

void CCamera_Manager::Free()
{
	Safe_Release(m_pCurrentCamera);
}
