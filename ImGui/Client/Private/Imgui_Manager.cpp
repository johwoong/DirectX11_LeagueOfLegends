#include "pch.h"
#include "Imgui_Manager.h"
#include <windows.h>
#include <string.h>

#include <shlobj.h>
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include "GameInstance.h"
#include "ModelManager.h"
#include "NonAnim.h"
#include "Anim.h"
#include "PickingMgr.h"
#include "Actor.h"
#include "Animation.h"

IMPLEMENT_SINGLETON(CImGui_Manager)

typedef basic_string<TCHAR> tstring;

TCHAR* StringToTCHAR(string& s)
{
	tstring tstr;
	const char* all = s.c_str();
	int len = 1 + (int)strlen(all);
	wchar_t* t = new wchar_t[len];
	if (NULL == t) throw std::bad_alloc();
	mbstowcs(t, all, len);
	return (TCHAR*)t;
}

string TCHARToString(const TCHAR* ptsz)
{
	int len = (int)wcslen((wchar_t*)ptsz);
	char* psz = new char[2 * len + 1];
	wcstombs(psz, (wchar_t*)ptsz, 2 * len + 1);
	std::string s = psz;
	delete[] psz;
	return s;
}

string ToString(wstring value)
{
	string temp;
	temp.assign(value.begin(), value.end());
	return temp;
}

vector<string> SplitPath(string path, char sep) {
	vector<string> out;
	stringstream stream(path);
	string temp;

	while (getline(stream, temp, sep)) {
		out.push_back(temp);
	}

	return out;
}

string CutOnSpecificDecimalPt(string num, int pos)
{
	return num.substr(0, num.find('.') + pos + 1);
}

CImGui_Manager::CImGui_Manager()
	: m_pTerrianManager(CTerrianManager::GetInstance())
	, m_pModel_Manager(CModelManager::GetInstance())
{
	Safe_AddRef(m_pModel_Manager);

	const char* LayerTag = "Model";
	const char* LayerTag2 = "Layer_Terrain";
	m_stLayerTags.push_back(LayerTag);
	m_stLayerTags.push_back(LayerTag2);
}

HRESULT CImGui_Manager::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	m_TempLayerTags.reserve(0);

	m_pDevice = pDevice;
	m_pContext = pContext;
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;


	// Style Setting
	io.FontDefault = io.Fonts->AddFontFromFileTTF("../../../Bin/Resources/Fonts/Quicksand-Medium.ttf", 16.0f);
	Apply_UnityDark_Theme();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(g_hWnd);
	ImGui_ImplDX11_Init(m_pDevice, m_pContext);
	
	
	return S_OK;
}

void CImGui_Manager::Update(_float fTimeDelta)
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	Update_Imgui();
	ImGui::EndFrame();
}

void CImGui_Manager::Render()
{
	// Rendering
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void CImGui_Manager::Update_Imgui()
{
	// Begin 실패 시 바로 return 해야 함
	if (!ImGui::Begin(u8"Editor", NULL,
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_HorizontalScrollbar |
		ImGuiWindowFlags_AlwaysVerticalScrollbar))
	{
		ImGui::End();
		return;
	}

	ImGui::NewLine();

	// 메뉴바
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("save"))
				m_bSave = true;
			ImGui::Separator();
			if (ImGui::MenuItem("open"))
				m_bLoad = true;
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	static int iCurrentLevel = (int)m_iCurrentLevel;
	m_iCurrentLevel = (LEVEL)iCurrentLevel;

	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
	if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
	{
		if (ImGui::BeginTabItem("Terrian Tool"))
		{
			Set_Terrian_Map();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Model Tool"))
		{
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Picking for Object");
			ImGui::SameLine();
			ImGui::RadioButton("##Picking for Object", &m_PickingType, PICKING_MODEL);

			if (ImGui::BeginTabBar("ModelsTabs", ImGuiTabBarFlags_None))
			{
				if (ImGui::BeginTabItem("Show Model List"))
				{
					Show_ModelList();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Scene"))
				{
					Show_CurrentModelList();
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar(); 
			}
			ImGui::EndTabItem();
		}

		//if (ImGui::BeginTabItem("Animation Tool"))
		//{
		//	Set_Animation();
		//	ImGui::EndTabItem();
		//}
		ImGui::EndTabBar(); 
	}

	if (m_bSave || m_bLoad)
		BrowseForFolder();

	if (m_bShow_app_style_editor)
	{
		ImGui::Begin("Dear ImGui Style Editor", &m_bShow_app_style_editor);
		ImGui::ShowStyleEditor();
		ImGui::End();
	}

	ImGui::End(); // 최종 End


}


void CImGui_Manager::ShowPickedObj()
{
	CPickingMgr* pPickingMgr = CPickingMgr::GetInstance();
	pPickingMgr->AddRef();
	CGameObject* pPickedObj = pPickingMgr->Get_PickedObj();


	if (pPickedObj != nullptr)
	{
		DirectX::XMStoreFloat3(&m_vPickedObjPos, dynamic_cast<CActor*>(pPickedObj)->Get_Position());
		m_vPickedObjScale = dynamic_cast<CActor*>(pPickedObj)->Get_Scale();

		if (dynamic_cast<CActor*>(pPickedObj)->Get_ObjectID() != OBJ_BACKGROUND)
		{

			m_vPickedRotAxis = dynamic_cast<CNonAnim*>(pPickedObj)->Get_ModelDesc().vRotation;
			m_fRotAngle = dynamic_cast<CNonAnim*>(pPickedObj)->Get_ModelDesc().m_fAngle;
		}

	}
	else
	{
		m_vPickedObjPos = _float3(0.f, 0.f, 0.f);
		m_vPickedObjScale = _float3(1.f, 1.f, 1.f);
		m_fRotAngle = 0.f;
	}


	ImGui::NewLine();
	ImGui::BulletText("Position");
	ImGui::Text("Position X");
	ImGui::SameLine();
	ImGui::DragFloat("##PositionX", &m_vPickedObjPos.x, 0.1f);

	ImGui::Text("Position Z");
	ImGui::SameLine();
	ImGui::DragFloat("##PositionZ", &m_vPickedObjPos.z, 0.01f);

	ImGui::Text("Position Y");
	ImGui::SameLine();
	ImGui::DragFloat("##PositionY", &m_vPickedObjPos.y, 0.01f);

	ImGui::NewLine();

	ImGui::BulletText("Scale");

	static _float Pos[3] = { m_vPickedObjScale.x, m_vPickedObjScale.y,  m_vPickedObjScale.z };
	Pos[0] = m_vPickedObjScale.x;
	Pos[1] = m_vPickedObjScale.y;
	Pos[2] = m_vPickedObjScale.z;

	ImGui::Text("Scale");
	ImGui::SameLine();
	ImGui::DragFloat3("##SettingScale", Pos);
	m_vPickedObjScale = _float3(Pos[0], Pos[1], Pos[2]);

	ImGui::BulletText("Rotation");
	static _float Rotation[3] = { m_vPickedRotAxis.x , m_vPickedRotAxis.y, m_vPickedRotAxis.z };
	Rotation[0] = m_vPickedRotAxis.x;
	Rotation[1] = m_vPickedRotAxis.y;
	Rotation[2] = m_vPickedRotAxis.z;
	ImGui::Text("Rotation axis"); ImGui::SameLine();
	ImGui::DragFloat3("##Setting RotAxis", Rotation);

	m_vPickedRotAxis = _float3(Rotation[0], Rotation[1], Rotation[2]);



	if (pPickedObj != nullptr && dynamic_cast<CActor*>(pPickedObj)->Get_ObjectID() == OBJ_BLOCK)
	{
		_vector vSettingPosition = DirectX::XMLoadFloat3(&m_vPickedObjPos);
		vSettingPosition = XMVectorSetW(vSettingPosition, 1.f);
		dynamic_cast<CActor*>(pPickedObj)->Set_State(CTransform::STATE_TRANSLATION, vSettingPosition);
		dynamic_cast<CActor*>(pPickedObj)->Set_Scale(m_vPickedObjScale);
		dynamic_cast<CNonAnim*>(pPickedObj)->Turn(m_vPickedRotAxis, 0.f);
	}

	if (ImGui::Button("Delete Object"))
	{
		CPickingMgr* pPickingMgr = CPickingMgr::GetInstance();
		pPickingMgr->AddRef();
		if (pPickingMgr->Get_PickedObj() != nullptr)
			ImGui::OpenPopup("Delete Object?");
		pPickingMgr->Release();

	}
	Show_PopupBox();
	pPickingMgr->Release();
}

void CImGui_Manager::Apply_UnityDark_Theme()
{
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 6.0f;
	style.FrameRounding = 5.0f;
	style.GrabRounding = 4.0f;
	style.PopupRounding = 5.0f;
	style.ScrollbarRounding = 6.0f;
	style.TabRounding = 5.0f;
	style.FrameBorderSize = 0.0f;

	style.WindowPadding = ImVec2(10, 10);
	style.FramePadding = ImVec2(8, 5);
	style.ItemSpacing = ImVec2(8, 6);
	style.ItemInnerSpacing = ImVec2(6, 4);
	style.ScrollbarSize = 14.0f;

	ImVec4* colors = style.Colors;

	// ===== Unity Dark Pro Style =====
	colors[ImGuiCol_Text] = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);

	colors[ImGuiCol_WindowBg] = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);

	colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 0.90f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

	colors[ImGuiCol_FrameBg] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.43f, 0.43f, 0.43f, 1.00f);

	colors[ImGuiCol_TitleBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.10f, 0.70f);

	colors[ImGuiCol_MenuBarBg] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.60f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);

	colors[ImGuiCol_Button] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);

	colors[ImGuiCol_Header] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);

	colors[ImGuiCol_Tab] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.43f, 0.43f, 0.43f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);

	colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);

	colors[ImGuiCol_Separator] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
}

