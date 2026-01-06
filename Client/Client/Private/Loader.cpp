
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
#include "UI_AttackRange.h"
#include "UI_Minion_HpBar.h"
#include "UI_Enemy_HpBar.h"
#include "UI_Building_HpBar.h"

// Effect
#include "EffectTexture.h"
#include "EffectMesh.h"
#include "ParticleSystem.h"
#include "Particle_Rect.h"
#include "Particle_Point.h"
#include "EffectObject.h"


// Champion
#include "Ezreal.h"
#include "Vi.h"

// Enemy
#include "Vayne.h"
#include "Renekton.h"
#include "Leona.h"

// Projectile
#include "EzrealProjectile.h"
#include "Bow.h"
#include "MinionProjectile.h"
#include "TurretProjectile.h"
#include "VayneProjectile.h"

// Building
#include "Nexus.h"
#include "Turret.h"
#include "Inhibitor.h"

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
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	CLoader* pLoader = static_cast<CLoader*>(pArg);

	EnterCriticalSection(&pLoader->Get_CriticalSection());

	switch (pLoader->Get_NextLevelID())
	{
	case LEVEL_LOGO:      pLoader->Loading_For_Logo(); break;
	case LEVEL_GAMEPLAY:  pLoader->Loading_For_GamePlay(); break;
	}

	LeaveCriticalSection(&pLoader->Get_CriticalSection());
	if (SUCCEEDED(hr))
		CoUninitialize();

	return 0;
}

HRESULT CLoader::Initialize(LEVEL eNextLevel)
{

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

	lstrcpy(m_szLoadingText, TEXT("이펙트 로딩 중"));
	if (FAILED(Loading_ForEffect()))
		return E_FAIL;
	if (FAILED(Loading_ForEffectTexture()))
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

	/*For.Prototype_GameObject_EffectTexture*/
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_EffectTexture"),
		CEffectTexture::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/*For.Prototype_GameObject_EffectMesh*/
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_EffectMesh"),
		CEffectMesh::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/*For.GameObject_ParticleSystem*/
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_ParticleSystem"),
		CParticleSystem::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/*For.Prototype_GameObject_Particle_Rect */
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Particle_Rect"),
		CParticle_Rect::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/*For.Prototype_GameObject_Particle_Point */
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Particle_Point"),
		CParticle_Point::Create(m_pDevice, m_pContext))))
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
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_UI_AttackRange"),
		CAttackRange::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_UI_MinionHpBar"),
		CMinion_HpBar::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_EnemyHpBar"),
		CEnemy_HpBar::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_BuildingHpBar"),
		CBuilding_HpBar::Create(m_pDevice, m_pContext))))
		return E_FAIL;
#pragma endregion 
#pragma region Champion
	/*For.Prototype_GameObject_Champion*/
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Ezreal"),
		CEzreal::Create(m_pDevice, m_pContext))))
		return E_FAIL;

#pragma endregion

#pragma region Enemy
	/*For.Prototype_GameObject_Champion*/
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Vayne"),
		CVayne::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	/*For.Prototype_GameObject_Champion*/
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Renekton"),
		CRenekton::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	/*For.Prototype_GameObject_Champion*/
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Leona"),
		CLeona::Create(m_pDevice, m_pContext))))
		return E_FAIL;
#pragma endregion

#pragma region Projectile
	/*For.Prototype_GameObject_EzrealProjectile*/
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_EzrealProjectile"),
		CEzrealProjectile::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	/*For.Prototype_GameObject_EzrealProjectile*/
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Bow"),
		CBow::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	/*For.Prototype_GameObject_EzrealProjectile*/
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_MinionProjectile"),
		CMinionProjectile::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	/*For.Prototype_GameObject_EzrealProjectile*/
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_TurretProjectile"),
		CTurretProjectile::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	/*For.Prototype_GameObject_VayneProjectile*/
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_VayneProjectile"),
		CVayneProjectile::Create(m_pDevice, m_pContext))))
		return E_FAIL;
#pragma endregion

