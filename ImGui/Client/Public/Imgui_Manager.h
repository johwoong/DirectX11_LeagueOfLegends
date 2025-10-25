#pragma once
#include "Base.h"
#include "Client_Defines.h"

#include "Level_Manager.h"
#include "ModelManager.h"
#include "TerrianManager.h"
#include "NonAnim.h"

NS_BEGIN(Engine)
class CGameObject;
NS_END

NS_BEGIN(Client)
class CImGui_Manager final : public CBase
{
    DECLARE_SINGLETON(CImGui_Manager)

public:
    enum PICKING_TYPE {
		PICKING_TERRAIN_TRANSFORM,
        PICKING_MODEL
    };

    enum COLOR { RED, GREEN, BLUE };
private:
    CImGui_Manager();
    virtual ~CImGui_Manager() = default;

public:
    HRESULT Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    void    Update(_float fTimeDelta);
    void    Render();
    void    Update_Imgui();
public:
	PICKING_TYPE Get_PickingType() { return (PICKING_TYPE)m_PickingType; }

public:
	/* For Debug*/
	void ShowSimpleMousePos(bool* p_open);
	void ShowPickedObj();
	void Show_PopupBox();

	// For Terrian Tool
	void Set_Terrian_Map();
	void Save_Terrian();
	void Load_Terrian();


	/* For Model Tool */
	void Set_Object_Map();
	void Set_LayerTag();
	void Set_Macro();
	void Show_ModelList();
	void Show_CurrentModelList();
	void BrowseForFolder();

	void Set_Play(_bool bPlay) { m_bIsPlaying = bPlay; }

	/* For Animation*/
	void Set_Animation();

public:
	void Create_Model(const _tchar* pPrototypeTag, const _tchar* pLayerTag, _bool bCreatePrototype = false);
	void Add_PrototypeTag(_tchar* TempTag) { m_TempLayerTags.push_back(TempTag); }
	void Add_PrototypeTag(char* TempTag) { m_TempCharTags.push_back(TempTag); }

private:
	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;
	_bool m_bShowSimpleMousePos = false;
	_bool m_bShowPickedObject = false;
	_bool m_bShow_app_style_editor = false;
	_bool m_bSave = false;
	_bool m_bLoad = false;
	LEVEL m_iCurrentLevel = LEVEL_GAMEPLAY;

private:
	// For Terrian
	CTerrianManager* m_pTerrianManager = { nullptr };
	CTerrianManager::TERRIANDESC		m_pTerrianDesc;
	_bool								m_bTerrian_Show = true;
	_bool									m_bUpdateTerrain = false;


    //  For Object
	CModelManager* m_pModel_Manager = nullptr;
	NONANIMDESC								m_InitDesc;
	_tchar									m_pFilePath[MAX_PATH] = L"../../../Bin/Resources/Meshes/";
	vector<string>							m_stLayerTags;
	vector<const _tchar*>					m_TempLayerTags;
	vector<const _tchar*>					m_TerrainTags;
	vector<char*>							m_TempCharTags;
	_int									m_iCreatedSelected = 0;
	_int									m_iSeletecLayerNum = 0;
	_int									m_iBmpTerrainNum = 0;
	_float									m_fDist = 1.f;
	_float3									m_vfOffset = _float3(0.f, 0.f, 0.f);
	_bool									m_bRandomX = false;
	_bool									m_bRandomY = false;
	_bool									m_bRandomZ = false;

	/* For Picking */
	_float3									m_vPickedObjPos = _float3(1.f, 1.f, 1.f);
	_float3									m_vPickedObjScale = _float3(1.f, 1.f, 1.f);
	_int									m_PickingType = 0;
	_float3									m_vPickedRotAxis = _float3(0.f, 1.f, 0.f);
	_float									m_fRotAngle = 0.f;

	/* For. Animation */
public:
	_bool m_isLoad = false;
	_bool m_isSecondLoad = false;
	_bool m_isSaveComplete = false;

	vector<string> m_AnimObj;
	vector<string> m_WeaponObj;

	_int m_iLoadModel = -1;
	_int m_iLoadSecondModel = -1;
	_int m_iPreModel = -1;
	_int m_iPreSecondModel = -1;

	_int m_iBoneChoice = -1;
	_int m_iAnimationChoice = -1;
	_int m_iSecondAnimationChoice = -1;

	_float m_fAnimationDuration = 0.f;
	_float m_fAnimationTime = 0.f;
	_float m_fSecondAnimationDuration = 0.f;
	_float m_fSecondAnimationTime = 0.f;

	_bool m_isAnimationPlay = false;
	_bool m_isAnimationEdit = false;
	_bool m_isSecondAnimationPlay = false;
	_bool m_isStrikePlay = false;

	_int m_iTickChoice = -1;
	_int m_iTimeChoice = -1;

	_float m_fAnimationTickTime = 0.f;
	_float m_fAnimationTick = 0.f;

	_bool m_isWeaponWindow = false;
	_int m_iWeaponChoice = -1;

	_bool m_isEventWindow = false;
	_float m_fEventStartTime = 0.f;
	_float m_fEventEndTime = 0.f;
	_int m_iEventType = 0;
	_int m_iEventChoice = -1;

	_float m_Position[3] = { 0.f, 0.f, 0.f };
	_float m_Rotation[3] = { 0.f, 0.f, 0.f };

	_float m_SecondPosition[3] = { 0.f, 0.f, 0.f };
	_float m_SecondRotation[3] = { 0.f, 0.f, 0.f };

	// Custom
	_bool m_bIsPlaying = false;
	vector<string> m_SpawnTypes = { "LOOP", "BURST" };
	string m_sCurrentSpawnType = "LOOP";

	string m_sSelectedMaskTexture = "";
	string m_sSelectedNoiseTexture = "";

	_float m_fCurveValue = 1.f, m_fCurveStart = 1.f, m_fCurveEnd = 1.f;
	_float m_fCurveRed = 1.f, m_fCurveGreen = 1.f, m_fCurveBlue = 1.f;
	_float m_fCurveScaleX = 1.f, m_fCurveScaleY = 1.f, m_fCurveScaleZ = 1.f;
	_uint m_iSelectedColorCurve = 0, m_iSelectedVelocityCurve = 0, m_iSelectedSizeCurve = 0, m_iSelectedScaleCurve = 0, m_iSelectedAlphaCurve = 0,
		m_iSelectedTurnVelocityCurve = 0, m_iSelectedNoisePowerCurve = 0, m_iSelectedDistortPowerCurve = 0;
	_float m_fEditorAlpha = 0.9f;
public:
	virtual void Free() override;
};

NS_END

