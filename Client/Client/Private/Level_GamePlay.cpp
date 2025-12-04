
#include "Level_GamePlay.h"
#include "GameInstance.h"
#include "CCamera_Dynamic.h"
#include "PlayerManager.h"
#include "PickingManager.h"
#include "Ezreal.h"
#include "Building.h"
#include "PoolManager.h"
#include "MinionManager.h"
#include "Collision_Manager.h"

CLevel_GamePlay::CLevel_GamePlay(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CLevel { pDevice, pContext } 
{
}

HRESULT CLevel_GamePlay::Initialize()
{
	if (FAILED(__super::Initialize()))
		return E_FAIL;

	if (FAILED(Ready_Lights()))
		return E_FAIL;

	if(FAILED(Ready_Layer_Terrian(TEXT("Layer_BackGround"))))
		return E_FAIL;
	if (FAILED(Ready_Layer_MapObject(TEXT("Layer_MapObject"))))
		return E_FAIL;
	if (FAILED(Ready_Layer_Building(TEXT("Layer_Building"))))
		return E_FAIL;
	if (FAILED(Ready_Layer_Player(TEXT("Layer_Player"))))
		return E_FAIL;
	if (FAILED(Ready_Layer_Enemy(TEXT("Layer_Enemy"))))
		return E_FAIL;
	if (FAILED(Ready_Layer_Camera(TEXT("Layer_Camera"))))
		return E_FAIL;
	if (FAILED(Ready_Layer_UI(TEXT("Layer_UI"))))
		return E_FAIL;

	CPlayerManager* pPM = CPlayerManager::GetInstance();
	CChampion* pActiveChampion = dynamic_cast<CChampion*>(pPM->Get_ActivePlayer());
	if (!pActiveChampion)
		return E_FAIL;

	CGameObject* pObj = nullptr;

	if (FAILED(m_pGameInstance->Add_GameObject_Out(
		TEXT("Prototype_GameObject_UI_AttackRange"), // 새 프로토타입
		LEVEL_GAMEPLAY,
		TEXT("Layer_Effect"),                     // UI말고 이펙트 레이어 추천
		pObj,
		pActiveChampion)))                        // owner로 챔피언 넘김
	{
		return E_FAIL;
	}

	CAttackRange* pRange = dynamic_cast<CAttackRange*>(pObj);
	if (pRange)
	{
		pRange->Set_Owner(pActiveChampion);
		pActiveChampion->Set_AttackRangeObj(pRange);
	}



	// ★ 미니언 풀 재사용 또는 새로 생성
	// 블루 미니언 레이어가 풀에 없으면 -> 처음 진입한 거라고 보고 둘 다 생성
	if (CPoolManager::GetInstance()->Reuse_Pooling_Layer(LEVEL_GAMEPLAY, TEXT("Layer_Minion_Blue_Melee")) == false)
	{
		if (FAILED(Ready_Layer_Minion()))
			return E_FAIL;
	}


	// ★ MinionManager 초기화 (이 레벨에서 운영)
	if (FAILED(CMinionManager::GetInstance()->Initialize(LEVEL_GAMEPLAY)))
		return E_FAIL;

    return S_OK;
}

void CLevel_GamePlay::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);
	// ★ 미니언 웨이브/스폰 처리
	CMinionManager::GetInstance()->Update(fTimeDelta);
	CChampion* pPlayer = CPlayerManager::GetInstance()->Get_ActivePlayer();
	if (!pPlayer)
		return;
	if (GetAsyncKeyState(VK_F4) & 0x0001)
		CPlayerManager::GetInstance()->ChangeChampion();
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

