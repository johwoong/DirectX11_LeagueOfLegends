#include "pch.h"
#include "Loader.h"

#include "GameInstance.h"

#include "Camera_Dynamic.h"
#include "BackGround.h"
#include "Imgui_Manager.h"
#include "ModelManager.h"
#include "NonAnim.h"
#include "Model.h"
#include "Anim.h"
#include "Terrian.h"

CLoader::CLoader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice(pDevice)
	, m_pContext(pContext)
{
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

unsigned int APIENTRY Thread_Main(void* pArg)
{
	CLoader*		pLoader = (CLoader*)pArg;

	EnterCriticalSection(&pLoader->Get_CriticalSection());
	
	switch (pLoader->Get_NextLevelID())
	{
	case LEVEL_LOGO:
		pLoader->Loading_ForLogoLevel();
		break;
	case LEVEL_GAMEPLAY:
		pLoader->Loading_ForGamePlayLevel();
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
	
	m_hThread = (HANDLE)_beginthreadex(nullptr, 0, Thread_Main, this, 0, nullptr);
	if (0 == m_hThread)
		return E_FAIL;

	return S_OK;
}

HRESULT CLoader::Loading_ForLogoLevel()
{
	CGameInstance* pGameInstance = CGameInstance::GetInstance();
	if (nullptr == pGameInstance)
		return E_FAIL;

	Safe_AddRef(pGameInstance);

	lstrcpy(m_szLoadingText, TEXT("Loading Level"));
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_LOGO, TEXT("Prototype_Component_Texture_BackGround"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/UI/Logo/BackGround.dds"), 1))))
		return E_FAIL;

	if (FAILED(pGameInstance->Add_Prototype(TEXT("Prototype_GameObject_BackGround"),
		CBackGround::Create(m_pDevice, m_pContext))))
		return E_FAIL;


	lstrcpy(m_szLoadingText, TEXT("Finished Loading"));
	m_isFinished = true;

	Safe_Release(pGameInstance);
	return S_OK;
}

HRESULT CLoader::Loading_ForGamePlayLevel()
{
	CGameInstance* pGameInstance = CGameInstance::GetInstance();
	if (nullptr == pGameInstance)
		return E_FAIL;

	lstrcpy(m_szLoadingText, TEXT("Loading Texture"));

	lstrcpy(m_szLoadingText, TEXT("Loading For Shader"));

	/* For.Prototype_Component_Shader_VtxModel */
 	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Shader_VtxModel"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../../../Bin/ShaderFiles/Shader_VtxModelTool.hlsl"), VTXMODEL_DECLARATION::Elements, VTXMODEL_DECLARATION::iNumElements))))
		return E_FAIL;

	/* For.Prototype_Component_Shader_VtxModel */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Shader_VtxAnimModel"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../../../Bin/ShaderFiles/Shader_VtxAnimModel.hlsl"), VTXANIMMODEL_DECLARATION::Elements, VTXANIMMODEL_DECLARATION::iNumElements))))
		return E_FAIL;

	lstrcpy(m_szLoadingText, TEXT("Loading For Prototype"));

	/*For.Prototype_GameObject_Player*/
	if (FAILED(pGameInstance->Add_Prototype(TEXT("Prototype_GameObject_NonAnim"),
		CNonAnim::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(pGameInstance->Add_Prototype(TEXT("Prototype_GameObject_Anim"),
		CAnim::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(pGameInstance->Add_Prototype(TEXT("Prototype_GameObject_Terrain"),
		CTerrian::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/*For.Prototype_GameObject_Camera_Dynamic */
	if (FAILED(pGameInstance->Add_Prototype(TEXT("Prototype_GameObject_Camera_Dynamic"),
		CCamera_Dynamic::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(pGameInstance->Add_Prototype(
		LEVEL_GAMEPLAY,
		TEXT("Map"),
		CModel::Create(
			m_pDevice, m_pContext,
			CModel::TYPE_NONANIM,
			"../../../Bin/Bin_Data/NonAnim/Map/Map.dat"
		))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Map"));


	if (FAILED(Loading_ForActor()))
		return E_FAIL;

	m_isFinished = true;
	Safe_Release(pGameInstance);
	return S_OK;
}

HRESULT CLoader::Loading_ForActor()
{
	CGameInstance* pGameInstance = CGameInstance::GetInstance();
	CImGui_Manager* pImgui = CImGui_Manager::GetInstance();
	if (nullptr == pGameInstance)
		return E_FAIL;

#pragma region Animation

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Ezreal"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Ezreal/Ezreal.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Ezreal");

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Braum"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Braum/Braum.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Braum");

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Galio"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Galio/Galio.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Galio");
	

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("LeeSin"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Leesin/Leesin.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("LeeSin");

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Renekton"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Renekton/Renekton.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Renekton");

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Yasuo"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Yasuo/Yasuo.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Yasuo");

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Garen"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Garen/Garen.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Garen");

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Gwen"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Gwen/Gwen.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Gwen");

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Leona"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Leona/Leona.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Leona");

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Vi"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Vi/Vi.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Vi");


	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Vayne"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Vayne/Vayne.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Vayne");

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Minion_Cannon_Blue"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Minion/Minion_Cannon_Blue/Minion_Cannon_Blue.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Minion_Cannon_Blue");

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Minion_Cannon_Red"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Minion/Minion_Cannon_Red/Minion_Cannon_Red.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Minion_Cannon_Red");

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Minion_Melee_Blue"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Minion/Minion_Melee_Blue/Minion_Melee_Blue.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Minion_Melee_Blue");

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Minion_Melee_Red"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Minion/Minion_Melee_Red/Minion_Melee_Red.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Minion_Melee_Red");


	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Minion_Range_Blue"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Minion/Minion_Range_Blue/Minion_Range_Blue.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Minion_Range_Blue");

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Minion_Range_Red"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Minion/Minion_Range_Red/Minion_Range_Red.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Minion_Range_Red");

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Minion_Super_Blue"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Minion/Minion_Super_Blue/Minion_Super_Blue.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Minion_Super_Blue");


	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Minion_Super_Red"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Minion/Minion_Super_Red/Minion_Super_Red.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Minion_Super_Red");

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Inhibitor_Blue"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Buildings/Inhibitor_Blue/Inhibitor_Blue.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Inhibitor_Blue");


	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Inhibitor_Red"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Buildings/Inhibitor_Red/Inhibitor_Red.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Inhibitor_Red");


	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Nexus_Blue"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Buildings/Nexus_Blue/Nexus_Blue.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Nexus_Blue");


	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Nexus_Red"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Buildings/Nexus_Red/Nexus_Red.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Nexus_Red");


	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Turret_Blue"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Buildings/Turret_Blue/Turret_Blue.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Turret_Blue");


	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Turret_Red"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Buildings/Turret_Red/Turret_Red.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Turret_Red");

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Turret_Shield"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Buildings/Turret_Shield/Turret_Shield.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Turret_Shield");

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Npc_Store"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Buildings/Npc_Store/Npc_Store.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Npc_Store");

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Npc_StoreKeeper"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Buildings/Npc_StoreKeeper/Npc_StoreKeeper.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Npc_StoreKeeper");