#pragma region Building
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Nexus"),
		CNexus::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Inhibitor"),
		CInhibitor::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(TEXT("Prototype_GameObject_Turret"),
		CTurret::Create(m_pDevice, m_pContext))))
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

	/* For.Prototype_Component_Shader_Effect*/
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Shader_Effect"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Shaderfiles/Shader_Effect.hlsl"), VTXTEXALPHA_DECLARATION::Elements, VTXTEXALPHA_DECLARATION::iNumElements))))
		return E_FAIL;

	// For Prototype Component Shader VtxNoAnim
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxModel"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../../../Bin/ShaderFiles/Shader_VtxModel.hlsl"), VTXNORTEX_DECLARATION::Elements, VTXNORTEX_DECLARATION::iNumElements))))
		return E_FAIL;

	// For Prototype Component Shader VtxAnim
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxAnimModel"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../../../Bin/ShaderFiles/Shader_VtxAnimModel.hlsl"), VTXANIMMODEL_DECLARATION::Elements, VTXANIMMODEL_DECLARATION::iNumElements))))
		return E_FAIL;

	/* For.Prototype_Component_Shader_VtxRectInstance */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxRectInstance"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Shaderfiles/Shader_VtxRectInstance.hlsl"), VTXRECTINSTANCE_DECLARATION::Elements, VTXRECTINSTANCE_DECLARATION::iNumElements))))
		return E_FAIL;

	/* For.Prototype_Component_Shader_VtxPointInstance */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxPointInstance"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Shaderfiles/Shader_VtxPointInstance.hlsl"), VTXPOINTINSTANCE_DECLARATION::Elements, VTXPOINTINSTANCE_DECLARATION::iNumElements))))
		return E_FAIL;

	/* For.Prototype_Component_Shader_VtxCubeTexture */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxCubeTexture"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../../../Bin/ShaderFiles/Shader_VtxCubeTexture.hlsl"), VTXCUBETEX_DECLARATION::Elements, VTXCUBETEX_DECLARATION::iNumElements))))
		return E_FAIL;

	/* For.Prototype_Component_Shader_VtxPoint */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxPoint"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Shaderfiles/Shader_VtxPoint.hlsl"), VTXPOINT_DECLARATION::Elements, VTXPOINT_DECLARATION::iNumElements))))
		return E_FAIL;

#pragma region 셰이더 로딩


#pragma region 버퍼 로딩
	lstrcpy(m_szLoadingText, TEXT("버퍼 로딩 중"));
	m_fProgress = 10.f;
	/*For.Prototype_Component_VIBuffer_Cube */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_VIBuffer_Cube"),
		CVIBuffer_Cube::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	/*For.Prototype_Component_VIBuffer_Quad */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_VIBuffer_Quad"),
		CVIBuffer_Quad::Create(m_pDevice, m_pContext))))
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
	// 바이
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Vi"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Vi/Vi.dat"))))
		return E_FAIL;

	// 베인
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Vayne"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Vayne/Vayne.dat"))))
		return E_FAIL;

	// 레넥톤
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Renekton"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Renekton/Renekton.dat"))))
		return E_FAIL;

	// 레오나
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Leona"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Leona/Leona.dat"))))
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

	// 화살
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("BowMesh"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Bow/Bow.dat"))))
		return E_FAIL;

	lstrcpy(m_szLoadingText, TEXT("건물 로딩 중"));
	m_fProgress = 70.f;
	// 넥서스
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Nexus_Blue_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Buildings/Nexus_Blue/Nexus_Blue.dat"))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Nexus_Red_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Buildings/Nexus_Red/Nexus_Red.dat"))))
		return E_FAIL;
	// 포탑
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Turret_Blue_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Buildings/Turret_Blue/Turret_Blue.dat"))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Turret_Red_NonAnim"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_ANIM, "../../../Bin/Bin_Data/Anim/Champion/Buildings/Turret_Red/Turret_Red.dat"))))
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
#pragma 
	
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Colider_AABB"),
		CColider::Create(m_pDevice, m_pContext, CColider::TYPE_AABB))))
		return E_FAIL;

	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Colider_OBB"),
		CColider::Create(m_pDevice, m_pContext, CColider::TYPE_OBB))))
		return E_FAIL;

	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Prototype_Component_Colider_Sphere"),
		CColider::Create(m_pDevice, m_pContext, CColider::TYPE_SPHERE))))
		return E_FAIL;


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
	// Champion Attack Range 표시
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_AttackRange"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/SkillRange/SkillRange.dds"), 1))))
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

