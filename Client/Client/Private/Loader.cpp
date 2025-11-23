
#include "Loader.h"

#include "GameInstance.h"

#include "Terrian.h"
#include "CCamera_Dynamic.h"
#include "NonAnim.h"
#include "Sky.h"

// UI
#include "UI_SkillMenu.h"
#include "UI_SkillImage.h"
#include "UI_ExpBar.h"
#include "UI_ChampionIcon.h"
#include "UI_MiniMap.h"
#include "UI_GameInfo.h"
#include "UI_ItemInfo.h"

// Champion
#include "Ezreal.h"
#include "Renekton.h"
#include "Vi.h"

// Building
#include "Nexus.h"
#include "Turret.h"
#include "Inhibitor.h"
#include "StoreNpc.h"

// Minion
#include "Minion_Melee.h"
#include "Minion_Range.h"



CLoader::CLoader(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pGameInstnace(CGameInstance::GetInstance())
	, m_pDevice { pDevice }
	, m_pContext { pContext }
{
	Safe_AddRef(m_pGameInstnace);
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

unsigned int APIENTRY ThreadMain(void* pArg)
{
	CLoader* pLoader = static_cast<CLoader*>(pArg);

	
	EnterCriticalSection(&pLoader->Get_CriticalSection());

	switch (pLoader->Get_NextLevelID())
	{
	case LEVEL_LOGO:
		pLoader->Loading_For_Logo();
		break;
	case LEVEL_GAMEPLAY:
		pLoader->Loading_For_GamePlay();
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

	m_hThread = (HANDLE)_beginthreadex(nullptr, 0, ThreadMain, this, 0, nullptr);
	if (0 == m_hThread)
		return E_FAIL;

	return S_OK;
}


HRESULT CLoader::Loading_For_Logo()
{	
 	if (m_pGameInstnace == nullptr)
		return E_FAIL;

	lstrcpy(m_szLoadingText, TEXT("텍스쳐 로딩 중 "));
	m_fProgress = 0.f;

	lstrcpy(m_szLoadingText, TEXT("원본 객체 로딩 중"));

	if(FAILED(Loading_For_Prtotype()))
		return E_FAIL;

	lstrcpy(m_szLoadingText, TEXT("로딩이 완료되었슴니다."));
	m_fProgress = 100.f;
	m_isFinished = true;
	return S_OK;
}

HRESULT CLoader::Loading_For_Prtotype()
{
 	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_NonAnim"),
		CNonAnim::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	// For Prototype Object Terrian 
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Terrian"),
		CTerrian::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	// For Prototype Object CameraDynamic 
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Camera_Dynamic"),
		CCamera_Dynamic::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	// For Prototype Object Sky
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Sky"),
		CSky::Create(m_pDevice, m_pContext))))
		return E_FAIL;

#pragma region UI_Proto
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_UI_SkillMenu"),
		CUI_SkillMenu::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_UI_SkillImage"),
		CUI_SkillImage::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_UI_ExpBar"),
		CUI_ExpBar::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_UI_ChampionIcon"),
		CUI_ChampionIcon::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_UI_MiniMap"),
		CUI_MiniMap::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_UI_GameInfo"),
		CUI_GameInfo::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_UI_ItemInfo"),
		CUI_ItemInfo::Create(m_pDevice, m_pContext))))
		return E_FAIL;
#pragma endregion 
#pragma region Champion
	/*For.Prototype_GameObject_Champion*/
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Ezreal"),
		CEzreal::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	/*For.Prototype_GameObject_Champion*/
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Renekton"),
		CRenekton::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	/*For.Prototype_GameObject_Champion*/
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Vi"),
		CVi::Create(m_pDevice, m_pContext))))
		return E_FAIL;

#pragma endregion

#pragma region Building
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Nexus"),
		CNexus::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Inhibitor"),
		CTurret::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Turret"),
		CInhibitor::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Npc"),
		CStoreNpc::Create(m_pDevice, m_pContext))))
		return E_FAIL;
#pragma endregion Building

#pragma region Minion
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Minion_Melee"),
		CMinion_Melee::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Minion_Range"),
		CMinion_Range::Create(m_pDevice, m_pContext))))
		return E_FAIL;
