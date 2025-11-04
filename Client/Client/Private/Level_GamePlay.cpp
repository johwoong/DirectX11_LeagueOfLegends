
#include "Level_GamePlay.h"
#include "GameInstance.h"
#include "CCamera_Dynamic.h"
#include "PlayerManager.h"
#include "PickingManager.h"
#include "Ezreal.h"
#include "Renekton.h"
#include "Building.h"

CLevel_GamePlay::CLevel_GamePlay(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CLevel { pDevice, pContext } 
{
}

HRESULT CLevel_GamePlay::Initialize()
{
	if (FAILED(__super::Initialize()))
		return E_FAIL;


	if (FAILED(Ready_Layer_Camera(TEXT("Layer_Camera"))))
		return E_FAIL;
	if(FAILED(Ready_Layer_Terrian(TEXT("Layer_BackGround"))))
		return E_FAIL;
	if (FAILED(Ready_Layer_MapObject(TEXT("Layer_MapObject"))))
		return E_FAIL;
	if (FAILED(Ready_Layer_Building(TEXT("Layer_Building"))))
		return E_FAIL;
	if (FAILED(Ready_Layer_Player(TEXT("Layer_Player"))))
		return E_FAIL;

    return S_OK;
}

void CLevel_GamePlay::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);

	CChampion* pPlayer = CPlayerManager::GetInstance()->Get_ActivePlayer();
	if (!pPlayer)
		return;
	if (GetAsyncKeyState(VK_F4) & 0x0001)
		CPlayerManager::GetInstance()->ChangeChampion();

	// 마우스 클릭 시 이동 (Terrain 클릭 시만)
	if (m_pGameInstance->Mouse_Down(DIMK_RBUTTON))
	{
		if (CPickingManager::GetInstance()->Picking())
		{
			_float3 vClickPos = CPickingManager::GetInstance()->Get_PickingPos();
			pPlayer->Set_TargetPosition(vClickPos);
		}
	}
}

void CLevel_GamePlay::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);
}