#pragma region  HPBAR
	// RED_Minion
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_Minion_HpBar"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/HpBar/Minion_HpBar.dds"), 1))))
		return E_FAIL;
	// Blue Minion
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_Minion_Minion_HpBar_Blue"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/HpBar/Minion_HpBar_Blue.dds"), 1))))
		return E_FAIL;
	// Frame
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_Minion_Minion_HpBar_Frame"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/HpBar/Minion_HpBarFrame.dds"), 1))))
		return E_FAIL;
	// Champion Frame
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_Champion_HpBar_Frame"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/HpBar/ChampionHpBarFrame.dds"), 1))))
		return E_FAIL;

	// Enemy HpBar
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_Enemy_HpBar"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/HpBar/EnemyHpBar.dds"), 1))))
		return E_FAIL;

	// Building Frame
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_Building_HpBar_Frame"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/HpBar/BuildingHpBarFrame.dds"), 1))))
		return E_FAIL;

	// Builindg HpBar
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_Building_HpBar"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/UI/HpBar/BuildingHpBar.dds"), 1))))
		return E_FAIL;
#pragma endregion
	return S_OK;
}

HRESULT CLoader::Loading_ForEffect()
{

	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Plane"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Plane/Plane.dat"))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("HalfSphere"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Sphere/HalfSphere.dat"))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Sphere"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Sphere/Sphere.dat"))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Bow"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Bow/Bow.dat"))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Cylinder0"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Cylinder/Cylinder0.dat"))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Cylinder1"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Cylinder/Cylinder1.dat"))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Cylinder2"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Cylinder/Cylinder2.dat"))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("CylinderSphere"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Cylinder/CylinderSphere.dat"))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("CylinderWind"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Cylinder/CylinderWind.dat"))))
		return E_FAIL;

	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("FloorCircle"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Plane/FloorCircle.dat"))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Ring"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Ring/Ring.dat"))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Slash"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Slash/Slash.dat"))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Trail"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Trail/Trail.dat"))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Sphere0"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Sphere/Sphere0.dat"))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("HalfSphere_ak_04"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Sphere/HalfSphere_ak_04.dat"))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Akizame"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Akizame/Akizame.dat"))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Arrow"),
		CModel::Create(m_pDevice, m_pContext, CModel::TYPE_NONANIM, "../../../Bin/Bin_Data/Effect/Arrow/Arrow.dat"))))
		return E_FAIL;

	return S_OK;
}