#pragma endregion Minion
	return S_OK;
}

HRESULT CLoader::Loading_For_GamePlay()
{
	lstrcpy(m_szLoadingText, TEXT("로딩이 완료되었슴니다."));
	m_fProgress = 0.f;
#pragma region 셰이더 로딩
	lstrcpy(m_szLoadingText, TEXT("원본 셰이더 로딩 중"));
	// For Prototype Component Shader VtxNorTex
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxNorTex"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../../../Bin/ShaderFiles/Shader_NorTexTerrianl.hlsl"), VTXNORTEX_DECLARATION::Elements, VTXNORTEX_DECLARATION::iNumElements))))
		return E_FAIL;

	// For Prototype Component Shader VtxNoAnim
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxModel"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../../../Bin/ShaderFiles/Shader_VtxModel.hlsl"), VTXNORTEX_DECLARATION::Elements, VTXNORTEX_DECLARATION::iNumElements))))
		return E_FAIL;

	// For Prototype Component Shader VtxAnim
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxAnimModel"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../../../Bin/ShaderFiles/Shader_VtxAnimModel.hlsl"), VTXANIMMODEL_DECLARATION::Elements, VTXANIMMODEL_DECLARATION::iNumElements))))
		return E_FAIL;

	/* For.Prototype_Component_Shader_VtxCubeTexture */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxCubeTexture"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../../../Bin/ShaderFiles/Shader_VtxCubeTexture.hlsl"), VTXCUBETEX_DECLARATION::Elements, VTXCUBETEX_DECLARATION::iNumElements))))
		return E_FAIL;

#pragma region 셰이더 로딩


#pragma region 버퍼 로딩
	lstrcpy(m_szLoadingText, TEXT("버퍼 로딩 중"));
	m_fProgress = 10.f;
	/*For.Prototype_Component_VIBuffer_Cube */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_VIBuffer_Cube"),
		CVIBuffer_Cube::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	HANDLE hFile = 0;
	_ulong dwByte = 0;
	_uint iNum = 0;
	hFile = CreateFile(TEXT("../../../Bin/Data/Summoners'LiftData/Terrian.dat"), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	ReadFile(hFile, &(iNum), sizeof(_uint), &dwByte, nullptr);
 	if(FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Summoners_Rift"), CVIBuffer_Terrian::Create(m_pDevice, m_pContext, hFile, dwByte))))
		return E_FAIL;

#pragma endregion 
	m_fProgress = 40.f;
	if (FAILED(Loading_For_GameUI()))
		return E_FAIL;

#pragma region 모델 로딩
	lstrcpy(m_szLoadingText, TEXT("맵 로딩중 ")); 
	m_fProgress = 50.f;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Map"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Map/Map.dat"))))
		return E_FAIL;
	lstrcpy(m_szLoadingText, TEXT("챔피언 모델 로딩중 "));
	m_fProgress = 60.f;
	// 이즈리얼
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Ezreal"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Ezreal/Ezreal.dat"))))
		return E_FAIL;
	// 레넥톤
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Renekton"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Renekton/Renekton.dat"))))
		return E_FAIL;
	// 바이
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Vi"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Vi/Vi.dat"))))
		return E_FAIL;

	// 근거리 미니언
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Minion_Melee_Blue"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Minion/Minion_Melee_Blue/Minion_Melee_Blue.dat"))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Minion_Melee_Red"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Minion/Minion_Melee_Red/Minion_Melee_Red.dat"))))
		return E_FAIL;

	// 원거리 미니언
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Minion_Range_Blue"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Minion/Minion_Range_Blue/Minion_Range_Blue.dat"))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Minion_Range_Red"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Minion/Minion_Range_Red/Minion_Range_Red.dat"))))
		return E_FAIL;

	lstrcpy(m_szLoadingText, TEXT("건물 로딩 중"));
	m_fProgress = 70.f;
	// 넥서스
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Nexus_Blue_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Buildings/Nexus_Blue/Nexus_Blue.dat"))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Nexus_Red_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Buildings/Nexus_Red/Nexus_Red.dat"))))
		return E_FAIL;
	// 포탑
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Turret_Blue_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Buildings/Turret_Blue/Turret_Blue.dat"))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Turret_Red_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Buildings/Turret_Red/Turret_Red.dat"))))
		return E_FAIL;
	// 억제기
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Inhibitor_Blue_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Buildings/Inhibitor_Blue/Inhibitor_Blue.dat"))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Inhibitor_Red_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Buildings/Inhibitor_Red/Inhibitor_Red.dat"))))
		return E_FAIL;
	// Npc
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Npc_StoreKeeper_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Buildings/Npc_StoreKeeper/Npc_StoreKeeper.dat"))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Npc_Store_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/NonAnim/Champion/Buildings/Npc_Store/Npc_Store.dat"))))
		return E_FAIL;


