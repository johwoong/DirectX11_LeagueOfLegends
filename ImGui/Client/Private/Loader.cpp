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

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("BowMesh"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Bow/Bow.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("BowMesh");

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Teemo"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Teemo/Teemo.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("Teemo");

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("MushRoom"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/MushRoom/MushRoom.dat"))))
		return E_FAIL;
	pImgui->m_AnimObj.push_back("MushRoom");

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
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Bow"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Bow/Bow.dat"))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Cylinder0"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Cylinder/Cylinder0.dat"))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Cylinder1"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Cylinder/Cylinder1.dat"))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Cylinder2"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Cylinder/Cylinder2.dat"))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("CylinderSphere"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Cylinder/CylinderSphere.dat"))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("CylinderWind"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Cylinder/CylinderWind.dat"))))
		return E_FAIL;

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("FloorCircle"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Plane/FloorCircle.dat"))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Ring"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Ring/Ring.dat"))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Slash"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Slash/Slash.dat"))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Trail"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Trail/Trail.dat"))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Sphere0"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Sphere/Sphere.dat"))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("HalfSphere_ak_04"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Sphere/HalfSphere.dat"))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Akizame"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Akizame/Akizame.dat"))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Arrow"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Arrow/Arrow.dat"))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Bullet"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Bullet/Bullet.dat"))))
		return E_FAIL;
	return S_OK;
}