void CImGui_Manager::Show_PopupBox()
{
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	bool unused_open = true;

	if (ImGui::BeginPopupModal("Delete Object?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		CPickingMgr* pPickingMgr = CPickingMgr::GetInstance();
		pPickingMgr->AddRef();

		CGameObject* pPickedObj = pPickingMgr->Get_PickedObj();
		CNonAnim* pNonAnim = dynamic_cast<CNonAnim*>(pPickedObj);
		ImGui::Text("Are yoou Sure Delete This Object?\n\n");
		ImGui::Separator();

		ImGui::Text("Object Name : ");  ImGui::SameLine(); ImGui::Text(pNonAnim->Get_Modeltag());

		if (ImGui::Button("OK", ImVec2(120, 0)))
		{
			m_iCreatedSelected = 0;
			pPickingMgr->Set_PickedObj(nullptr);
			m_pModel_Manager->Out_CreatedModel(pNonAnim);
			pPickedObj->Set_Dead(true);
			pPickingMgr->Out_PickingGroup(pNonAnim);

			_tchar* LayerTag = StringToTCHAR(m_stLayerTags[m_iSeletecLayerNum]);
			m_iCurrentLevel = (LEVEL)CGameInstance::GetInstance()->Get_CurrentLevelIndex();
			list<CGameObject*>* plistClone = CGameInstance::GetInstance()->Get_ObjectList(m_iCurrentLevel, LayerTag);
			if (nullptr == plistClone || plistClone->size() == 0)
			{
				delete LayerTag;
				return;
			}

			for (auto& iter : *plistClone)
			{
				if (iter == pNonAnim)
				{
					delete LayerTag;
				}
			}
			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();

		pPickingMgr->Release();
	}


	if (ImGui::BeginPopupModal("Delete Layer Tag?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("All those LayerTag files will be deleted.\nThis operation cannot be undone!\n\n");
		ImGui::Separator();

		static int unused_i = 0;
		ImGui::Combo("Combo", &unused_i, "Layer1\0Layer2\0");

		static bool dont_ask_me_next_time = false;
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::Checkbox("Don't ask me next time", &dont_ask_me_next_time);
		ImGui::PopStyleVar();

		if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}


	if (ImGui::BeginPopupModal("Add Layer Tag", NULL, ImGuiWindowFlags_MenuBar))
	{
		static char LayerBuffer[MAX_PATH] = "";

		ImGui::Text("Enter Layer Tag");
		ImGui::InputText("Layer Tag", LayerBuffer, MAX_PATH);

		if (ImGui::Button("Add Layer Tag"))
		{
			bool bFailed = false;

			if (m_stLayerTags.size() != 0)
			{
				for (auto& iter : m_stLayerTags)
				{
					if (!strcmp("", LayerBuffer) || !strcmp(iter.c_str(), LayerBuffer))
					{
						bFailed = true;
						ImGui::OpenPopup("Failed");
					}

				}
			}
			if (!bFailed)
			{
				string NewLayer = LayerBuffer;
				m_stLayerTags.push_back(NewLayer);
				ImGui::OpenPopup("Succese");
			}

		}

		if (ImGui::BeginPopupModal("Failed", &unused_open))
		{
			ImGui::Text("Failed Add Layer Tag");
			if (ImGui::Button("Close"))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
		if (ImGui::BeginPopupModal("Succese", &unused_open))
		{
			ImGui::Text("Succese");
			if (ImGui::Button("Close"))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}


		ImGui::SameLine();
		if (ImGui::Button("Close"))  ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
}

void CImGui_Manager::Set_Terrian_Map()
{
	ImGui::GetIO().NavActive = false;
	ImGui::GetIO().WantCaptureMouse = true;

	if (!ImGui::CollapsingHeader("Terrain Tool", ImGuiTreeNodeFlags_DefaultOpen))
		return;

	auto Section = [](const char* title)
		{
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::TextColored(ImVec4(1.0f, 0.95f, 0.3f, 1.0f), "%s", title);
			ImGui::Separator();
		};

	CTerrianManager::TERRIANDESC TerrainDesc = m_pTerrianManager->Get_TerrianDesc();
	static int selectedTerrain = -1;

	// --- Terrain 리스트 ---
	list<CGameObject*>* pTerrainList =
		CGameInstance::GetInstance()->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Terrain"));

	CTerrian* pSelectedTerr = nullptr;
	if (pTerrainList && selectedTerrain >= 0)
	{
		int idx = 0;
		for (auto& obj : *pTerrainList)
		{
			if (idx == selectedTerrain)
			{
				pSelectedTerr = dynamic_cast<CTerrian*>(obj);
				break;
			}
			++idx;
		}
	}

	// --- 선택된 Terrain 값 UI에 반영 ---
	if (pSelectedTerr)
	{
		CTransform* pTr = pSelectedTerr->Get_Transform();
		_float3 vPos; XMStoreFloat3(&vPos, pTr->Get_State(CTransform::STATE_TRANSLATION));
		TerrainDesc.TerrianDesc.m_iPositionX = (int)vPos.x;
		TerrainDesc.TerrianDesc.m_fHeight = vPos.y;
		TerrainDesc.TerrianDesc.m_iPositionZ = (int)vPos.z;
		TerrainDesc.TerrianDesc.m_fRotationX = pTr->Get_CurrentRotationX();
		TerrainDesc.TerrianDesc.m_fRotationY = pTr->Get_CurrentRotationY();
		TerrainDesc.TerrianDesc.m_fRotationZ = pTr->Get_CurrentRotationZ();
	}

	// ===================== Vertex =====================
	Section("Vertex");
	static int iNumVert[2] = {
		TerrainDesc.TerrianDesc.m_iVerticeNumX,
		TerrainDesc.TerrianDesc.m_iVerticeNumZ
	};

	ImGui::Columns(2, nullptr, false);
	ImGui::SetColumnWidth(0, 110);
	ImGui::Text("NumX / NumZ"); ImGui::NextColumn();
	ImGui::PushItemWidth(180);
	if (ImGui::InputInt2("##NumXZ", iNumVert))
	{
		TerrainDesc.TerrianDesc.m_iVerticeNumX = max(2, iNumVert[0]);
		TerrainDesc.TerrianDesc.m_iVerticeNumZ = max(2, iNumVert[1]);
	}
	ImGui::PopItemWidth();
	ImGui::Columns(1);

	// ===================== Transform =====================
	Section("Transform");
	bool anyChanged = false;
	ImGui::Columns(2, nullptr, false);
	ImGui::SetColumnWidth(0, 110);

	ImGui::Text("Position"); ImGui::NextColumn();
	ImGui::PushItemWidth(70);
	anyChanged |= ImGui::DragInt("X##Pos", &TerrainDesc.TerrianDesc.m_iPositionX, 0.5f);
	ImGui::SameLine();
	anyChanged |= ImGui::DragFloat("Y##Pos", &TerrainDesc.TerrianDesc.m_fHeight, 0.1f);
	ImGui::SameLine();
	anyChanged |= ImGui::DragInt("Z##Pos", &TerrainDesc.TerrianDesc.m_iPositionZ, 0.5f);
	ImGui::PopItemWidth();

	ImGui::NextColumn();
	ImGui::Text("Rotation"); ImGui::NextColumn();
	ImGui::PushItemWidth(70);
	anyChanged |= ImGui::DragFloat("X##Rot", &TerrainDesc.TerrianDesc.m_fRotationX, 0.5f);
	ImGui::SameLine();
	anyChanged |= ImGui::DragFloat("Y##Rot", &TerrainDesc.TerrianDesc.m_fRotationY, 0.5f);
	ImGui::SameLine();
	anyChanged |= ImGui::DragFloat("Z##Rot", &TerrainDesc.TerrianDesc.m_fRotationZ, 0.5f);
	ImGui::PopItemWidth();
	ImGui::Columns(1);

	// 선택된 Terrain 실시간 갱신
	if (anyChanged && pSelectedTerr)
	{
		CTransform* pTr = pSelectedTerr->Get_Transform();
		if (pTr)
		{
			pTr->Set_State(CTransform::STATE_TRANSLATION,
				XMVectorSet((float)TerrainDesc.TerrianDesc.m_iPositionX,
					TerrainDesc.TerrianDesc.m_fHeight,
					(float)TerrainDesc.TerrianDesc.m_iPositionZ, 1.f));

			pTr->Set_Rotation(_float3(
				TerrainDesc.TerrianDesc.m_fRotationX,
				TerrainDesc.TerrianDesc.m_fRotationY,
				TerrainDesc.TerrianDesc.m_fRotationZ));
		}
	}

	// ===================== Options =====================
	Section("Options");
	ImGui::Checkbox("WireFrame", &TerrainDesc.m_bShowWireFrame);
	ImGui::SameLine(160);
	ImGui::Checkbox("Show", &m_bTerrian_Show);

	m_pTerrianManager->Set_bWireFrame(TerrainDesc.m_bShowWireFrame);
	m_pTerrianManager->Set_TerrainShow(m_bTerrian_Show);

	ImGui::Spacing();
	ImGui::Separator();

	if (ImGui::Button("Create Terrain", ImVec2(180, 28)))
	{
		m_pTerrianManager->Set_TerrainDesc(&TerrainDesc);
		if (FAILED(m_pTerrianManager->Create_Terrain(LEVEL_GAMEPLAY, TEXT("Layer_Terrain"))))
			return;
	}

	// ===================== Terrain List =====================
	Section("Current Terrain List");
	ImGui::BeginChild("TerrainList", ImVec2(0, 160), true);
	if (pTerrainList)
	{
		int i = 0;
		for (auto& obj : *pTerrainList)
		{
			if (auto* pTerr = dynamic_cast<CTerrian*>(obj))
			{
				char label[64]; sprintf_s(label, "Terrain_%d", i);
				if (ImGui::Selectable(label, selectedTerrain == i))
					selectedTerrain = i;
			}
			++i;
		}
	}
	ImGui::EndChild();

	// ===================== Save / Load / Delete =====================
	Section("File Controls");
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 1.f, 1.f));
	if (ImGui::Button("Save Terrain", ImVec2(140, 28)))
		Save_Terrian();
	ImGui::PopStyleColor();

	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 1.f, 0.3f, 1.f));
	if (ImGui::Button("Load Terrain", ImVec2(140, 28)))
		Load_Terrian();
	ImGui::PopStyleColor();

	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.f, 0.3f, 0.3f, 1.f));
	if (ImGui::Button("Delete Selected", ImVec2(160, 28)))
	{
		if (pSelectedTerr)
		{
			pSelectedTerr->Set_Dead(true);
			pSelectedTerr = nullptr;
			selectedTerrain = -1;
		}
	}
	ImGui::PopStyleColor();

	// --- desc 반영 ---
	m_pTerrianManager->Set_TerrainDesc(&TerrainDesc);
}


