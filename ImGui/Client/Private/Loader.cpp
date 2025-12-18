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
#include "EffectTexture.h"
#include "EffectMesh.h"
#include "ParticleSystem.h"

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
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/Logo/BackGround0.dds"), 1))))
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

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Picking_Symbol"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Picking_Symbol/Picking_Symbol.dat"))))
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

	/*For.Prototype_GameObject_EffectTexture*/
	if (FAILED(pGameInstance->Add_Prototype(TEXT("Prototype_GameObject_EffectTexture"),
		CEffectTexture::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/*For.Prototype_GameObject_EffectMesh*/
	if (FAILED(pGameInstance->Add_Prototype(TEXT("Prototype_GameObject_EffectMesh"),
		CEffectMesh::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/*For.GameObject_ParticleSystem*/
	if (FAILED(pGameInstance->Add_Prototype(TEXT("Prototype_GameObject_ParticleSystem"),
		CParticleSystem::Create(m_pDevice, m_pContext))))
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

	if (FAILED(Loading_ForEffect()))
		return E_FAIL;

	if (FAILED(Loading_ForEffectTexture()))
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

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Alistar"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Alistar/Alistar.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Alistar");

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

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Cube"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Picking_Symbol/Picking_Symbol.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Cube"));

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
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Renekton_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Renekton/Renekton.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Renekton_NonAnim"));
//
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Yasuo_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Yasuo/Yasuo.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Yasuo_NonAnim"));

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Vayne_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Vayne/Vayne.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Vayne_NonAnim"));

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Alistar_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Alistar/Alistar.dat"))))
		return E_FAIL;
	CModelManager::GetInstance()->Add_PrototypeTag(TEXT("Alistar_NonAnim"));
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

HRESULT CLoader::Loading_ForEffect()
{
	CGameInstance* pGameInstance = CGameInstance::GetInstance();

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Plane"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Plane/Plane.dat"))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("HalfSphere"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Sphere/HalfSphere.dat"))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Sphere"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Sphere/Sphere.dat"))))
		return E_FAIL;
	return S_OK;
}

HRESULT CLoader::Loading_ForEffectTexture()
{
	CGameInstance* pGameInstance = CGameInstance::GetInstance();
	pGameInstance->AddRef();

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Distortion_Noise"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Distortion_Noise.png"), 1))))
		return E_FAIL;

#pragma region Ezreal Effect
	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("common_color-hold"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/common_color-hold.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("common_crystal-flash"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/common_crystal-flash.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("common_flare-rainbow_blue_green_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/common_flare-rainbow_blue_green_02.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("common_glow-soft"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/common_glow-soft.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_ashe_base_ground_cracknoise_mult"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_ashe_base_ground_cracknoise_mult.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_ba_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_ba_glow.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_ba_mis"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_ba_mis.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_ba_whisps"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_ba_whisps.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_q_erode"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_q_erode.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_q_hit_spark"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_q_hit_spark.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_q_mis_trail"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_q_mis_trail.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_r_bow_arrow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_r_bow_arrow.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_r_bow_solid_texutre"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_r_bow_solid_texutre.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_r_bow_tex"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_r_bow_tex.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_r_mis_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_r_mis_glow.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_r_tail_tex"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_r_tail_tex.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_r_trail"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_r_trail.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_w_cas_tex"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_w_cas_tex.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_w_meshlines"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_w_meshlines.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_w_orb"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_w_orb.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_w_ringglow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_w_ringglow.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_w_spintex"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_w_spintex.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_z_ball01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_z_ball01.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_z_color-beacontrail"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_z_color-beacontrail.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_z_color-hold"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_z_color-hold.png"), 1))))
		return E_FAIL;


	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_z_flare-sun"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_z_flare-sun.png"), 1))))
		return E_FAIL;


	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_z_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_z_glow.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_z_glowflare"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_z_glowflare.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_z_ring"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_z_ring.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_z_sliver"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_z_sliver.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_z_solar_tag"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_z_solar_tag.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_r_cas_pulse"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_r_cas_pulse.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezvgu_tristana_skin12_ba_rings"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezvgu_tristana_skin12_ba_rings.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezvgu_tristana_skin12_z_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezvgu_tristana_skin12_z_glow.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ezvgu_tristana_skin12_z_praxiswave"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezvgu_tristana_skin12_z_praxiswave.png"), 1))))
		return E_FAIL;

#pragma endregion 

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
