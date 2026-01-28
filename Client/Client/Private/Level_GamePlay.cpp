
#include "Level_GamePlay.h"
#include "GameInstance.h"
#include "CCamera_Dynamic.h"
#include "PlayerManager.h"
#include "PickingManager.h"
#include "Ezreal.h"
#include "Building.h"
#include "PoolManager.h"
#include "MinionManager.h"
#include "Projectile.h"
#include "Collision_Manager.h"
#include "Nexus.h"
#include "GameFlow.h"
#include "UI_TextImage.h"
CLevel_GamePlay::CLevel_GamePlay(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CLevel { pDevice, pContext } 
{
}

HRESULT CLevel_GamePlay::Initialize()
{
	m_bGameOver = false;
	m_bVictoryUISpawned = false;
	m_pRedNexus = nullptr;
	CGameFlow::GetInstance()->Set_GameOver(false);
	// ★ 웰컴(10초) 초기화
	m_fWelcomeAcc = 0.f;
	m_bWelcomePlayed = false;
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


	CAttackRange* pRange = dynamic_cast<CAttackRange*>(pObj);
	if (pRange)
	{
		pRange->Set_Owner(pActiveChampion);
		pActiveChampion->Set_AttackRangeObj(pRange);

		const float fWorldRange = pActiveChampion->Get_WorldAttackRange();
		pRange->Set_RadiusWorld(fWorldRange);
		pRange->Set_Alpha(1.f);
		// 처음엔 안 보이게
		pRange->Set_Visible(false);
	}



	// ★ 미니언 풀 재사용 또는 새로 생성
	// 블루 미니언 레이어가 풀에 없으면 -> 처음 진입한 거라고 보고 둘 다 생성
	if (CPoolManager::GetInstance()->Reuse_Pooling_Layer(LEVEL_GAMEPLAY, TEXT("Layer_Minion_Blue_Melee")) == false)
	{
		if (FAILED(Ready_Layer_Minion()))
			return E_FAIL;
	}
	// 풀에 레이어가 없으면 “풀 생성”만 한다
	if (!CPoolManager::GetInstance()->Find_LayerTag(LEVEL_GAMEPLAY, TEXT("Layer_Projectile")))
	{
		if (FAILED(Ready_Layer_Projectile())) // 내부에서 Add_Pooling_Layer로 씬에서 빼서 풀로 넣기
			return E_FAIL;
	}
	CPoolManager::GetInstance()->Reuse_Pooling_Layer(LEVEL_GAMEPLAY, TEXT("Layer_Effects"));

	if (FAILED(CMinionManager::GetInstance()->Initialize(LEVEL_GAMEPLAY)))
		return E_FAIL;
	CGameInstance::GetInstance()->PlayBGM(const_cast<TCHAR*>(L"SummonerBackGroundSound.wav"),g_fSoundVolume);

    return S_OK;
}

void CLevel_GamePlay::Update(_float fTimeDelta)
{
	g_fSoundVolume = 0.2F;
	m_pGameInstance->SetChannelVolume(SOUND_BGM, g_fSoundVolume);
	const float fWorldDT = (m_bGameOver) ? 0.f : fTimeDelta;

	__super::Update(fWorldDT);

	if (!m_bGameOver)
	{
		if (!m_bWelcomePlayed)
		{
			// "정확히 10초"를 원하면 fTimeDelta 사용 (월드 시간 기준)
			m_fWelcomeAcc += fTimeDelta;

			if (m_fWelcomeAcc >= 10.f)
			{
				m_bWelcomePlayed = true;

				CUI_TextImage::TEXTIMAGEDESC ui{};
				ui.eType = CUI_TextImage::TEXT_TYPE::WELCOME_SUMMONERS_RIFT;
				ui.fLifeTime = 2.8f;

				m_pGameInstance->Add_GameObject(
					TEXT("Prototype_GameObject_UI_TextImage"),
					LEVEL_GAMEPLAY, TEXT("Layer_UI"), &ui);

				m_pGameInstance->PlaySounds(const_cast<TCHAR*>(L"SummoersWelCome.wav"), SOUND_SISTER, g_fSoundVolume);
			}
		}

		// 월드가 정상일 때만 시스템 업데이트
		CMinionManager::GetInstance()->Update(fTimeDelta);

		// 넥서스 파괴 체크
		Check_GameOver_RedNexus();

		// 챔피언 교체 (원래대로 유지)
		if (GetAsyncKeyState(VK_F4) & 0x0001)
			CPlayerManager::GetInstance()->ChangeChampion();

		return;
	}

	// ===========================
	// ★ GameOver 상태: 넥서스 죽음 연출만 계속 + UI 1회
	// ===========================
	const float unscaled = CGameFlow::GetInstance()->Get_UnscaledDT();

	if (m_pRedNexus)
		m_pRedNexus->Tick_GameOverSequence(unscaled);

	if (!m_bVictoryUISpawned)
	{
		m_bVictoryUISpawned = true;
		m_pGameInstance->Add_GameObject(
			TEXT("Prototype_GameObject_EndingUI"),
			LEVEL_GAMEPLAY, TEXT("Layer_UI"), nullptr);
	}
}

void CLevel_GamePlay::Late_Update(_float fTimeDelta)
{
	const float fWorldDT = (m_bGameOver) ? 0.f : fTimeDelta;
	__super::Late_Update(fWorldDT);
}

void CLevel_GamePlay::Check_GameOver_RedNexus()
{
	if (m_bGameOver)
		return;

	if (m_pRedNexus && m_pRedNexus->IsDestroyed())
		Start_GameOver_Victory();
}

void CLevel_GamePlay::Start_GameOver_Victory()
{
	m_bGameOver = true;
	CGameFlow::GetInstance()->Set_GameOver(true);
	m_pGameInstance->PlaySounds(const_cast<TCHAR*>(L"Victory.wav"), SOUND_SISTER, g_fSoundVolume);
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

	LIGHTDESC LightDesc{};
	LightDesc.eTYPE = LIGHTDESC::TYPE_DIRECTIONAL;

	// ★ 발 바로 아래로 떨어지게: 수직 하향(Top-Down)
	LightDesc.vDirection = _float4(0.f, -1.f, 0.f, 0.f);

	LightDesc.vDiffuse = _float4(1.1f, 1.1f, 1.1f, 1.f);
	LightDesc.vAmbient = _float4(0.55f, 0.55f, 0.55f, 1.f);
	LightDesc.vSpecular = _float4(0.25f, 0.25f, 0.25f, 1.f);

	if (FAILED(m_pGameInstance->Add_Light(m_pDevice, m_pContext, LightDesc)))
		return E_FAIL;

	_float4 vLightEye = _float4(36.f, 300.f, 200.f, 1.f);
	_float4 vLightAt = _float4(36.f, 0.f, 200.f, 1.f);

	m_pGameInstance->Set_ShadowLightView(vLightEye, vLightAt);
	m_pGameInstance->Set_ShadowLightProj_Ortho(400.f, 400.f, 1.f, 1000.f);

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

			// ★ 레드 넥서스 포인터 확보 필수
			CGameObject* pObj = nullptr;
			if (FAILED(m_pGameInstance->Add_GameObject_Out(
				TEXT("Prototype_GameObject_Nexus"),
				LEVEL_GAMEPLAY, pLayerTag, pObj, &BuildDesc)))
				return E_FAIL;

			m_pRedNexus = dynamic_cast<CNexus*>(pObj);
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

HRESULT CLevel_GamePlay::Ready_Layer_Enemy(const _tchar* pLayerTag)
{
	HANDLE hFile = 0;
	_ulong dwByte = 0;
	NONANIMDESC ModelDesc;
	_uint iNum = 0;
	hFile = CreateFile(TEXT("../../../Bin/Data/PositionData/EnemySpawnPos.dat"), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (0 == hFile)
		return E_FAIL;

	ReadFile(hFile, &(iNum), sizeof(_uint), &dwByte, nullptr);

	for (_uint i = 0; i < iNum; ++i)
	{
		ReadFile(hFile, &(ModelDesc), sizeof(NONANIMDESC), &dwByte, nullptr);
		_tchar pModeltag[MAX_PATH];
		MultiByteToWideChar(CP_ACP, 0, ModelDesc.pModeltag, MAX_PATH, pModeltag, MAX_PATH);

		if (!wcscmp(pModeltag, TEXT("Vayne_NonAnim")))
		{
			if (FAILED(m_pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_Vayne"), LEVEL_STATIC, TEXT("Layer_Enemy"), &ModelDesc)))
				return E_FAIL;
		}
		if (!wcscmp(pModeltag, TEXT("Renekton_NonAnim")))
		{
			if (FAILED(m_pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_Renekton"), LEVEL_STATIC, TEXT("Layer_Enemy"), &ModelDesc)))
				return E_FAIL;
		}
		if (!wcscmp(pModeltag, TEXT("Leona_NonAnim")))
		{
			if (FAILED(m_pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_Leona"), LEVEL_STATIC, TEXT("Layer_Enemy"), &ModelDesc)))
				return E_FAIL;
		}
	}

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

HRESULT CLevel_GamePlay::Ready_Layer_Projectile()
{
	constexpr int PROJ_POOL_COUNT = 200;

	CProjectile::PROJECTILEDESC d{};
	d.vInitPositon = XMVectorSet(0.f, 0.f, 0.f, 1.f);
	d.vTargetDir = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	d.fVelocity = 15.f;
	d.fDeadTime = 1.2f;
	d.iDamage = 0;
	d.pOwner = nullptr;
	d.pTarget = nullptr;
	d.ePoolLevel = LEVEL_GAMEPLAY;
	d.pPoolLayerTag = TEXT("Layer_Projectile");

	d.bPrewarm = true;   

	for (int i = 0; i < PROJ_POOL_COUNT; ++i)
	{
		if (FAILED(m_pGameInstance->Add_GameObject(
			TEXT("Prototype_GameObject_EzrealProjectile"),
			LEVEL_GAMEPLAY, TEXT("Layer_Projectile"), &d)))
			return E_FAIL;
	}

	CPoolManager::GetInstance()->Add_Pooling_Layer(LEVEL_GAMEPLAY, TEXT("Layer_Projectile"));
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