HRESULT CLevel_GamePlay::Ready_Layer_Terrian(const _tchar* pLayerTag)
{
	if (FAILED(m_pGameInstance->Add_GameObjectLoad(TEXT("Prototype_GameObject_Terrian"), LEVEL_GAMEPLAY, pLayerTag, TEXT("Prototype_Summoners_Rift"))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_Sky"), LEVEL_GAMEPLAY, pLayerTag, nullptr)))
		return E_FAIL;

	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_MapObject(const _tchar* pLayerTag)
{
	HANDLE hFile = 0;
	_ulong dwByte = 0;
	NONANIMDESC ModelDesc;
	_uint iNum = 0;

	hFile = CreateFile(TEXT("../../../Bin/Data/Summoners'LiftData/Summoners'LiftMap.dat"), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (0 == hFile)
		return E_FAIL;

	ReadFile(hFile, &(iNum), sizeof(_uint), &dwByte, nullptr);

	for (_uint i = 0; i < iNum; ++i)
	{
		ReadFile(hFile, &(ModelDesc), sizeof(NONANIMDESC), &dwByte, nullptr);
		if (FAILED(m_pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_NonAnim"), LEVEL_GAMEPLAY, pLayerTag, &ModelDesc)))
			return E_FAIL;
	}
	CloseHandle(hFile);
	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Building(const _tchar* pLayerTag)
{
	HANDLE hFile = 0;
	_ulong dwByte = 0;
	CBuilding::BUILDINGDESC BuildDesc;
	_uint iNum = 0;

	hFile = CreateFile(TEXT("../../../Bin/Data/BuildingData/Build.dat"), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (0 == hFile)
		return E_FAIL;

	ReadFile(hFile, & (iNum), sizeof(_uint), &dwByte, nullptr);

	for (_uint i = 0; i < iNum; ++i)
	{
		ReadFile(hFile, &(BuildDesc.ModelDesc), sizeof(NONANIMDESC), &dwByte, nullptr);
		_tchar pModelTag[MAX_PATH];
		MultiByteToWideChar(CP_ACP, 0, BuildDesc.ModelDesc.pModeltag, MAX_PATH, pModelTag, MAX_PATH);

		if (!wcscmp(pModelTag, TEXT("Nexus_Blue_NonAnim")))
		{
			BuildDesc.eTeamType = CBuilding::BLUE;
			if (FAILED(m_pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_Nexus"), LEVEL_GAMEPLAY, pLayerTag, &BuildDesc)))
				return E_FAIL;
		}
		else if (!wcscmp(pModelTag, TEXT("Nexus_Red_NonAnim")))
		{
			BuildDesc.eTeamType = CBuilding::RED;
			if (FAILED(m_pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_Nexus"), LEVEL_GAMEPLAY, pLayerTag, &BuildDesc)))
				return E_FAIL;
		}
		else if (!wcscmp(pModelTag, TEXT("Inhibitor_Blue_NonAnim")))
		{
			BuildDesc.eTeamType = CBuilding::BLUE;
			if (FAILED(m_pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_Inhibitor"), LEVEL_GAMEPLAY, pLayerTag, &BuildDesc)))
				return E_FAIL;
		}
		else if (!wcscmp(pModelTag, TEXT("Inhibitor_Red_NonAnim")))
		{
			BuildDesc.eTeamType = CBuilding::RED;
			if (FAILED(m_pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_Inhibitor"), LEVEL_GAMEPLAY, pLayerTag, &BuildDesc)))
				return E_FAIL;
		}
		else if (!wcscmp(pModelTag, TEXT("Turret_Blue_NonAnim")))
		{
			BuildDesc.eTeamType = CBuilding::BLUE;
			if (FAILED(m_pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_Turret"), LEVEL_GAMEPLAY, pLayerTag, &BuildDesc)))
				return E_FAIL;
		}
		else if (!wcscmp(pModelTag, TEXT("Turret_Red_NonAnim")))
		{
			BuildDesc.eTeamType = CBuilding::RED;
			if (FAILED(m_pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_Turret"), LEVEL_GAMEPLAY, pLayerTag, &BuildDesc)))
				return E_FAIL;
		}
	}



	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Player(const _tchar* pLayerTag)
{
	auto pManager = CPlayerManager::GetInstance();
	pManager->Initialize(m_pDevice, m_pContext);
	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Camera(const _tchar* pLayerTag)
{	
	CCamera_Dynamic::CAMERADESC_DERIVED			CameraDesc;
	ZeroMemory(&CameraDesc, sizeof(CCamera_Dynamic::CAMERADESC_DERIVED));

	CameraDesc.InitPostion = _float4(0.f, 0.f, 0.f, 1.f);
	CameraDesc.vDistance = _float4(0, 10, -10, 0.f);

	CameraDesc.CameraDesc.vEye = _float4(0.f, 10.f, -10.f, 1.f);
	CameraDesc.CameraDesc.vAt = _float4(0.f, 0.f, 0.f, 1.f);

	CameraDesc.CameraDesc.fFov = XMConvertToRadians(60.0f);
	CameraDesc.CameraDesc.fAspect = (_float)g_WinSizeX / g_WinSizeY;
	CameraDesc.CameraDesc.fNear = 0.1f;
	CameraDesc.CameraDesc.fFar = 1000.f;

	CameraDesc.CameraDesc.TransformDesc.fSpeedPerSec = 20.f;
	CameraDesc.CameraDesc.TransformDesc.fRotationPerSec = XMConvertToRadians(60.f);

	if (FAILED(m_pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_Camera_Dynamic"), LEVEL_GAMEPLAY, pLayerTag, &CameraDesc)))
		return E_FAIL;


	return S_OK;
	
}



CLevel_GamePlay* CLevel_GamePlay::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CLevel_GamePlay* pInstance = new CLevel_GamePlay(pDevice, pContext);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevel_GamePlay");
		Safe_Release(pInstance);
	}
	return pInstance;
}


void CLevel_GamePlay::Free()
{
    __super::Free();
}