#pragma endregion

#pragma region 네비게이션 로딩
	m_fProgress = 80.f;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_ChampionNavigation"),
		CNavigation::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Data/NavDat/Champion_Nav.dat")))))
		return E_FAIL;
#pragma rendregion

#pragma region 텍스쳐 로딩
	m_fProgress = 90.f;
	lstrcpy(m_szLoadingText, TEXT("텍스쳐 로딩 중"));
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Texture_Sky_Map"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/SkyBox/riots_sru_skybox.dds"), 1))))
		return E_FAIL;

	m_fProgress = 100.f;
	m_isFinished = true;

	return S_OK;
}

HRESULT CLoader::Loading_For_GameUI()
{
	lstrcpy(m_szLoadingText, TEXT("UI 로딩 중"));
	// Skill Interface
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_SkillInterface"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/Interface/SkillWindow.dds"), 1))))
		return E_FAIL;
	// Exp Bar
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_ExpBar"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/Interface/ChampionExp.dds"), 1))))
		return E_FAIL;
	// MiniMap
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_MiniMap"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/Interface/MiniMapFrame.dds"), 1))))
		return E_FAIL;
	// GameInfo
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_GameInfo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/Interface/GameInfo.dds"), 1))))
		return E_FAIL;
	// ItemInfo
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_ItemInfo"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/Interface/ItemInfo.dds"), 1))))
		return E_FAIL;
#pragma region Ezreal_SkillImage
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_Ezreal_Icon"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/SkillImage/Ezreal/ezreal_circle.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_EzrealSkill_Passive"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/SkillImage/Ezreal/ezreal_passive.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_EzrealSkill_Q"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/SkillImage/Ezreal/ezreal_q.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_EzrealSkill_W"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/SkillImage/Ezreal/ezreal_w.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_EzrealSkill_E"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/SkillImage/Ezreal/ezreal_e.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_EzrealSkill_R"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/SkillImage/Ezreal/ezreal_r.dds"), 1))))
		return E_FAIL;
#pragma endregion

#pragma region Renekton_SkillImage
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_Renekton_Icon"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/SkillImage/Renekton/renekton_circle_0.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_RenektonSkill_Passive"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/SkillImage/Renekton/renekton_passive.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_RenektonSkill_Q"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/SkillImage/Renekton/renekton_q.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_RenektonSkill_W"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/SkillImage/Renekton/renekton_w.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_RenektonSkill_E"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/SkillImage/Renekton/renekton_e.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_RenektonSkill_R"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/SkillImage/Renekton/renekton_r.dds"), 1))))
		return E_FAIL;
#pragma endregion

#pragma region Vi_SkillImage
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_Vi_Icon"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/SkillImage/Vi/vi_circle.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_ViSkill_Passive"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/SkillImage/Vi/vipassive.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_ViSkill_Q"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/SkillImage/Vi/viq.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_ViSkill_W"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/SkillImage/Vi/viw.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_ViSkill_E"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/SkillImage/Vi/vie.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_ViSkill_R"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/SkillImage/Vi/vir.dds"), 1))))
		return E_FAIL;
#pragma endregion
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
	WaitForSingleObject(m_hThread, INFINITE);

	CloseHandle(m_hThread);

	DeleteCriticalSection(&m_CriticalSection);

	Safe_Release(m_pGameInstnace);
	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);
}
