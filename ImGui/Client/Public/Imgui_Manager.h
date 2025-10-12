#pragma once
#include "Base.h"
#include "Client_Defines.h"

#include "Level_Manager.h"
#include "ModelManager.h"
#include "NonAnim.h"

NS_BEGIN(Engine)
class CGameObject;
NS_END

NS_BEGIN(Client)
class CImGui_Manager :
    public CBase
{
    DECLARE_SINGLETON(CImGui_Manager)

public:
    enum PICKING_TYPE {PICKING_MODEL};

    enum COLOR {RED, GREEN, BLUE};
    enum CAMERATYPE {ACTION};

private:
    CImGui_Manager();
    virtual ~CImGui_Manager() = default;

public:
    HRESULT Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    void Update(_float fTimeDelta);
    void Update_Imgui();
    void Render();

public:
public:
    PICKING_TYPE Get_PickingType() { return (PICKING_TYPE)m_PickingType; }

public:
    // Debug
    void ShowSimpleMousePos(bool* p_open);
    void ShowPickedObj();
    void Show_PopupBox();

    /* For Model Tool */
    void Set_Object_Map();
    void Set_LayerTag();
    void Set_Macro();
    void Show_ModelList();
    void Show_CurrentModelList();
    void BrowseForFolder();

    void Set_TrasureBox();
    void Save_TrasureBox();
    void Load_TrasureBox();

public:
    void Create_Model(const _tchar* pPrototypeTag, const _tchar* pLayerTag, _bool bCreatePrototype = false);
    void Add_PrototypeTag(_tchar* TempTag) { m_TempLayerTags.push_back(TempTag); }
    void Add_PrototypeTag(char* TempTag) { m_TempCharTags.push_back(TempTag); }

private:
    ID3D11Device* m_pDevice = { nullptr };
    ID3D11DeviceContext* m_pContext = { nullptr };
    _bool m_bShowSimpleMousePos = false;
    _bool m_bShowPickedObject = false;
    _bool m_bShow_app_style_editor = false;
    _bool m_bSave = false;
    _bool m_bLoad = false;
    LEVEL m_iCurrentLevel = LEVEL_GAMEPLAY;

    // For Picking
    _float3     m_vPickedObjPos = _float3(1.f, 1.f, 1.f);
    _float3     m_vPickedObjScale = _float3(1.f, 1.f, 1.f);
    _int     m_PickingType   = 0;
    _float3									m_vPickedRotAxis = _float3(0.f, 1.f, 0.f);
    _float									m_fRotAngle = 0.f;

    /* For Object */
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


public:
    virtual void Free() override;
};
NS_END