void CImGui_Manager::Save_Terrian()
{
	OPENFILENAME OFN;
	TCHAR filePathName[300] = L"";
	TCHAR lpstrFile[300] = L"";
	static TCHAR filter[] = L"데이터 파일\0*.dat\0텍스트 파일\0*.txt";

	memset(&OFN, 0, sizeof(OPENFILENAME));
	OFN.lStructSize = sizeof(OPENFILENAME);
	OFN.hwndOwner = g_hWnd;
	OFN.lpstrFilter = filter;
	OFN.lpstrFile = lpstrFile;
	OFN.nMaxFile = 300;
	OFN.lpstrInitialDir = L".";
	OFN.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT;

	if (GetSaveFileName(&OFN))
	{
		HANDLE hFile = 0;
		_ulong dwByte = 0;
		_uint iNum = 0;
		_float3	 vPoints[3];
		list<CGameObject*>* pTerrainList = CGameInstance::GetInstance()->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Terrain"));


		iNum = (_int)pTerrainList->size();

		hFile = CreateFile(OFN.lpstrFile, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);;
		if (0 == hFile)
			return;

		WriteFile(hFile, &(iNum), sizeof(_uint), &dwByte, nullptr);

		for (auto& iter : *pTerrainList)
		{
			dynamic_cast<CTerrian*>(iter)->Save_Terrain(hFile, &dwByte);
		}

		CloseHandle(hFile);
	}
}

void CImGui_Manager::Load_Terrian()
{
	OPENFILENAME OFN;
	TCHAR filePathName[300] = L"";
	TCHAR lpstrFile[300] = L"";
	static TCHAR filter[] = L"데이터 파일\0*.dat\0텍스트 파일\0*.txt";

	memset(&OFN, 0, sizeof(OPENFILENAME));
	OFN.lStructSize = sizeof(OPENFILENAME);
	OFN.hwndOwner = g_hWnd;
	OFN.lpstrFilter = filter;
	OFN.lpstrFile = lpstrFile;
	OFN.nMaxFile = 300;
	OFN.lpstrInitialDir = L".";

	if (GetOpenFileName(&OFN) == 0)
		return;

	wsprintf(filePathName, L"%s 파일을 열겠습니까?", OFN.lpstrFile);
	MessageBox(g_hWnd, filePathName, L"열기 선택", MB_OK);

	HANDLE hFile = CreateFile(OFN.lpstrFile, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE)
		return;

	CGameInstance* pGameInstance = CGameInstance::GetInstance();
	pGameInstance->AddRef();

	_ulong dwByte = 0;
	_uint iNum = 0;
	ReadFile(hFile, &iNum, sizeof(_uint), &dwByte, nullptr);

	for (_uint i = 0; i < iNum; ++i)
	{
		_tchar szFullPath[MAX_PATH] = TEXT("Prototype_Component_VIBuffer_Terrain_Load");

		// Terrain 생성
		CVIBuffer_Terrian* pBuffer = m_pTerrianManager->Create_Terrain(m_pDevice, m_pContext, hFile, dwByte);
		pGameInstance->Add_Prototype(LEVEL_STATIC, szFullPath, pBuffer);
		
		pGameInstance->Add_GameObjectLoad(TEXT("Prototype_GameObject_Terrain"), LEVEL_GAMEPLAY, TEXT("Layer_Terrain"), szFullPath);

		// 이제 Terrain 버텍스 데이터 바로 다음 위치에 Transform 정보가 붙어있음
		_float3 vPos{}, vRot{};
		DWORD bytesRead = 0;
		ReadFile(hFile, &vPos, sizeof(_float3), &bytesRead, nullptr);
		ReadFile(hFile, &vRot, sizeof(_float3), &bytesRead, nullptr);

		list<CGameObject*>* pTerrainList =
			pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Terrain"));

		if (pTerrainList && !pTerrainList->empty())
		{
			CTerrian* pTerrain = dynamic_cast<CTerrian*>(pTerrainList->back());
			if (pTerrain)
			{
				CTransform* pTransform = pTerrain->Get_Transform();
				pTransform->Set_State(CTransform::STATE_TRANSLATION,
					XMVectorSet(vPos.x, vPos.y, vPos.z, 1.f));
				pTransform->Set_Rotation(vRot);
			}
		}
	}

	CloseHandle(hFile);
	pGameInstance->Release();
}

void CImGui_Manager::Set_Object_Map()
{
	ImGui::NewLine();
	ImGui::BulletText("Transform Setting");

	static float Position[3] = { m_InitDesc.vPosition.x, m_InitDesc.vPosition.y, m_InitDesc.vPosition.z };
	ImGui::Text("Position"); ImGui::SameLine();
	ImGui::InputFloat3("##1", Position);
	
	static _float OffsetPos[3] = { m_vfOffset.x, m_vfOffset.y, m_vfOffset.z };
	ImGui::Text("Offset"); ImGui::SameLine();
	ImGui::InputFloat3("##PosOffset", OffsetPos);
	m_vfOffset = _float3(OffsetPos[0], OffsetPos[1], OffsetPos[2]);
	m_InitDesc.vPosition = _float3(Position[0] + OffsetPos[0], Position[1] + OffsetPos[1], Position[2] + OffsetPos[2]);

	static _float Scale[3] = { m_InitDesc.vScale.x , m_InitDesc.vScale.y, m_InitDesc.vScale.z };
	ImGui::Text("Scale"); ImGui::SameLine();
	ImGui::InputFloat3("##Scale", Scale);
	m_InitDesc.vScale = _float3(Scale[0], Scale[1], Scale[2]);

	static _float Rotation[3] = { m_InitDesc.vRotation.x , m_InitDesc.vRotation.y, m_InitDesc.vRotation.z };
	ImGui::Text("Rotation axis"); ImGui::SameLine();
	ImGui::DragFloat3("##2", Rotation);

	m_InitDesc.vRotation = _float3(Rotation[0], Rotation[1], Rotation[2]);
}