HRESULT CLoader::Loading_ForEffectTexture()
{
	CGameInstance* pGameInstance = CGameInstance::GetInstance();
	pGameInstance->AddRef();


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

#pragma region Vayne
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("aura_self"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/aura_self.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("base_alphaslice_1"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/base_alphaslice_1.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("color-hold"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/color-hold.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("colorcommon_alpha_02hold"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/common_alpha_02.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("common_bats32"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/common_bats32.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("common_color-bellcurve4"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/common_color-bellcurve4.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("common_color-eyefade32"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/common_color-eyefade32.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("common_color-hit-physical"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/common_color-hit-physical.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("common_color-smoke32"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/common_color-smoke32.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("common_kogmaw_skin05_tar_flash"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/common_kogmaw_skin05_tar_flash.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("common_sand_04"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/common_sand_04.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("distort-pinch"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/distort-pinch.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_ba_erosionpack01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_ba_erosionpack01.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_ba_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_ba_glow.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_ba_lensflare"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_ba_lensflare.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_ba_smokeshape01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_ba_smokeshape01.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_ba_smoketip"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_ba_smoketip.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_ba_wispysmoke"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_ba_wispysmoke.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_e_erosionpack01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_e_erosionpack01.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_e_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_e_glow.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_e_lensflare"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_e_lensflare.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_e_smokeshape01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_e_smokeshape01.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_e_smoketip"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_e_smoketip.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_e_tar_terrain_dust"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_e_tar_terrain_dust.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_e_terrain_dirt"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_e_terrain_dirt.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_flames2"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_flames2.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_glow.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_hit_spark"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_hit_spark.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_noise01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_noise01.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_noise2"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_noise2.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_noise04"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_noise04.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_omniflare"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_omniflare.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_p_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_p_glow.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_particles_1_204"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_particles_1_204.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_particles_1_399"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_particles_1_399.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_q_erosionpack01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_q_erosionpack01.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_q_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_q_glow.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_q_wispysmoke"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_q_wispysmoke.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_r_erosionpack01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_r_erosionpack01.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_r_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_r_glow.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_r_ground_03"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_r_ground_03.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_r_ground_04"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_r_ground_04.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_r_lensflare"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_r_lensflare.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_r_smokeshape01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_r_smokeshape01.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_r_smoketip"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_r_smoketip.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_r_wispysmoke"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_r_wispysmoke.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_sparkles01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_sparkles01.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_taunt_flecks"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_taunt_flecks.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_w_dust"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_w_dust.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_w_erode"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_w_erode.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_w_erosionpack01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_w_erosionpack01.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_w_flare"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_w_flare.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_w_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_w_glow.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_w_linesbigmedsmall"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_w_linesbigmedsmall.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_w_ring_1"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_w_ring_1.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_w_ring_2"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_w_ring_2.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_w_ring_3"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_w_ring_3.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_w_ring_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_w_ring_glow.dds"), 1))))
		return E_FAIL;
#pragma endregion

	/* Effect Textures */
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Line_hn_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Unique/TO14_T_FX_Line_hn_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Line_hn_03"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Unique/TO14_T_FX_Line_hn_03.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_trail_ak_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Unique/TO14_T_FX_trail_ak_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_trail_rt_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Unique/TO14_T_FX_trail_rt_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_trail_rt_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Unique/TO14_T_FX_trail_rt_02.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_trail_rt_03"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Unique/TO14_T_FX_trail_rt_03.png"), 1))))
		return E_FAIL;

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("HolyLance_Mask_04_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/HolyLance/HolyLance_Mask_04_M.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("HolyLance_Trail01_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/HolyLance/HolyLance_Trail01_M.png"), 1))))
		return E_FAIL;

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("HolyLanceGradient02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/HolyLance/HolyLanceGradient02.png"), 1))))
		return E_FAIL;

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ShieldTexture"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Unique/ShieldTexture.png"), 1))))
		return E_FAIL;


	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Distortion_Noise"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Distortion_Noise.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Heart"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Heart.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Particle_Horizontal"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Particle_Horizontal.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Particle_Vertical"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Particle_Vertical.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Default_Mask"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Default_Mask.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Spark_00"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Spark_00.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Spark_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Spark_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Spark_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Spark_02.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Spark_03"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Spark_03.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Flash_00"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Flash_00.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Flash_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Flash_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Line"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Line.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Aura_ak_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_Aura_ak_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Aura_ak_03"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_Aura_ak_03.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Aura_ak_04"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_Aura_ak_04.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Common_Noise_02_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_Common_Noise_02_M.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Common_Noize_03_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_Common_Noize_03_M.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Common_Pattern_01_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_Common_Pattern_01_M.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Common_Pattern_02_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_Common_Pattern_02_M.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Common_Pattern_03_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_Common_Pattern_03_M.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Common_Pattern_04_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_Common_Pattern_04_M.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Common_Pattern_05_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_Common_Pattern_05_M.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Common_Pattern_06_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_Common_Pattern_06_M.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Common_Pattern_07_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_Common_Pattern_07_M.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Common_Pattern_08_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_Common_Pattern_08_M.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Common_Pattern_09_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_Common_Pattern_09_M.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Common_WaveNoise_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_Common_WaveNoise_M.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Nois_hn_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_Nois_hn_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Nois_hn_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_Nois_hn_02.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Nois_hn_03"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_Nois_hn_03.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Nois_hn_04"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_Nois_hn_04.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Nois_hn_05"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_Nois_hn_05.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_07_SO_AlphaC"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_07_SO_AlphaC.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_256_ny_08"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_256_ny_08.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_256_ny_10"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_256_ny_10.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_256_ny_11"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_256_ny_11.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_512_ny_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_512_ny_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_512_ny_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_512_ny_02.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_512_ny_03"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_512_ny_03.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_512_ny_04"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_512_ny_04.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_512_ny_05"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_512_ny_05.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_512_ny_06"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_512_ny_06.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_512_ny_07"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_512_ny_07.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_512_ny_09"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_512_ny_09.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_512_ny_12"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_512_ny_12.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_512_ny_13"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_512_ny_13.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_512_ny_14"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_512_ny_14.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_512_ny_15"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_512_ny_15.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_512_ny_16"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_512_ny_16.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_512_ny_17"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_512_ny_17.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_512_ny_18"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_512_ny_18.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_ny_AlphaC09"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_ny_AlphaC09.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Noise_SO_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_Noise_SO_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_SO_AlphaC01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_SO_AlphaC01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_SO_AlphaC01_t2"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_SO_AlphaC01_t2.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_SO_AlphaC02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_SO_AlphaC02.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_SO_AlphaC02_t2"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_SO_AlphaC02_t2.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_SO_AlphaC03"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_SO_AlphaC03.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_SO_AlphaC04"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_SO_AlphaC04.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_SO_AlphaC08"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_SO_AlphaC08.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_SO_AlphaC09"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_SO_AlphaC09.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_SO_AlphaC10"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_SO_AlphaC10.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_t2_10"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/TO14_T_FX_noise_t2_10.png"), 1))))
		return E_FAIL;

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Cloud_ny_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Smoke/TO14_T_FX_Cloud_ny_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Common_SandMist_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Smoke/TO14_T_FX_Common_SandMist_M.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Common_Snow_01_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Smoke/TO14_T_FX_Common_Snow_01_M.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Common_Snow_01_M_copy"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Smoke/TO14_T_FX_Common_Snow_01_M_copy.png"), 1))))
		return E_FAIL;

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("MagumaTrail_tk_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Water/MagumaTrail_tk_02.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("copy_Foam2_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Water/copy_Foam2_M.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Warter_Splash_N_ny01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Water/Warter_Splash_N_ny01.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Warter_Splash_ny01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Water/Warter_Splash_ny01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Water_rm_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Water/Water_rm_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Wave_SO_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Water/Wave_SO_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("FlareHorizon"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/FlareHorizon.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("FlareHorizon_Vertical"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/FlareHorizon_Vertical.png"), 1))))
		return E_FAIL;


	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Line_hn_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Line/Line_hn_02.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Line_hn_11"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Line/Line_hn_11.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("LineScroll_SO_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Line/LineScroll_SO_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("LineUnique_ym_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Line/LineUnique_ym_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("SpellText"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Line/SpellText.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("SpellText_rm"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Line/SpellText_rm.png"), 1))))
		return E_FAIL;


	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_trail_ny_08"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Line/TO14_T_FX_trail_ny_08.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ShockWave"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Shock/ShockWave.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ShockWave2"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Shock/ShockWave2.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ShockWave3"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Shock/ShockWave3.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ShockWave4"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Shock/ShockWave4.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Flare01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Shock/Flare01.png"), 1))))
		return E_FAIL;

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("RockFormation0"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Particle/RockFormation0.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("RockFormation1"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Particle/RockFormation1.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("RockFormation2"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Particle/RockFormation2.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Common_RadialGradient_01_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Ring/TO14_T_FX_Common_RadialGradient_01_M.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Common_RadialGradient_inv_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Ring/TO14_T_FX_Common_RadialGradient_inv_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Common_RainbowFlare01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Ring/TO14_T_FX_Common_RainbowFlare01.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("DotNoise_rm_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/DotNoise/DotNoise_rm_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("DotNoise_rm_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/DotNoise/DotNoise_rm_02.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("DotNoise_rm_03"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/DotNoise/DotNoise_rm_03.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("DotNoise_tk_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/DotNoise/DotNoise_tk_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("DotNoise_tk_03"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/DotNoise/DotNoise_tk_03.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("DotNoise_tk_05"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/DotNoise/DotNoise_tk_05.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("DotNoise_tk_06"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/DotNoise/DotNoise_tk_06.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Noise_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/DotNoise/Noise_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Voronoi_Noise"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/DotNoise/Voronoi_Noise.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Gradient"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/Gradient.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_circle_ny_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/TO14_T_FX_circle_ny_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Gradation_ak_05"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/TO14_T_FX_Gradation_ak_05.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Gradation_hn_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/TO14_T_FX_Gradation_hn_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_RadiusQuarter_tk_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/TO14_T_FX_RadiusQuarter_tk_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Ring_hn_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/TO14_T_FX_Ring_hn_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Ring_hn_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/TO14_T_FX_Ring_hn_02.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Ring_hn_03"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/TO14_T_FX_Ring_hn_03.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Ring_hn_04"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/TO14_T_FX_Ring_hn_04.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Round_SO_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/TO14_T_FX_Round_SO_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Round_SO_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/TO14_T_FX_Round_SO_02.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Round_SO_03"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/TO14_T_FX_Round_SO_03.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Round_SO_04"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/TO14_T_FX_Round_SO_04.png"), 1))))
		return E_FAIL;



	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("CoreAura_rm_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Unique/CoreAura_rm_01.png"), 1))))
		return E_FAIL;

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("CoreAura_rm_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Unique/CoreAura_rm_02.png"), 1))))
		return E_FAIL;

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("DropMask_tk_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Unique/DropMask_tk_01.png"), 1))))
		return E_FAIL;

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("SlashMask_tk_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Unique/SlashMask_tk_01.png"), 1))))
		return E_FAIL;

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Smash_rm_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Unique/Smash_rm_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Smash_rm_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Unique/Smash_rm_02.png"), 1))))
		return E_FAIL;


	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("circle_ny_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Ring/circle_ny_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Gradient_rm_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Ring/Gradient_rm_01.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Round_SO_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Ring/Round_SO_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Round_SO_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Ring/Round_SO_02.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Round_SO_03"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Ring/Round_SO_03.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Round_SO_04"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Ring/Round_SO_04.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Round_SO_04_2"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Ring/Round_SO_04_2.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Explode_Fractal01_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Explode/Explode_Fractal01_M.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Explode_Smoke01_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Explode/Explode_Smoke01_M.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("FireNoise_is_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Explode/FireNoise_is_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("DustParticle_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Particle/DustParticle_M.png"), 1))))
		return E_FAIL;

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Smoke_Mask"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Smoke/Smoke_Mask.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Smoke_Mask1"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Smoke/Smoke_Mask1.png"), 1))))
		return E_FAIL;

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Puddle"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Mask/Puddle.png"), 1))))
		return E_FAIL;

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("CrackPattern"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Crack/CrackPattern.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("MagmaCracks"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Crack/MagmaCracks.dds"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Crack_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Crack/Crack_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Crack_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Crack/Crack_02.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Crack_03"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Crack/Crack_03.png"), 1))))
		return E_FAIL;

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Height_Gradation"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/Height_Gradation.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Height_Gradation2"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/Height_Gradation2.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Height_Gradation3"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/Height_Gradation3.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Height_Gradation4"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/Height_Gradation4.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Height_Gradation5"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/Height_Gradation5.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Height_Gradation6"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/Height_Gradation6.png"), 1))))
		return E_FAIL;

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("floormask"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Mask/floormask.png"), 1))))
		return E_FAIL;

	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Thunder_1"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Thunder/Thunder_1.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Thunder_2"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Thunder/Thunder_2.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Thunder_3"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Thunder/Thunder_3.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Thunder_4"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Thunder/Thunder_4.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Thunder_a"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Thunder/Thunder_a.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Thunder_b"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Thunder/Thunder_b.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("Thunder_c"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Thunder/Thunder_c.png"), 1))))
		return E_FAIL;

#pragma region LeeSin
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_moon_sight_indicator"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_moon_sight_indicator.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("glowflare"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/glowflare.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_flare-rainbow_yellow_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_flare-rainbow_yellow_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_fire_mult"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_fire_mult.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_q_swipekick_03"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_q_swipekick_03.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_q_swipekick_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_q_swipekick_02.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_q_swipe_dissolve_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_q_swipe_dissolve_02.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_q_swipe_dissolve_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_q_swipe_dissolve_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_fire_ash_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_fire_ash_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_alphastrike_prison__ball__p-rgba"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_alphastrike_prison__ball__p-rgba.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_sonicwave"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_sonicwave.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_galaxysphere_streaks"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_galaxysphere_streaks.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("alpha_energymask_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/alpha_energymask_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("alphaslice_bubbles_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/alphaslice_bubbles_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("alphaslice_bubbles_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/alphaslice_bubbles_02.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("circle_rainbow_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/circle_rainbow_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_p_armfire"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_p_armfire.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("color-bellcurve"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/color-bellcurve.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("dissolve_cloudy_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/dissolve_cloudy_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("dissolve_cloudy_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/dissolve_cloudy_02.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("empty32"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/empty32.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_color-firefade"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_color-firefade.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_shield_decal"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_shield_decal.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_glow-soft"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_glow-soft.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_glow_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_glow_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_flame_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_flame_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_shockwave"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_shockwave.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_distort-ripples"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_distort-ripples.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_speedcolor"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_speedcolor.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_trailgrey"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_trailgrey.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_fireswirl"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_fireswirl.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_ring_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_ring_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_shield_ring_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_shield_ring_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11__moon_sight_06"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11__moon_sight_06.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_radial_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_radial_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_ring_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_ring_02.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_red_orange_lightbar_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_red_orange_lightbar_02.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_linger_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_linger_glow.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_blast_nova_bunny_04"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_blast_nova_bunny_04.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_splat_05"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_splat_05.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_z_erosion_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_z_erosion_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("energynebula_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/energynebula_01.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("energynebula_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/energynebula_02.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("groundcracks_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/groundcracks_02.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("leesin_skin11_crater_01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/LeeSin/leesin_skin11_crater_01.png"), 1))))
		return E_FAIL;
#pragma endregion 


#pragma region Teemo
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_e_dart_motionblur.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_e_dart_motionblur.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_ba_mis_cap"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_ba_mis_cap.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_trail_mult.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_trail_mult.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_trail01.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_trail01.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_mis_erode.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_mis_erode.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_generic_noisemulti01.asu_teemo.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_generic_noisemulti01.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_energymote_01.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_energymote_01.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_emote_distortion_gradient.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_emote_distortion_gradient.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_emote_distortion.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_emote_distortion.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_e_tar_flash.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_e_tar_flash.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_e_smoketrail.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_e_smoketrail.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_e_small_mote.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_e_small_mote.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_e_posion_smoke_cloud.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_e_posion_smoke_cloud.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_e_mis_cap_mult.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_e_mis_cap_mult.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_e_global_poison_splat.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_e_global_poison_splat.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_e_global_poison_smoke.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_e_global_poison_smoke.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_e_global_poison_mult_v3.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_e_global_poison_mult_v3.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_glowsoft.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_glowsoft.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_ba_mis_core_add"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_ba_mis_core_add.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_e_global_poison_bubble.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_e_global_poison_bubble.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_q_dart_motionblur_2.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_q_dart_motionblur_2.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_q_trail.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_q_trail.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_q_blind_icon_01.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_q_blind_icon_01.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_wisps_white_01.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_wisps_white_01.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_w_mis_cap.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_w_mis_cap.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_w_leaf.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_w_leaf.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_w_buf2_pierce_mult.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_w_buf2_pierce_mult.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_w_buf2_pierce.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_w_buf2_pierce.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("ring_highres.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/ring_highres.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_r_grassfliptxt.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_r_grassfliptxt.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_lantern_ring_01.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_lantern_ring_01.asu_teemo.png"), 1))))
		return E_FAIL;
	if (FAILED(pGameInstance->Add_Prototype(LEVEL_STATIC, TEXT("teemo_base_r_circle_normal.asu_teemo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Teemo/teemo_base_r_circle_normal.asu_teemo.png"), 1))))
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