#pragma endregion Animation

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Ezreal_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Ezreal/Ezreal.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Ezreal_NonAnim"));

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Galio_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Galio/Galio.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Galio_NonAnim"));
//
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Leona_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Leona/Leona.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Leona_NonAnim"));
//
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Vi_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Vi/Vi.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Vi_NonAnim"));
//
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Leesin_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Leesin/Leesin.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Leesin_NonAnim"));
//

//
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Braum_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Braum/Braum.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Braum_NonAnim"));
//
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Garen_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Garen/Garen.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Garen_NonAnim"));
//
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Gwen_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Gwen/Gwen.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Gwen_NonAnim"));
//
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Renekton_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Renekton/Renekton.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Renekton_NonAnim"));
//
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Smolder_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Smolder/Smolder.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Smolder_NonAnim"));
//
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Yasuo_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Yasuo/Yasuo.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Yasuo_NonAnim"));

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Vayne_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Vayne/Vayne.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Vayne_NonAnim"));
//
#pragma region Buildings
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Nexus_Blue_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Buildings/Nexus_Blue/Nexus_Blue.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Nexus_Blue_NonAnim"));
//
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Nexus_Red_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Buildings/Nexus_Red/Nexus_Red.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Nexus_Red_NonAnim"));
//
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Inhibitor_Blue_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Buildings/Inhibitor_Blue/Inhibitor_Blue.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Inhibitor_Blue_NonAnim"));

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Inhibitor_Red_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Buildings/Inhibitor_Red/Inhibitor_Red.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Inhibitor_Red_NonAnim"));

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Turret_Blue_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Buildings/Turret_Blue/Turret_Blue.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Turret_Blue_NonAnim"));

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Turret_Red_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Buildings/Turret_Red/Turret_Red.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Turret_Red_NonAnim"));

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Turret_Shield_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Buildings/Turret_Shield/Turret_Shield.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Turret_Shield_NonAnim"));

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Npc_Store_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Buildings/Npc_Store/Npc_Store.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Npc_Store_NonAnim"));

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Npc_StoreKeeper_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Buildings/Npc_StoreKeeper/Npc_StoreKeeper.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Npc_StoreKeeper_NonAnim"));

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Minion_Cannon_Blue_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Minion/Minion_Cannon_Blue/Minion_Cannon_Blue.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Minion_Cannon_Blue_NonAnim"));

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Minion_Cannon_Red_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Minion/Minion_Cannon_Red/Minion_Cannon_Red.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Minion_Cannon_Red_NonAnim"));


	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Minion_Melee_Blue_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Minion/Minion_Melee_Blue/Minion_Melee_Blue.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Minion_Melee_Blue_NonAnim"));

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Minion_Melee_Red_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Minion/Minion_Melee_Red/Minion_Melee_Red.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Minion_Melee_Red_NonAnim"));

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Minion_Range_Blue_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Minion/Minion_Range_Blue/Minion_Range_Blue.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Minion_Range_Blue_NonAnim"));

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Minion_Range_Red_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Minion/Minion_Range_Red/Minion_Range_Red.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Minion_Range_Red_NonAnim"));

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Minion_Super_Blue_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Minion/Minion_Super_Blue/Minion_Super_Blue.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Minion_Super_Blue_NonAnim"));

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Minion_Super_Red_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Minion/Minion_Super_Red/Minion_Super_Red.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Minion_Super_Red_NonAnim"));



#pragma endregion 



	m_isFinished = true;
	Safe_Release(pGameInstance);
	return S_OK;
}

CLoader* CLoader::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eNextLevel)
{
	CLoader* pInstance = new CLoader(pDevice, pContext);

	if (FAILED(pInstance->Initialize(eNextLevel)))
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

	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);
}