void CImGui_Manager::Set_LayerTag()
{
	//Layer tag를 만들거나, 삭제한다. 
	ImGui::BulletText("Layer Setting");

	if (ImGui::BeginCombo("Layer List", m_stLayerTags[m_iSeletecLayerNum].c_str()))
	{
		for (int i = 0; i < m_stLayerTags.size(); i++)
		{
			if (ImGui::Selectable(m_stLayerTags[i].c_str(), m_iSeletecLayerNum == i))
				m_iSeletecLayerNum = i;
		}
		ImGui::EndCombo();
	}
	if (ImGui::Button("Delete Layer Tag"))
		ImGui::OpenPopup("Delete Layer Tag?"); ImGui::SameLine();

	if (ImGui::Button("Add New Layer Tag"))
		ImGui::OpenPopup("Add Layer Tag");

	Show_PopupBox();
}

void CImGui_Manager::Show_ModelList()
{
	vector<const _tchar*> ModelTags = m_pModel_Manager->Get_PrototypeTag();

	// ------------------------ Left-----------------------------------
	static int selected = 0;
	{
		ImGui::BeginChild("left pane", ImVec2(200, 0), true);

		int i = 0;

		if (ModelTags.size() != 0)
		{
			for (auto& iter : ModelTags)
			{
				if (iter == nullptr)
					continue;
				char label[MAX_PATH];
				char szLayertag[MAX_PATH] = "";
				WideCharToMultiByte(CP_ACP, 0, iter, MAX_PATH, szLayertag, MAX_PATH, NULL, NULL);
				sprintf(label, szLayertag);
				if (ImGui::Selectable(label, selected == i))
					selected = i;
				i++;
			}
		}
		ImGui::EndChild();
	}
	ImGui::SameLine();

	// ------------------------ Right -----------------------------------
	{
		ImGui::BeginGroup();
		ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
		char szLayertag[MAX_PATH] = "";
		if (ModelTags.size() != 0)
			WideCharToMultiByte(CP_ACP, 0, ModelTags[selected], MAX_PATH, szLayertag, MAX_PATH, NULL, NULL);
		ImGui::Text("Selected :"); ImGui::SameLine();  ImGui::Text(szLayertag);
		ImGui::Separator();
		if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem("Setting"))
			{
				ImGui::NewLine();
				Set_LayerTag();
				Set_Object_Map();
				ImGui::EndTabItem();

				if (ImGui::Button("Add_Model"))
				{
					_tchar* LayerTag = StringToTCHAR(m_stLayerTags[m_iSeletecLayerNum]);
					Create_Model(ModelTags[selected], LayerTag);
					m_TempLayerTags.push_back(LayerTag);
				}
			}
			ImGui::EndTabBar();
		}

		ImGui::EndChild();
		ImGui::EndGroup();
	}


	if (m_PickingType == PICKING_MODEL && CGameInstance::GetInstance()->Key_Up(DIK_X))
	{
		if (CPickingMgr::GetInstance()->Picking())
		{
			m_InitDesc.vPosition = CPickingMgr::GetInstance()->Get_PickingPos();
			m_InitDesc.vPosition.x += m_vfOffset.x;
			m_InitDesc.vPosition.y += m_vfOffset.y;
			m_InitDesc.vPosition.z += m_vfOffset.z;
			_tchar* LayerTag = StringToTCHAR(m_stLayerTags[m_iSeletecLayerNum]);
			Create_Model(ModelTags[selected], LayerTag, true);
			m_TempLayerTags.push_back(LayerTag);

		}
	}

	if (m_PickingType == PICKING_MODEL && CGameInstance::GetInstance()->Key_Up(DIK_Z))
	{
		_tchar* LayerTag = StringToTCHAR(m_stLayerTags[m_iSeletecLayerNum]);
		m_iCurrentLevel = (LEVEL)CGameInstance::GetInstance()->Get_CurrentLevelIndex();
		list<CGameObject*>* plistClone = CGameInstance::GetInstance()->Get_ObjectList(m_iCurrentLevel, LayerTag);
		if (nullptr == plistClone || plistClone->size() == 0)
		{
			delete LayerTag;
			return;
		}


		auto iter = --plistClone->end();
		CPickingMgr::GetInstance()->Out_PickingGroup(*iter);
		CPickingMgr::GetInstance()->Set_PickedObj(nullptr);
		m_pModel_Manager->Out_CreatedModel(dynamic_cast<CNonAnim*>(*iter));
		Safe_Release(*iter);
		plistClone->erase(iter);

		delete LayerTag;
	}
}