HRESULT CLevel_GamePlay::Ready_Lights()
{
	m_pGameInstance->Clear_AllLight();
	LIGHTDESC LightDesc;
	LightDesc.eTYPE = LIGHTDESC::TYPE_DIRECTIONAL;
	LightDesc.vDirection = _float4(1.f, -1.f, 1.f, 0.f);
	LightDesc.vDiffuse = _float4(1.f, 1.f, 1.f, 1.f);
	LightDesc.vAmbient = _float4(1.f, 1.f, 1.f, 1.f);
	LightDesc.vSpecular = _float4(1.f, 1.f, 1.f, 1.f);

	if (FAILED(m_pGameInstance->Add_Light(m_pDevice, m_pContext, LightDesc)))
		return E_FAIL;

	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_MapObject(const _tchar* pLayerTag)
{
	HANDLE hFile = 0;
	_ulong dwByte = 0;
	NONANIMDESC ModelDesc;
	_uint iNum = 0;

	hFile = CreateFile(TEXT("../../../Bin/Data/Summoners'LiftData/Map.dat"), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
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
		else if (!wcscmp(pModelTag, TEXT("Npc_StoreKeeper_NonAnim")))
		{
			BuildDesc.eTeamType = CBuilding::BLUE;
			if (FAILED(m_pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_Npc"), LEVEL_GAMEPLAY, pLayerTag, &BuildDesc)))
				return E_FAIL;
		}
		//else if (!wcscmp(pModelTag, TEXT("Npc_Store_NonAnim")))
		//{
		//	BuildDesc.eTeamType = CBuilding::RED;
		//	if (FAILED(m_pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_Npc"), LEVEL_GAMEPLAY, pLayerTag, &BuildDesc)))
		//		return E_FAIL;
		//}
	}

	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Player(const _tchar* pLayerTag)
{
	auto pManager = CPlayerManager::GetInstance();
	pManager->Initialize(m_pDevice, m_pContext);
	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Enemy(const _tchar* pLayerTag)
{
	if (FAILED(m_pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_Vayne"), LEVEL_GAMEPLAY, pLayerTag, nullptr)))
		return E_FAIL;
	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Camera(const _tchar* pLayerTag)
{	
	CCamera_Dynamic::CAMERADESC_DERIVED CameraDesc;
	ZeroMemory(&CameraDesc, sizeof(CCamera_Dynamic::CAMERADESC_DERIVED));

	CameraDesc.InitPostion = _float4(0.f, 10.f, -10.f, 1.f);

	CameraDesc.CameraDesc.vEye = _float4(0.f, 10.f, -10.f, 1.f);
	CameraDesc.CameraDesc.vAt = _float4(0.f, 0.f, 0.f, 1.f);
	CameraDesc.CameraDesc.fFov = XMConvertToRadians(60.0f);
	CameraDesc.CameraDesc.fAspect = (_float)g_WinSizeX / g_WinSizeY;
	CameraDesc.CameraDesc.fNear = 0.1f;
	CameraDesc.CameraDesc.fFar = 1000.f;
	CameraDesc.CameraDesc.TransformDesc.fSpeedPerSec = 20.f;
	CameraDesc.CameraDesc.TransformDesc.fRotationPerSec = XMConvertToRadians(60.f);

	CCamera_Dynamic* pCamera = nullptr;
	if (FAILED(m_pGameInstance->Add_GameObject_Out(
		TEXT("Prototype_GameObject_Camera_Dynamic"),
		LEVEL_GAMEPLAY, pLayerTag, (CGameObject*&)pCamera, &CameraDesc)))
	{
		return E_FAIL;
	}

	if (pCamera)
	{
		CChampion* pPlayer = CPlayerManager::GetInstance()->Get_ActivePlayer();
		if (pPlayer)
		{
			pCamera->Set_Target(pPlayer);
			pCamera->Set_CamMode(CCamera_Dynamic::CAM_GAME);
		}
	}

	return S_OK;
	
}

HRESULT CLevel_GamePlay::Ready_Layer_UI(const _tchar* pLayerTag)
{
	if (FAILED(m_pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_UI_SkillMenu"), LEVEL_GAMEPLAY, pLayerTag)))
		return E_FAIL;
	if (FAILED(m_pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_UI_ExpBar"), LEVEL_GAMEPLAY, pLayerTag)))
		return E_FAIL;
	if (FAILED(m_pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_UI_MiniMap"), LEVEL_GAMEPLAY, pLayerTag)))
		return E_FAIL;
	if (FAILED(m_pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_UI_GameInfo"), LEVEL_GAMEPLAY, pLayerTag)))
		return E_FAIL;
	if (FAILED(m_pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_UI_ItemInfo"), LEVEL_GAMEPLAY, pLayerTag)))
		return E_FAIL;


	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Minion()
{
	constexpr int MELEE_POOL_COUNT = 30;
	constexpr int CASTER_POOL_COUNT = 30;

	// 1) 블루 근접 미니언 풀
	{
		const _tchar* pLayerTag = TEXT("Layer_Minion_Blue_Melee");
		MINION_SPAWNDESC desc{};
		desc.eTeam = TEAM_BLUE;
		desc.eLane = LANE_MID;        // 초기값 의미 없음
		desc.eMinionType = MINION_MELEE;
		desc.vSpawnPos = _float3(0.f, 0.f, 0.f);

		for (int i = 0; i < MELEE_POOL_COUNT; ++i)
		{
			if (FAILED(m_pGameInstance->Add_GameObject(
				TEXT("Prototype_GameObject_Minion_Melee"),
				LEVEL_GAMEPLAY, pLayerTag, &desc)))
			{
				return E_FAIL;
			}
		}

		CPoolManager::GetInstance()->Add_Pooling_Layer(LEVEL_GAMEPLAY, pLayerTag);
	}

	// 2) 레드 근접 미니언 풀
	{
		const _tchar* pLayerTag = TEXT("Layer_Minion_Red_Melee");
		MINION_SPAWNDESC desc{};
		desc.eTeam = TEAM_RED;
		desc.eLane = LANE_MID;
		desc.eMinionType = MINION_MELEE;
		desc.vSpawnPos = _float3(0.f, 0.f, 0.f);

		for (int i = 0; i < MELEE_POOL_COUNT; ++i)
		{
			if (FAILED(m_pGameInstance->Add_GameObject(
				TEXT("Prototype_GameObject_Minion_Melee"),
				LEVEL_GAMEPLAY, pLayerTag, &desc)))
			{
				return E_FAIL;
			}
		}

		CPoolManager::GetInstance()->Add_Pooling_Layer(LEVEL_GAMEPLAY, pLayerTag);
	}

	// 3) 블루 원거리 미니언 풀
	{
		const _tchar* pLayerTag = TEXT("Layer_Minion_Blue_Range");
		MINION_SPAWNDESC desc{};
		desc.eTeam = TEAM_BLUE;
		desc.eLane = LANE_MID;
		desc.eMinionType = MINION_CASTER;
		desc.vSpawnPos = _float3(0.f, 0.f, 0.f);

		for (int i = 0; i < CASTER_POOL_COUNT; ++i)
		{
			if (FAILED(m_pGameInstance->Add_GameObject(
				TEXT("Prototype_GameObject_Minion_Range"), // ← 캐스터 프로토타입 이름 맞게
				LEVEL_GAMEPLAY, pLayerTag, &desc)))
			{
				return E_FAIL;
			}
		}

		CPoolManager::GetInstance()->Add_Pooling_Layer(LEVEL_GAMEPLAY, pLayerTag);
	}

	// 4) 레드 원거리 미니언 풀
	{
		const _tchar* pLayerTag = TEXT("Layer_Minion_Red_Range");
		MINION_SPAWNDESC desc{};
		desc.eTeam = TEAM_RED;
		desc.eLane = LANE_MID;
		desc.eMinionType = MINION_CASTER;
		desc.vSpawnPos = _float3(0.f, 0.f, 0.f);

		for (int i = 0; i < CASTER_POOL_COUNT; ++i)
		{
			if (FAILED(m_pGameInstance->Add_GameObject(
				TEXT("Prototype_GameObject_Minion_Range"), // ← 이 이름도 네 프로젝트에 맞게
				LEVEL_GAMEPLAY, pLayerTag, &desc)))
			{
				return E_FAIL;
			}
		}

		CPoolManager::GetInstance()->Add_Pooling_Layer(LEVEL_GAMEPLAY, pLayerTag);
	}

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
	CCollision_Manager::GetInstance()->Clear_AllCollisionGroup();
	CMinionManager::GetInstance()->DestroyInstance();
}