HRESULT CLoader::Loading_ForEffectTexture()
{

	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Aura_ak_03"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/Aura.png"), 1))))
		return E_FAIL;

	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Common_Noise_02_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/NoiseNormal.png"), 1))))
		return E_FAIL;

	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Spark_00"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Spark.png"), 1))))
		return E_FAIL;

	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Distortion_Noise"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Distortion_Noise.png"), 1))))
		return E_FAIL;

	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Height_Gradation"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/Height_Gradation.png"), 1))))
		return E_FAIL;

	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Height_Gradation2"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/Height_Gradation2.png"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Height_Gradation6"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/Height_Gradation6.png"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Height_Gradation5"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/Height_Gradation5.png"), 1))))
		return E_FAIL;

	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_512_ny_03"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/Noise4.png"), 1))))
		return E_FAIL;

	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Line_hn_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Line.png"), 1))))
		return E_FAIL;

	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_Common_WaveNoise_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/WaveNoise.png"), 1))))
		return E_FAIL;


	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Height_Gradation4"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Gradation/Height_Gradation4.png"), 1))))
		return E_FAIL;

	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_SO_AlphaC02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/NoiseAlpha.png"), 1))))
		return E_FAIL;

	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_SO_AlphaC03"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/NoiseAlpha2.png"), 1))))
		return E_FAIL;


	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("Default_Mask"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Default_Mask.png"), 1))))
		return E_FAIL;

	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("TO14_T_FX_noise_256_ny_10"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/Noise3.png"), 1))))
		return E_FAIL;

	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("HolyLance_Trail01_M"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Aura/Trail.png"), 1))))
		return E_FAIL;


#pragma region Ezreal Effect
	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("common_color-hold"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/common_color-hold.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("common_crystal-flash"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/common_crystal-flash.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("common_flare-rainbow_blue_green_02"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/common_flare-rainbow_blue_green_02.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("common_glow-soft"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/common_glow-soft.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_ashe_base_ground_cracknoise_mult"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_ashe_base_ground_cracknoise_mult.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_ba_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_ba_glow.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_ba_mis"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_ba_mis.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_ba_whisps"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_ba_whisps.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_q_erode"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_q_erode.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_q_hit_spark"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_q_hit_spark.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_q_mis_trail"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_q_mis_trail.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_r_bow_arrow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_r_bow_arrow.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_r_bow_solid_texutre"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_r_bow_solid_texutre.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_r_bow_tex"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_r_bow_tex.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_r_mis_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_r_mis_glow.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_r_tail_tex"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_r_tail_tex.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_r_trail"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_r_trail.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_w_cas_tex"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_w_cas_tex.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_w_meshlines"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_w_meshlines.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_w_orb"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_w_orb.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_w_ringglow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_w_ringglow.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_w_spintex"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_w_spintex.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_z_ball01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_z_ball01.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_z_color-beacontrail"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_z_color-beacontrail.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_z_color-hold"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_z_color-hold.png"), 1))))
		return E_FAIL;


	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_z_flare-sun"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_z_flare-sun.png"), 1))))
		return E_FAIL;


	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_z_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_z_glow.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_z_glowflare"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_z_glowflare.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_z_ring"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_z_ring.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_z_sliver"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_z_sliver.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_base_z_solar_tag"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_base_z_solar_tag.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezreal_r_cas_pulse"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezreal_r_cas_pulse.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezvgu_tristana_skin12_ba_rings"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezvgu_tristana_skin12_ba_rings.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezvgu_tristana_skin12_z_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezvgu_tristana_skin12_z_glow.png"), 1))))
		return E_FAIL;

	/* Effect Textures */
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("ezvgu_tristana_skin12_z_praxiswave"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Ezreal/ezvgu_tristana_skin12_z_praxiswave.png"), 1))))
		return E_FAIL;

#pragma endregion 

#pragma region Vayne
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("aura_self"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/aura_self.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("base_alphaslice_1"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/base_alphaslice_1.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("color-hold"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/color-hold.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("colorcommon_alpha_02hold"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/common_alpha_02.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("common_bats32"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/common_bats32.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("common_color-bellcurve4"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/common_color-bellcurve4.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("common_color-eyefade32"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/common_color-eyefade32.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("common_color-hit-physical"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/common_color-hit-physical.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("common_color-smoke32"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/common_color-smoke32.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("common_kogmaw_skin05_tar_flash"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/common_kogmaw_skin05_tar_flash.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("common_sand_04"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/common_sand_04.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("distort-pinch"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/distort-pinch.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_ba_erosionpack01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_ba_erosionpack01.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_ba_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_ba_glow.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_ba_lensflare"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_ba_lensflare.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_ba_smokeshape01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_ba_smokeshape01.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_ba_smoketip"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_ba_smoketip.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_ba_wispysmoke"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_ba_wispysmoke.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_e_erosionpack01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_e_erosionpack01.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_e_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_e_glow.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_e_lensflare"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_e_lensflare.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_e_smokeshape01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_e_smokeshape01.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_e_smoketip"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_e_smoketip.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_e_tar_terrain_dust"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_e_tar_terrain_dust.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_e_terrain_dirt"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_e_terrain_dirt.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_flames2"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_flames2.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_glow.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_hit_spark"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_hit_spark.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_noise01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_noise01.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_noise2"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_noise2.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_noise04"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_noise04.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_omniflare"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_omniflare.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_p_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_p_glow.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_particles_1_204"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_particles_1_204.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_particles_1_399"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_particles_1_399.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_q_erosionpack01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_q_erosionpack01.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_q_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_q_glow.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_q_wispysmoke"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_q_wispysmoke.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_r_erosionpack01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_r_erosionpack01.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_r_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_r_glow.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_r_ground_03"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_r_ground_03.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_r_ground_04"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_r_ground_04.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_r_lensflare"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_r_lensflare.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_r_smokeshape01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_r_smokeshape01.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_r_smoketip"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_r_smoketip.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_r_wispysmoke"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_r_wispysmoke.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_sparkles01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_sparkles01.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_taunt_flecks"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_taunt_flecks.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_w_dust"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_w_dust.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_w_erode"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_w_erode.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_w_erosionpack01"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_w_erosionpack01.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_w_flare"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_w_flare.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_w_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_w_glow.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_w_linesbigmedsmall"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_w_linesbigmedsmall.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_w_ring_1"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_w_ring_1.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_w_ring_2"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_w_ring_2.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_w_ring_3"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_w_ring_3.dds"), 1))))
		return E_FAIL;
	if (FAILED(m_pGameInstnace->Add_Prototype(LEVEL_STATIC, TEXT("vayne_base_w_ring_glow"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../../Bin/Resources/Textures/Effect/Champion/Vayne/vayne_base_w_ring_glow.dds"), 1))))
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