void CImGui_Manager::Show_CurrentModelList()
{
	vector<class CNonAnim*> vecCreatedModel = m_pModel_Manager->Get_CreatedModel();


	// Left
	{
		ImGui::BeginChild("Object List", ImVec2(150, 0), true);

		int i = 0;

		if (vecCreatedModel.size() != 0)
		{
			for (auto& iter : vecCreatedModel)
			{
				if (iter == nullptr)
					continue;

				char label[MAX_PATH] = "";
				char szLayertag[MAX_PATH] = "";

				string  ModelTag = iter->Get_Modeltag();
				ModelTag = ModelTag + "_" + to_string(i);
				_tchar* RealModelTag = StringToTCHAR(ModelTag);
				WideCharToMultiByte(CP_ACP, 0, RealModelTag, MAX_PATH, szLayertag, MAX_PATH, NULL, NULL);
				sprintf(label, szLayertag);
				delete(RealModelTag);
				if (ImGui::Selectable(label, m_iCreatedSelected == i))
				{
					m_iCreatedSelected = i;
					vecCreatedModel[i]->Set_Picked();
				}

				i++;
			}
		}
		ImGui::EndChild();
	}
	ImGui::SameLine();

	// Right
	{
		ImGui::BeginGroup();
		ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
		char szLayertag[MAX_PATH] = "";
		if (vecCreatedModel.size() != 0)
		{
			if (vecCreatedModel[m_iCreatedSelected] != nullptr)
				strcpy_s(szLayertag, MAX_PATH, vecCreatedModel[m_iCreatedSelected]->Get_Modeltag());
		}

		ImGui::Text(szLayertag);
		ImGui::Separator();
		if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem("LayerList"))
			{
				ShowPickedObj();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Details"))
			{
				ImGui::Text("ID: 0123456789");
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		ImGui::EndChild();
		if (ImGui::Button("Revert")) {}
		ImGui::SameLine();
		if (ImGui::Button("Save")) {}
		ImGui::EndGroup();
	}


	if (m_PickingType == PICKING_MODEL && CGameInstance::GetInstance()->Key_Up(DIK_Z))
	{
		_tchar* LayerTag = StringToTCHAR(m_stLayerTags[m_iSeletecLayerNum]);
		m_iCurrentLevel = (LEVEL)CGameInstance::GetInstance()->Get_CurrentLevelIndex();
		list<CGameObject*>* plistClone = CGameInstance::GetInstance()->Get_ObjectList(m_iCurrentLevel, LayerTag);
		if (nullptr == plistClone || plistClone->size() == 0)
		{
			delete LayerTag;
			return;
		}

		auto iter = --plistClone->end();
		m_pModel_Manager->Out_CreatedModel(dynamic_cast<CNonAnim*>(*iter));
		Safe_Release(*iter);
		plistClone->erase(iter);

		delete LayerTag;
	}
}

void CImGui_Manager::BrowseForFolder()
{
	if (m_bSave)
	{
		OPENFILENAME OFN;
		TCHAR filePathName[300] = L"";
		TCHAR lpstrFile[300] = L"";
		static TCHAR filter[] = L"데이터 파일\0*.dat\0텍스트 파일\0*.txt";

		memset(&OFN, 0, sizeof(OPENFILENAME));
		OFN.lStructSize = sizeof(OPENFILENAME);
		OFN.hwndOwner = g_hWnd;
		OFN.lpstrFilter = filter;
		OFN.lpstrFile = lpstrFile;
		OFN.nMaxFile = 300;
		OFN.lpstrInitialDir = L".";
		OFN.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT;

		if (GetSaveFileName(&OFN))
		{
			HANDLE hFile = 0;
			_ulong dwByte = 0;
			NONANIMDESC  ModelDesc;
			_int iNum = 0;
			_tchar* LayerTag = StringToTCHAR(m_stLayerTags[m_iSeletecLayerNum]);
			m_TempLayerTags.push_back(LayerTag);
			m_iCurrentLevel = (LEVEL)CGameInstance::GetInstance()->Get_CurrentLevelIndex();
			list<CGameObject*>* plistClone = CGameInstance::GetInstance()->Get_ObjectList(m_iCurrentLevel, LayerTag);
			if (nullptr == plistClone)
			{
				m_bSave = false;
				return;
			}


			iNum = (_int)plistClone->size();

			hFile = CreateFile(OFN.lpstrFile, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);;
			if (0 == hFile)
			{
				CloseHandle(hFile);
				return;
			}


			/* 첫줄은 object 리스트의 size 받아서 갯수만큼 for문 돌리게 하려고 저장해놓음*/
			WriteFile(hFile, &(iNum), sizeof(_int), &dwByte, nullptr);

			for (auto& iter : *plistClone)
			{
				if (iter == nullptr)
					continue;

				ModelDesc = dynamic_cast<CNonAnim*>(iter)->Get_ModelDesc();
				WriteFile(hFile, &ModelDesc, sizeof(NONANIMDESC), &dwByte, nullptr);

			}

			CloseHandle(hFile);

		}

		m_bSave = false;
	}
	else if (m_bLoad)
	{
		OPENFILENAME OFN;
		TCHAR filePathName[300] = L"";
		TCHAR lpstrFile[300] = L"";
		static TCHAR filter[] = L"데이터 파일\0*.dat\0텍스트 파일\0*.txt";

		memset(&OFN, 0, sizeof(OPENFILENAME));
		OFN.lStructSize = sizeof(OPENFILENAME);
		OFN.hwndOwner = g_hWnd;
		OFN.lpstrFilter = filter;
		OFN.lpstrFile = lpstrFile;
		OFN.nMaxFile = 300;
		OFN.lpstrInitialDir = L".";

		if (GetOpenFileName(&OFN) != 0) {
			wsprintf(filePathName, L"%s 파일을 열겠습니까?", OFN.lpstrFile);
			MessageBox(g_hWnd, filePathName, L"열기 선택", MB_OK);

			HANDLE hFile = 0;
			_ulong dwByte = 0;
			NONANIMDESC  ModelDesc;
			_int iNum = 0;
			LEVEL iLevel = (LEVEL)CGameInstance::GetInstance()->Get_CurrentLevelIndex();
			_matrix			PivotMatrix = XMMatrixIdentity();

			_tchar* LayerTag = StringToTCHAR(m_stLayerTags[m_iSeletecLayerNum]);
			m_TempLayerTags.push_back(LayerTag);

			hFile = CreateFile(OFN.lpstrFile, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			if (0 == hFile)
			{
				CloseHandle(hFile);
				return;
			}


			/* 타일의 개수 받아오기 */
			ReadFile(hFile, &(iNum), sizeof(_int), &dwByte, nullptr);

			for (_int i = 0; i < iNum; ++i)
			{
				ReadFile(hFile, &(ModelDesc), sizeof(NONANIMDESC), &dwByte, nullptr);
				m_pModel_Manager->Set_InitModelDesc(ModelDesc);
				_tchar			szModeltag[MAX_PATH] = TEXT("");
				MultiByteToWideChar(CP_ACP, 0, ModelDesc.pModeltag, (_int)strlen(ModelDesc.pModeltag), szModeltag, MAX_PATH);
				m_pModel_Manager->Create_Model(iLevel, szModeltag, LayerTag, m_pDevice, m_pContext, CModel::TYPE_NONANIM, PivotMatrix);
			}
			CloseHandle(hFile);
		}

		m_bLoad = false;
	}
}

void CImGui_Manager::Set_Animation()
{
	CGameInstance* pGameInstance = CGameInstance::GetInstance();

	if (ImGui::Button("Load Model"))
		m_isLoad = true;
	ImGui::SameLine();
	if (ImGui::Button("Load Second Model"))
		m_isSecondLoad = true;
	ImGui::SameLine();

	if (ImGui::Button("Save Animation"))
	{
		_tchar pAnimationDataFilePath[MAX_PATH] = TEXT("../../../Bin/Bin_Data/Anim/");

		CAnim::MODELDESC Pivot;
		CAnim::MODELDESC tempDesc =
			((CAnim*)(pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Player"))->front()))->Get_Desc();

		memcpy(&Pivot, &tempDesc, sizeof(CAnim::MODELDESC));


		switch (Pivot.eModel)
		{
		case MODEL_EZREAL:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Ezreal/Ezreal_Animation_Add.dat"));
			break;

		case MODEL_BRAUM:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Braum/Braum_Animation_Add.dat"));
			break;

		case MODEL_GALIO:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Galio/Galio_Animation_Add.dat"));
			break;
		case MODEL_LEESIN:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Leesin/Leesin_Animation_Add.dat"));
			break;
		case MODEL_YASUO:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Yasuo/Yasuo_Animation_Add.dat"));
			break;

		case MODEL_GAREN:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Garen/Garen_Animation_Add.dat"));
			break;
		case MODEL_GWEN:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Gwen/Gwen_Animation_Add.dat"));
			break;
		case MODEL_LEONA:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Leona/Leona_Animation_Add.dat"));
			break;
		case MODEL_VI:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Vi/Vi_Animation_Add.dat"));
			break;
		case MODEL_MINION_CANNON_B:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Minion/Minion_Cannon_Blue/Minion_Cannon_Blue_Animation_Add.dat"));
			break;
		case MODEL_MINION_CANNON_R:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Minion/Minion_Cannon_Red/Minion_Cannon_Red_Animation_Add.dat"));
			break;
		case MODEL_MINION_MELEE_B:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Minion/Minion_Melee_Blue/Minion_Melee_Blue_Animation_Add.dat"));
			break;
		case MODEL_MINION_MELEE_R:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Minion/Minion_Melee_Red/Minion_Melee_Red_Animation_Add.dat"));
			break;
		case MODEL_MINION_RANGE_B:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Minion/Minion_Range_Blue/Minion_Range_Blue_Animation_Add.dat"));
			break;
		case MODEL_MINION_RANGE_R:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Minion/Minion_Range_Red/Minion_Range_Red_Animation_Add.dat"));
			break;
		case MODEL_MINION_SUPER_B:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Minion/Minion_Super_Blue/Minion_Super_Blue_Animation_Add.dat"));
			break;
		case MODEL_MINION_SUPER_R:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Minion/Minion_Super_Red/Minion_Super_Red_Animation_Add.dat"));
			break;
		case MODEL_INHIBITOR_B:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Buildings/Inhibitor_Blue/Inhibitor_Blue_Animation_Add.dat"));
			break;
		case MODEL_INHIBITOR_R:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Buildings/Inhibitor_Red/Inhibitor_Red_Animation_Add.dat"));
			break;
		case MODEL_NEXUS_B:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Buildings/Nexus_Blue/Nexus_Blue_Animation_Add.dat"));
			break;
		case MODEL_NEXUS_R:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Buildings/Nexus_Red/Nexus_Red_Animation_Add.dat"));
			break;

		case TURRET_BLUE:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Buildings/Turret_Blue/Turret_Blue_Animation_Add.dat"));
			break;
		case TURRET_RED:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Buildings/Turret_Red/Turret_Red_Animation_Add.dat"));
			break;
		case TURRET_SHIELD:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Buildings/Turret_Shield/Turret_Shield_Animation_Add.dat"));
			break;
		case NPC_STORE:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Buildings/Npc_Store/Npc_Store_Animation_Add.dat"));
			break;
		case NPC_STOREKEPPER:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Buildings/Npc_StoreKeeper/Npc_StoreKeeper_Animation_Add.dat"));
			break;
		}

		CModel* pPlayerModel = (CModel*)pGameInstance->Get_Component(LEVEL_GAMEPLAY, TEXT("Layer_Player"), TEXT("Com_Model"));
		if (nullptr != pPlayerModel)
		{
			_ulong dwByte = 0;
			HANDLE hFile = CreateFile(pAnimationDataFilePath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
			if (0 == hFile)
			{
				MSG_BOX("Failed to Save : Animation_Add");
				return;
			}

			for (auto& Animation : pPlayerModel->Get_Animations())
			{
				vector<_float> TickPerSeconds = Animation->Get_TickPerSeconds();
				vector<_float> ChangeTimes = Animation->Get_ChangeTickTimes();
				vector<ANIMEVENT> AnimEvent = Animation->Get_Events();

				_int TickPerSecondsSize = TickPerSeconds.size();
				_int ChangeTimesSize = ChangeTimes.size();
				_int EventsSize = AnimEvent.size();

				_float fDuration = Animation->Get_Duration();
				WriteFile(hFile, &fDuration, sizeof(_float), &dwByte, nullptr);

				WriteFile(hFile, &TickPerSecondsSize, sizeof(_int), &dwByte, nullptr);
				for (_int i = 0; i < TickPerSecondsSize; ++i)
					WriteFile(hFile, &TickPerSeconds[i], sizeof(_float), &dwByte, nullptr);

				WriteFile(hFile, &ChangeTimesSize, sizeof(_int), &dwByte, nullptr);
				for (_int i = 0; i < ChangeTimesSize; ++i)
					WriteFile(hFile, &ChangeTimes[i], sizeof(_float), &dwByte, nullptr);

				WriteFile(hFile, &EventsSize, sizeof(_int), &dwByte, nullptr);
				for (_int i = 0; i < EventsSize; ++i)
					WriteFile(hFile, &AnimEvent[i], sizeof(ANIMEVENT), &dwByte, nullptr);
			}

			CloseHandle(hFile);
		}

		m_isSaveComplete = true;
	}

	if (m_isSaveComplete)
	{
		ImGui::Begin("Save Animation", &m_isSaveComplete);

		ImGui::Text("Save Complete");

		if (ImGui::Button("Close"))
			m_isSaveComplete = false;

		ImGui::End();
	}

	if (m_isLoad)
	{
		ImGui::Begin("Load", &m_isLoad);

		const char** pItems = new const char* [m_AnimObj.size()];


		for (_int i = 0; i < m_AnimObj.size(); ++i)
			pItems[i] = m_AnimObj[i].c_str();

		if (ImGui::ListBox("Load Model", &m_iLoadModel, pItems, _int(m_AnimObj.size())))
		{
			list<CGameObject*>* pPlayerList = pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Player"));

			if ((nullptr != pPlayerList) && !pPlayerList->empty())
				((CAnim*)(pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Player"))->front()))->Set_Dead(true);

			CAnim::MODELDESC Pivot;
			ZeroMemory(&Pivot, sizeof(CAnim::MODELDESC));

			Pivot.vPosition = _float3(0.f, 0.f, 0.f);
			Pivot.vScale = _float3(1.f, 1.f, 1.f);
			Pivot.vRotation = _float3(0.f, 0.f, 0.f);

			Pivot.eModel = (MODEL_ID)m_iLoadModel;

			if (FAILED(pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_Anim"), LEVEL_GAMEPLAY, TEXT("Layer_Player"), &Pivot)))
				return;

			if (m_iPreModel != m_iLoadModel)
			{
				m_iBoneChoice = -1;
				m_iAnimationChoice = -1;
			}

			m_iPreModel = m_iLoadModel;
		}

		Safe_Delete_Array(pItems);

		if (ImGui::Button("Close"))
		{
			m_isLoad = false;
			m_iLoadModel = -1;
		}

		ImGui::End();
	}

	if (m_isSecondLoad)
	{
		ImGui::Begin("Second Load", &m_isSecondLoad);

		const char** pItems = new const char* [m_AnimObj.size()];

		for (_int i = 0; i < m_AnimObj.size(); ++i)
			pItems[i] = m_AnimObj[i].c_str();

		if (ImGui::ListBox("Load Second Model", &m_iLoadSecondModel, pItems, _int(m_AnimObj.size())))
		{
			list<CGameObject*>* pPlayerList = pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Player"));

			if ((nullptr != pPlayerList) && (1 < pPlayerList->size()))
			{
				auto iter = pPlayerList->begin();
				++iter;
				(*iter)->Set_Dead(true);
			}

			CAnim::MODELDESC Pivot;
			ZeroMemory(&Pivot, sizeof(CAnim::MODELDESC));

			Pivot.vPosition = _float3(0.f, 0.f, 0.f);
			Pivot.vScale = _float3(1.f, 1.f, 1.f);
			Pivot.vRotation = _float3(0.f, 0.f, 0.f);

			Pivot.eModel = (MODEL_ID)m_iLoadSecondModel;

			if (FAILED(pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_Anim"), LEVEL_GAMEPLAY, TEXT("Layer_Player"), &Pivot)))
				return;

			if (m_iPreSecondModel != m_iLoadSecondModel)
				m_iSecondAnimationChoice = -1;

			m_iPreSecondModel = m_iLoadSecondModel;
		}

		Safe_Delete_Array(pItems);

		if (ImGui::Button("Close"))
		{
			m_isSecondLoad = false;
			m_iLoadSecondModel = -1;
		}

		ImGui::End();
	}

	CModel* pPlayerModel = (CModel*)pGameInstance->Get_Component(LEVEL_GAMEPLAY, TEXT("Layer_Player"), TEXT("Com_Model"));
	CModel* pSecondPlayerModel = (CModel*)pGameInstance->Get_Component(LEVEL_GAMEPLAY, TEXT("Layer_Player"), TEXT("Com_Model"), 1);

	if (nullptr != pPlayerModel)
	{
		ImGuiTreeNodeFlags TreeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen;

		if (ImGui::CollapsingHeader("World Transform", TreeNodeFlags))
		{
			if (ImGui::DragFloat3("Rotation", m_Rotation, 0.01f, 0.f, 360.f))
				((CAnim*)(pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Player"))->front()))->Get_Transform()->Set_Rotation(_float3(m_Rotation[0], m_Rotation[1], m_Rotation[2]));
			if (ImGui::DragFloat3("Position", m_Position, 0.01f))
			{
				_vector vPos = XMVectorSet(m_Position[0], m_Position[1], m_Position[2], 1.f);
				((CAnim*)(pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Player"))->front()))->Get_Transform()->Set_State(CTransform::STATE_TRANSLATION, vPos);
			}

			if (nullptr != pSecondPlayerModel)
			{
				if (ImGui::DragFloat3("Second Rotation", m_SecondRotation, 0.01f, 0.f, 360.f))
					((CAnim*)(*(++pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Player"))->begin())))->Get_Transform()->Set_Rotation(_float3(m_SecondRotation[0], m_SecondRotation[1], m_SecondRotation[2]));
				if (ImGui::DragFloat3("Second Position", m_SecondPosition, 0.01f))
				{
					_vector vPos = XMVectorSet(m_SecondPosition[0], m_SecondPosition[1], m_SecondPosition[2], 1.f);
					((CAnim*)(*(++pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Player"))->begin())))->Get_Transform()->Set_State(CTransform::STATE_TRANSLATION, vPos);
				}
			}
		}

		vector<CBone*> ModelBones = pPlayerModel->Get_Bones();
		vector<CAnimation*> ModelAnimations = pPlayerModel->Get_Animations();
		vector<CAnimation*> SecondModelAnimations;
		if (nullptr != pSecondPlayerModel)
			SecondModelAnimations = pSecondPlayerModel->Get_Animations();

		if (ImGui::CollapsingHeader("Bone", TreeNodeFlags))
		{
			char** pBoneItems = new char* [ModelBones.size()];

			for (_int i = 0; i < ModelBones.size(); ++i)
			{
				pBoneItems[i] = new char[MAX_PATH];
				strcpy_s(pBoneItems[i], sizeof(char) * MAX_PATH, ModelBones[i]->Get_Name());
			}

			if (ImGui::ListBox("", &m_iBoneChoice, pBoneItems, _int(ModelBones.size())))
			{
				_matrix BoneTransformation = ModelBones[m_iBoneChoice]->Get_TransformationMatrix();

			}

			for (_int i = 0; i < ModelBones.size(); ++i)
				Safe_Delete_Array(pBoneItems[i]);

			Safe_Delete_Array(pBoneItems);

			if (ImGui::Button("Weapon"))
				m_isWeaponWindow = true;
		}

		if (m_isWeaponWindow)
		{
			ImGui::Begin("Weapon", &m_isWeaponWindow);

			// ImGui용 const char* 배열 준비
			vector<const char*> pWeapon(m_WeaponObj.size());
			for (_int i = 0; i < m_WeaponObj.size(); ++i)
				pWeapon[i] = m_WeaponObj[i].c_str();

			if (ImGui::ListBox("Weapon List", &m_iWeaponChoice, pWeapon.data(), (int)pWeapon.size()))
			{
				CModel* pPlayerModel = (CModel*)pGameInstance->Get_Component(LEVEL_GAMEPLAY, TEXT("Layer_Player"), TEXT("Com_Model"));
				if (nullptr != pPlayerModel)
				{
					_tchar ModelName[MAX_PATH] = TEXT("");

					// string → wchar_t 변환
					const string& selectedWeapon = m_WeaponObj[m_iWeaponChoice];
					MultiByteToWideChar(CP_ACP, 0,
						selectedWeapon.c_str(),
						(int)selectedWeapon.size(),
						ModelName, MAX_PATH);

					_tchar ModelTag[MAX_PATH] = TEXT("Prototype_Component_Model_");
					lstrcat(ModelTag, ModelName);

					vector<CBone*> ModelBones = pPlayerModel->Get_Bones();
				}
			}

			if (ImGui::Button("Close"))
			{
				m_iWeaponChoice = -1;
				m_isWeaponWindow = false;
			}

			ImGui::End();
		}


		if (ImGui::CollapsingHeader("Animation", TreeNodeFlags))
		{
			char** pAnimItems = new char* [ModelAnimations.size()];

			for (_int i = 0; i < ModelAnimations.size(); ++i)
			{
				pAnimItems[i] = new char[MAX_PATH];
				strcpy_s(pAnimItems[i], sizeof(char) * MAX_PATH, ModelAnimations[i]->Get_Name());
			}
			ImGui::SetNextItemWidth(600.f);
			if (ImGui::ListBox("Animation", &m_iAnimationChoice, pAnimItems, _int(ModelAnimations.size())))
			{
				((CAnim*)pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Player"))->front())->Set_AnimIndex(m_iAnimationChoice);

				m_fAnimationDuration = ModelAnimations[m_iAnimationChoice]->Get_Duration();

				m_iEventChoice = -1;
			}

			for (_int i = 0; i < ModelAnimations.size(); ++i)
				Safe_Delete_Array(pAnimItems[i]);

			Safe_Delete_Array(pAnimItems);

			if (ImGui::DragFloat("Duration", &m_fAnimationDuration, 0.005f, 0.001f, 200.f))
				ModelAnimations[m_iAnimationChoice]->Change_Duration(m_fAnimationDuration);
			ImGui::SameLine();
			ImGui::Checkbox("Animation Play", &m_isAnimationPlay);

			if (m_isAnimationPlay)
			{
				if (-1 != m_iAnimationChoice)
				{
					((CAnim*)pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Player"))->front())->StartAnim();
					m_fAnimationTime = ModelAnimations[m_iAnimationChoice]->Get_CurrentTime();
					if (m_fAnimationTime > m_fAnimationDuration)
						m_fAnimationTime = 0.f;
				}
			}
			else
				((CAnim*)pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Player"))->front())->StopAnim();

			if (ImGui::SliderFloat("Playing", &m_fAnimationTime, 0.f, m_fAnimationDuration))
			{
				ModelAnimations[m_iAnimationChoice]->Reset();
				ModelAnimations[m_iAnimationChoice]->Set_CurrentTime(m_fAnimationTime);
				((CAnim*)pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Player"))->front())->PlayAnimation();
			}

			if (ImGui::Button("Animation Edit"))
				m_isAnimationEdit = true;

			ImGui::SameLine();

			if (ImGui::Button("Animation Event"))
				m_isEventWindow = true;

			/* Second */
			if (nullptr != pSecondPlayerModel)
			{
				char** pAnimItems = new char* [SecondModelAnimations.size()];

				for (_int i = 0; i < SecondModelAnimations.size(); ++i)
				{
					pAnimItems[i] = new char[MAX_PATH];
					strcpy_s(pAnimItems[i], sizeof(char) * MAX_PATH, SecondModelAnimations[i]->Get_Name());
				}
				ImGui::SetNextItemWidth(600.f);
				if (ImGui::ListBox("Second Animation", &m_iSecondAnimationChoice, pAnimItems, _int(SecondModelAnimations.size())))
				{
					((CAnim*)(*(++pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Player"))->begin())))->Set_AnimIndex(m_iSecondAnimationChoice);

					m_fSecondAnimationDuration = SecondModelAnimations[m_iSecondAnimationChoice]->Get_Duration();
				}

				for (_int i = 0; i < SecondModelAnimations.size(); ++i)
					Safe_Delete_Array(pAnimItems[i]);

				Safe_Delete_Array(pAnimItems);

				if (ImGui::DragFloat("Second Duration", &m_fSecondAnimationDuration, 0.005f, 0.001f, 200.f))
					SecondModelAnimations[m_iSecondAnimationChoice]->Change_Duration(m_fSecondAnimationDuration);
				ImGui::SameLine();
				ImGui::Checkbox("Second Animation Play", &m_isSecondAnimationPlay);

				if (m_isSecondAnimationPlay)
				{
					if (-1 != m_iSecondAnimationChoice)
					{
						((CAnim*)(*(++pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Player"))->begin())))->StartAnim();
						m_fSecondAnimationTime = SecondModelAnimations[m_iSecondAnimationChoice]->Get_CurrentTime();
						if (m_fSecondAnimationTime > m_fSecondAnimationDuration)
							m_fSecondAnimationTime = 0.f;
					}
				}
				else
					((CAnim*)(*(++pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Player"))->begin())))->StopAnim();

				if (ImGui::SliderFloat("Second Playing", &m_fSecondAnimationTime, 0.f, m_fSecondAnimationDuration))
				{
					SecondModelAnimations[m_iSecondAnimationChoice]->Reset();
					SecondModelAnimations[m_iSecondAnimationChoice]->Set_CurrentTime(m_fSecondAnimationTime);
					((CAnim*)(*(++pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Player"))->begin())))->PlayAnimation();
				}

				ImGui::Checkbox("Strike Play", &m_isStrikePlay);

				if (m_isStrikePlay)
				{
					if (-1 != m_iAnimationChoice && -1 != m_iSecondAnimationChoice)
					{
						((CAnim*)pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Player"))->front())->StartAnim();
						m_fAnimationTime = ModelAnimations[m_iAnimationChoice]->Get_CurrentTime();

						((CAnim*)(*(++pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Player"))->begin())))->StartAnim();
						m_fSecondAnimationTime = SecondModelAnimations[m_iSecondAnimationChoice]->Get_CurrentTime();
					}
				}
				else if (!m_isStrikePlay && !m_isAnimationPlay && !m_isSecondAnimationPlay)
				{
					((CAnim*)pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Player"))->front())->StopAnim();
					((CAnim*)(*(++pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Player"))->begin())))->StopAnim();

					if (-1 != m_iAnimationChoice && -1 != m_iSecondAnimationChoice)
					{
						ModelAnimations[m_iAnimationChoice]->Reset();
						SecondModelAnimations[m_iSecondAnimationChoice]->Reset();
					}

					if (m_fAnimationTime > m_fAnimationDuration)
						m_fAnimationTime = 0.f;

					if (m_fSecondAnimationTime > m_fSecondAnimationDuration)
						m_fSecondAnimationTime = 0.f;
				}
			}
		}

		if (m_isAnimationEdit)
		{
			ImGui::Begin("Animation Edit", &m_isAnimationEdit);

			CModel* pPlayerModel = (CModel*)pGameInstance->Get_Component(LEVEL_GAMEPLAY, TEXT("Layer_Player"), TEXT("Com_Model"));
			if ((nullptr != pPlayerModel) && (-1 != m_iAnimationChoice))
			{
				CAnimation* ChoiceModelAnimation = pPlayerModel->Get_Animations()[m_iAnimationChoice];

				vector<_float> TimeVector = ChoiceModelAnimation->Get_ChangeTickTimes();
				vector<_float> TickVector = ChoiceModelAnimation->Get_TickPerSeconds();

				char** pTime = new char* [TimeVector.size()];
				char** pTick = new char* [TickVector.size()];

				for (_int i = 0; i < TimeVector.size(); ++i)
				{
					pTime[i] = new char[MAX_PATH];
					snprintf(pTime[i], MAX_PATH, "%f", TimeVector[i]);
				}

				for (_int i = 0; i < TickVector.size(); ++i)
				{
					pTick[i] = new char[MAX_PATH];
					snprintf(pTick[i], MAX_PATH, "%f", TickVector[i]);
				}

				if (ImGui::ListBox("Time", &m_iTimeChoice, pTime, TimeVector.size()))
				{
					m_iTickChoice = m_iTimeChoice;

					m_fAnimationTickTime = TimeVector[m_iTimeChoice];
					m_fAnimationTick = TickVector[m_iTimeChoice];
				}

				if (ImGui::ListBox("TickPerSecond", &m_iTickChoice, pTick, TickVector.size()))
				{
					m_iTimeChoice = m_iTickChoice;

					m_fAnimationTickTime = TimeVector[m_iTickChoice];
					m_fAnimationTick = TickVector[m_iTickChoice];
				}

				ImGui::DragFloat("Playing Time", &m_fAnimationTickTime, 0.05f, 0.f, m_fAnimationDuration);
				ImGui::DragFloat("Playing TickPerSecond", &m_fAnimationTick, 0.05f, 0.f, 100.f);

				_float fOriginalTickPerSecond = ChoiceModelAnimation->Get_OriginalTickPerSecond();
				ImGui::InputFloat("Original TickPerSecond", &fOriginalTickPerSecond);

				if (ImGui::Button("Add"))
					ChoiceModelAnimation->Add_TickPerSecond(m_fAnimationTickTime, m_fAnimationTick);
				ImGui::SameLine();
				if (ImGui::Button("Fix"))
					ChoiceModelAnimation->Fix_TickPerSecond(m_iTimeChoice, m_fAnimationTickTime, m_fAnimationTick);
				ImGui::SameLine();
				if (ImGui::Button("Delete"))
				{
					ChoiceModelAnimation->Delete_TickPerSecond(m_iTimeChoice);

					m_fAnimationTickTime = 0.f;
					m_fAnimationTick = 0.f;
				}

				for (_int i = 0; i < TimeVector.size(); ++i)
					Safe_Delete_Array(pTime[i]);
				Safe_Delete_Array(pTime);

				for (_int i = 0; i < TickVector.size(); ++i)
					Safe_Delete_Array(pTick[i]);
				Safe_Delete_Array(pTick);
			}

			if (ImGui::Button("Close"))
			{
				m_iTickChoice = -1;
				m_iTimeChoice = -1;

				m_isAnimationEdit = false;
			}
			ImGui::End();
		}

		if (m_isEventWindow)
		{
			m_isEventWindow = true;
			ImGui::Begin("Event", &m_isEventWindow);

			CModel* pPlayerModel = (CModel*)pGameInstance->Get_Component(LEVEL_GAMEPLAY, TEXT("Layer_Player"), TEXT("Com_Model"));
			if ((nullptr != pPlayerModel) && (-1 != m_iAnimationChoice))
			{
				CAnimation* ChoiceModelAnimation = pPlayerModel->Get_Animations()[m_iAnimationChoice];

				vector<ANIMEVENT> AnimEventVector = ChoiceModelAnimation->Get_Events();
				_int iAnimEvents = AnimEventVector.size();

				char** pStartTime = new char* [iAnimEvents];
				char** pEndTime = new char* [iAnimEvents];
				const char** pEventType = new const char* [iAnimEvents];

				for (_int i = 0; i < iAnimEvents; ++i)
				{
					pStartTime[i] = new char[MAX_PATH];
					snprintf(pStartTime[i], MAX_PATH, "%f", AnimEventVector[i].fStartTime);
				}

				for (_int i = 0; i < iAnimEvents; ++i)
				{
					pEndTime[i] = new char[MAX_PATH];
					snprintf(pEndTime[i], MAX_PATH, "%f", AnimEventVector[i].fEndTime);
				}

				for (_int i = 0; i < iAnimEvents; ++i)
				{
					switch (AnimEventVector[i].eType)
					{
					case ANIMEVENT::EVENT_SOUND:
						pEventType[i] = "EVENT_SOUND";
						break;
					case ANIMEVENT::EVENT_EFFECT:
						pEventType[i] = "EVENT_EFFECT";
						break;
					case ANIMEVENT::EVENT_COLLIDER:
						pEventType[i] = "EVENT_COLLIDER";
						break;
					case ANIMEVENT::EVENT_STATE:
						pEventType[i] = "EVENT_STATE";
						break;
					case ANIMEVENT::EVENT_INPUT:
						pEventType[i] = "EVENT_INPUT";
						break;
					}
				}

				if (ImGui::ListBox("Event Start", &m_iEventChoice, pStartTime, iAnimEvents))
				{
					m_fEventStartTime = AnimEventVector[m_iEventChoice].fStartTime;
					m_fEventEndTime = AnimEventVector[m_iEventChoice].fEndTime;
				}
				if (ImGui::ListBox("Event End", &m_iEventChoice, pEndTime, iAnimEvents))
				{
					m_fEventStartTime = AnimEventVector[m_iEventChoice].fStartTime;
					m_fEventEndTime = AnimEventVector[m_iEventChoice].fEndTime;
				}
				if (ImGui::ListBox("Event Type", &m_iEventChoice, pEventType, iAnimEvents))
				{
					m_fEventStartTime = AnimEventVector[m_iEventChoice].fStartTime;
					m_fEventEndTime = AnimEventVector[m_iEventChoice].fEndTime;
				}

				ImGui::DragFloat("Start Time", &m_fEventStartTime, 0.05f, 0.f, m_fAnimationDuration);
				ImGui::DragFloat("End Time", &m_fEventEndTime, 0.05f, 0.f, m_fAnimationDuration);

				static char szEventName[MAX_PATH] = "";
				if (-1 != m_iEventChoice && m_iEventChoice < AnimEventVector.size())
					memcpy(szEventName, AnimEventVector[m_iEventChoice].szName, sizeof(char) * MAX_PATH);

				ImGui::SetNextItemWidth(175);
				ImGui::InputText("##Event Tag", szEventName, MAX_PATH);

				ImGui::RadioButton("Event_Sound", &m_iEventType, 0);
				ImGui::SameLine();
				ImGui::RadioButton("Event_Effect", &m_iEventType, 1);
				ImGui::SameLine();
				ImGui::RadioButton("Event_Collider", &m_iEventType, 2);
				ImGui::RadioButton("Event_State", &m_iEventType, 3);
				ImGui::SameLine();
				ImGui::RadioButton("Event_Input", &m_iEventType, 4);

				ImGui::Text("Event Name");
				static char cEventName[MAX_PATH] = "";
				ImGui::SetNextItemWidth(175);
				ImGui::InputText("##InputEvent", cEventName, MAX_PATH);

				if (ImGui::Button("Add"))
				{
					ANIMEVENT AnimEvent;
					ZeroMemory(&AnimEvent, sizeof(ANIMEVENT));

					AnimEvent.eType = (ANIMEVENT::EVENTTYPE)m_iEventType;
					AnimEvent.fStartTime = m_fEventStartTime;
					AnimEvent.fEndTime = m_fEventEndTime;
					memcpy(AnimEvent.szName, cEventName, sizeof(char) * MAX_PATH);

					ChoiceModelAnimation->Add_Event(AnimEvent);
				}
				ImGui::SameLine();
				if (ImGui::Button("Fix"))
				{
					ANIMEVENT AnimEvent;
					ZeroMemory(&AnimEvent, sizeof(ANIMEVENT));

					AnimEvent.eType = (ANIMEVENT::EVENTTYPE)m_iEventType;
					AnimEvent.fStartTime = m_fEventStartTime;
					AnimEvent.fEndTime = m_fEventEndTime;
					memcpy(AnimEvent.szName, cEventName, sizeof(char) * MAX_PATH);

					ChoiceModelAnimation->Fix_Event(m_iEventChoice, AnimEvent);
				}
				ImGui::SameLine();
				if (ImGui::Button("Delete"))
				{
					ChoiceModelAnimation->Delete_Event(m_iEventChoice);

					m_fEventStartTime = 0.f;
					m_fEventEndTime = 0.f;
					m_iEventType = ANIMEVENT::EVENTTYPE::EVENT_END;
					m_iEventChoice = -1;
				}

				for (_int i = 0; i < iAnimEvents; ++i)
				{
					Safe_Delete_Array(pStartTime[i]);
					Safe_Delete_Array(pEndTime[i]);
				}

				Safe_Delete_Array(pStartTime);
				Safe_Delete_Array(pEndTime);
				Safe_Delete_Array(pEventType);
			}

			if (ImGui::Button("Close"))
			{
				m_isEventWindow = false;
				m_iEventChoice = -1;
			}

			ImGui::End();
		}
	}
}

void CImGui_Manager::Create_Model(const _tchar* pPrototypeTag, const _tchar* pLayerTag, _bool bCreatePrototype)
{
	CGameInstance* pGameInstance = CGameInstance::GetInstance();
	pGameInstance->AddRef();
	m_InitDesc.vPosition.x += m_vfOffset.x;
	m_InitDesc.vPosition.y += m_vfOffset.y;
	m_InitDesc.vPosition.z += m_vfOffset.z;
	XMStoreFloat4x4(&m_InitDesc.WorldMatrix, XMMatrixIdentity());
	m_pModel_Manager->Set_InitModelDesc(m_InitDesc);

	_matrix			PivotMatrix = XMMatrixIdentity();
	LEVEL iLevel = (LEVEL)pGameInstance->Get_CurrentLevelIndex();
	m_pModel_Manager->Create_Model(iLevel, pPrototypeTag, pLayerTag, m_pDevice, m_pContext, CModel::TYPE_NONANIM, PivotMatrix, bCreatePrototype);
	pGameInstance->Release();
}

void CImGui_Manager::Free()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	for (auto& iter : m_TempLayerTags)
		Safe_Delete(iter);
	m_TempLayerTags.clear();
	m_stLayerTags.clear();
	Safe_Release(m_pTerrianManager);
	Safe_Release(m_pModel_Manager);

	CModelManager::GetInstance()->DestroyInstance();

	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);
}
