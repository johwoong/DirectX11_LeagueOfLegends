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
	, m_pNavigation_Manager(CNavigation_Manager::GetInstance())
	, m_pEffectManager(CEffectManager::GetInstance())
{
	Safe_AddRef(m_pModel_Manager);
	Safe_AddRef(m_pTerrianManager);
	Safe_AddRef(m_pNavigation_Manager);
	Safe_AddRef(m_pEffectManager);
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


	io.FontDefault = io.Fonts->AddFontFromFileTTF("../../../Bin/Resources/Fonts/dejavusans-bold.ttf", 16.0f);
	Apply_UnityDark_Theme();

	ImGui_ImplWin32_Init(g_hWnd);
	ImGui_ImplDX11_Init(m_pDevice, m_pContext);

	Read_EffectsData();

	_tchar wcTexturesPath[MAX_PATH] = TEXT("../../../Bin/Resources/Textures/Effect/");
	Read_Textures(wcTexturesPath);

	_tchar wcMeshesPath[MAX_PATH] = TEXT("../../../Bin/Bin_Data/Effect/");
	Read_Meshes(wcMeshesPath);
	
	
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
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void CImGui_Manager::Update_Imgui()
{
	ImGui::SetNextWindowDockID(0, ImGuiCond_Always);

	ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_Once);

	ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoSavedSettings;

	if (!ImGui::Begin(u8"Editor", nullptr, windowFlags))
	{
		ImGui::End();
		return;
	}

	ImGui::BeginChild("EditorScrollRegion",
		ImVec2(0, 0),      
		false,
		ImGuiWindowFlags_AlwaysVerticalScrollbar |
		ImGuiWindowFlags_AlwaysHorizontalScrollbar);

	ImGui::NewLine();

	static int iCurrentLevel = (int)m_iCurrentLevel;
	m_iCurrentLevel = (LEVEL)iCurrentLevel;

	if (ImGui::BeginTabBar("MyTabBar"))
	{
		if (ImGui::BeginTabItem("Terrian Tool"))
		{
			Set_Terrian_Map();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Model Tool"))
		{
			if (ImGui::BeginTabBar("ModelsTabs"))
			{
				if (ImGui::BeginTabItem("Model"))
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
		if (ImGui::BeginTabItem("Navigation Tool"))
		{
			ImGui::Checkbox("Picking", &m_bNaviPicking);
			ImGui::SameLine();
			ImGui::Checkbox("Show Navigation", &m_bShowOnlyNavi);
			ImGui::SameLine();
			ImGui::Checkbox("Show Map", &m_bShowMap);
			ImGui::Text("Navigation Tool");
			Set_Navigation();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Effect Tool"))
		{
			Set_Effect();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Animation Tool"))
		{
			Set_Animation();
			ImGui::EndTabItem();
		}

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

	ImGui::EndChild();

	ImGui::End();


}


void CImGui_Manager::ShowPickedObj()
{
	CPickingMgr* pPickingMgr = CPickingMgr::GetInstance();
	pPickingMgr->AddRef();

	CGameObject* pPickedObj = pPickingMgr->Get_PickedObj();

	if (pPickedObj)
	{
		auto pActor = dynamic_cast<CActor*>(pPickedObj);
		if (pActor)
		{
			DirectX::XMStoreFloat3(&m_vPickedObjPos, pActor->Get_Position());
			m_vPickedObjScale = pActor->Get_Scale();

			if (pActor->Get_ObjectID() != OBJ_BACKGROUND)
			{
				auto pNonAnim = dynamic_cast<CNonAnim*>(pPickedObj);
				if (pNonAnim)
				{
					m_vPickedRotAxis = pNonAnim->Get_ModelDesc().vRotation;
					m_fRotAngle = pNonAnim->Get_ModelDesc().m_fAngle;
				}
			}
		}
	}
	else
	{
		m_vPickedObjPos = _float3(0.f, 0.f, 0.f);
		m_vPickedObjScale = _float3(1.f, 1.f, 1.f);
		m_fRotAngle = 0.f;
	}

	ImGui::NewLine();
	ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Position");
	ImGui::DragFloat3("Position", (float*)&m_vPickedObjPos, 0.1f);

	ImGui::NewLine();
	ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Rotation");
	ImGui::DragFloat3("Rotation", (float*)&m_vPickedRotAxis, 0.1f);

	ImGui::NewLine();
	ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Scale");
	ImGui::DragFloat3("Scale", (float*)&m_vPickedObjScale, 0.01f);

	if (pPickedObj && dynamic_cast<CActor*>(pPickedObj)->Get_ObjectID() == OBJ_BLOCK)
	{
		auto pActor = dynamic_cast<CActor*>(pPickedObj);
		auto pNonAnim = dynamic_cast<CNonAnim*>(pPickedObj);

		_vector vSettingPosition = DirectX::XMLoadFloat3(&m_vPickedObjPos);
		vSettingPosition = XMVectorSetW(vSettingPosition, 1.f);

		pActor->Set_State(CTransform::STATE_TRANSLATION, vSettingPosition);
		pActor->Set_Scale(m_vPickedObjScale);

		if (pNonAnim)
			pNonAnim->Turn(m_vPickedRotAxis, 0.f);
	}

	// ======================= Delete 버튼 =======================
	if (ImGui::Button("Delete"))
	{
		if (pPickedObj)
			ImGui::OpenPopup("Delete Object?");
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

	colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);

	colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);    
	colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);

	colors[ImGuiCol_Border] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
	colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.40f, 0.65f, 1.00f);  // 파란 포인트
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.55f, 0.85f, 1.00f);

	colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.15f, 0.35f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.25f, 0.55f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.05f, 0.08f, 0.20f, 0.70f);

	colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.18f, 0.38f, 1.00f);  // 진한 파란색
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.60f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.35f, 0.35f, 0.45f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.45f, 0.55f, 0.75f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.55f, 0.65f, 0.85f, 1.00f);

	colors[ImGuiCol_Button] = ImVec4(0.22f, 0.35f, 0.60f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.45f, 0.75f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.40f, 0.90f, 1.00f);

	colors[ImGuiCol_Header] = ImVec4(0.25f, 0.35f, 0.55f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.50f, 0.80f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.45f, 0.65f, 0.95f, 1.00f);

	colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.25f, 0.35f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.40f, 0.75f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.28f, 0.48f, 0.88f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.20f, 0.25f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.30f, 0.50f, 1.00f);

	colors[ImGuiCol_CheckMark] = ImVec4(0.95f, 0.95f, 0.98f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.45f, 0.60f, 0.95f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.65f, 0.75f, 1.00f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.35f, 0.35f, 0.45f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.25f, 0.30f, 0.45f, 1.00f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.40f, 0.50f, 0.75f, 1.00f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.55f, 0.65f, 0.85f, 1.00f);
}

void CImGui_Manager::Show_PopupBox()
{
	if (ImGui::BeginPopupModal("Delete Object?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (ImGui::Button("OK", ImVec2(120, 0)))
		{
			if (auto pPickedObj = CPickingMgr::GetInstance()->Get_PickedObj())
			{
				CNonAnim* pNonAnim = dynamic_cast<CNonAnim*>(pPickedObj);
				if (pNonAnim)
				{
					m_pModel_Manager->Out_CreatedModel(pNonAnim);
					pNonAnim->Set_Dead(true);
					CPickingMgr::GetInstance()->Out_PickingGroup(pNonAnim);
				}
			}

			m_iCreatedSelected = -1;
			CPickingMgr::GetInstance()->Set_PickedObj(nullptr);
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
			ImGui::CloseCurrentPopup();

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
	OPENFILENAME OFN{};
	TCHAR filePathName[300] = L"";
	TCHAR lpstrFile[300] = L"";
	static TCHAR filter[] = L"데이터 파일\0*.dat\0텍스트 파일\0*.txt";

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

		CVIBuffer_Terrian* pBuffer = m_pTerrianManager->Create_Terrain(m_pDevice, m_pContext, hFile, dwByte);
		pGameInstance->Add_Prototype(LEVEL_STATIC, szFullPath, pBuffer);
		pGameInstance->Add_GameObjectLoad(TEXT("Prototype_GameObject_Terrain"), LEVEL_GAMEPLAY, TEXT("Layer_Terrain"), szFullPath);

		list<CGameObject*>* pTerrainList =
			pGameInstance->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Terrain"));

		if (pTerrainList && !pTerrainList->empty())
		{
			CTerrian* pTerrain = dynamic_cast<CTerrian*>(pTerrainList->back());
			if (pTerrain)
			{
				auto desc = pTerrain->Get_TerrianDesc();

				CTransform* pTransform = pTerrain->Get_Transform();
				if (pTransform)
				{
					pTransform->Set_State(CTransform::STATE_TRANSLATION,
						XMVectorSet((float)desc.m_iPositionX, desc.m_fHeight, (float)desc.m_iPositionZ, 1.f));

					pTransform->Set_Rotation(_float3(
						desc.m_fRotationX,
						desc.m_fRotationY,
						desc.m_fRotationZ));
				}
			}
		}
	}

	CloseHandle(hFile);
	pGameInstance->Release();
}

void CImGui_Manager::Set_Object_Map()
{
	ImGui::NewLine();
	ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Transform");

	// ----- Position -----
	static float Position[3] = { m_InitDesc.vPosition.x, m_InitDesc.vPosition.y, m_InitDesc.vPosition.z };
	ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Position");
	ImGui::SameLine();
	ImGui::InputFloat3("##Position", Position);

	// ----- Rotation -----
	static float Rotation[3] = { m_InitDesc.vRotation.x, m_InitDesc.vRotation.y, m_InitDesc.vRotation.z };
	ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Rotation");
	ImGui::SameLine();
	ImGui::DragFloat3("##Rotation", Rotation);
	m_InitDesc.vRotation = _float3(Rotation[0], Rotation[1], Rotation[2]);

	// ----- Scale -----
	static float Scale[3] = { m_InitDesc.vScale.x, m_InitDesc.vScale.y, m_InitDesc.vScale.z };
	ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Scale");
	ImGui::SameLine();
	ImGui::InputFloat3("##Scale", Scale);
	m_InitDesc.vScale = _float3(Scale[0], Scale[1], Scale[2]);
}

void CImGui_Manager::Set_LayerTag()
{
	////Layer tag를 만들거나, 삭제한다. 
	//ImGui::BulletText("Layer Setting");

	//if (ImGui::BeginCombo("Layer List", m_stLayerTags[m_iSeletecLayerNum].c_str()))
	//{
	//	for (int i = 0; i < m_stLayerTags.size(); i++)
	//	{
	//		if (ImGui::Selectable(m_stLayerTags[i].c_str(), m_iSeletecLayerNum == i))
	//			m_iSeletecLayerNum = i;
	//	}
	//	ImGui::EndCombo();
	//}
	//if (ImGui::Button("Delete Layer Tag"))
	//	ImGui::OpenPopup("Delete Layer Tag?"); ImGui::SameLine();

	//if (ImGui::Button("Add New Layer Tag"))
	//	ImGui::OpenPopup("Add Layer Tag");

	//Show_PopupBox();
}

void CImGui_Manager::Show_ModelList()
{
		vector<const _tchar*> ModelTags = m_pModel_Manager->Get_PrototypeTag();

	static int selected = 0;

	{
		ImGuiWindowFlags listFlags =
			ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar;

		ImGui::BeginChild("left pane", ImVec2(220, 500), true, listFlags);

		if (!ModelTags.empty())
		{
			ImGuiListClipper clipper;
			clipper.Begin((int)ModelTags.size());
			while (clipper.Step())
			{
				for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
				{
					const _tchar* tag = ModelTags[i];
					if (!tag) continue;

					char label[MAX_PATH] = "";
					WideCharToMultiByte(CP_ACP, 0, tag, -1, label, MAX_PATH, NULL, NULL);

					if (ImGui::Selectable(label, selected == i))
						selected = i;
				}
			}
			clipper.End();
		}
		ImGui::EndChild();
	}

	ImGui::SameLine();

	{
		ImGui::BeginGroup();
		ImGui::BeginChild("item view", ImVec2(0, 500), true);

		char szTag[MAX_PATH] = "";
		if (!ModelTags.empty())
			WideCharToMultiByte(CP_ACP, 0, ModelTags[selected], -1, szTag, MAX_PATH, NULL, NULL);

		ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.35f, 1.0f), "Selected : %s", szTag);
		ImGui::Separator();

		if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem("Setting"))
			{
				ImGui::NewLine();
				Set_LayerTag();
				Set_Object_Map();

				ImGui::NewLine();
				if (ImGui::Button("Add_Model", ImVec2(120, 30)))
				{
					_tchar* LayerTag = StringToTCHAR(m_stLayerTags[m_iSeletecLayerNum]);
					Create_Model(ModelTags[selected], LayerTag);
					m_TempLayerTags.push_back(LayerTag);
				}
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		ImGui::EndChild();
		ImGui::EndGroup();
	}

}

void CImGui_Manager::Show_CurrentModelList()
{
	vector<CNonAnim*> vecCreatedModel = m_pModel_Manager->Get_CreatedModel();

	{
		ImGuiWindowFlags listFlags =
			ImGuiWindowFlags_AlwaysVerticalScrollbar;

		ImGui::BeginChild("Object List", ImVec2(200, 500), true, listFlags);

		int i = 0;
		for (auto& iter : vecCreatedModel)
		{
			if (!iter) continue;

			char label[MAX_PATH] = "";
			char szLayertag[MAX_PATH] = "";

			std::string modelTag = iter->Get_Modeltag();
			modelTag += "_" + std::to_string(i);

			_tchar* realModelTag = StringToTCHAR(modelTag);
			WideCharToMultiByte(CP_ACP, 0, realModelTag, MAX_PATH, szLayertag, MAX_PATH, NULL, NULL);
			sprintf_s(label, "%s", szLayertag);
			delete[] realModelTag;

			if (ImGui::Selectable(label, m_iCreatedSelected == i))
			{
				m_iCreatedSelected = i;
				iter->Set_Picked();
			}
			++i;
		}

		ImGui::EndChild();
	}

	ImGui::SameLine();

	{
		ImGui::BeginGroup();
		ImGui::BeginChild("item view", ImVec2(0, 500), true);

		if (!vecCreatedModel.empty() &&
			m_iCreatedSelected >= 0 &&
			m_iCreatedSelected < (int)vecCreatedModel.size() &&
			vecCreatedModel[m_iCreatedSelected] != nullptr)
		{
			char nameA[MAX_PATH] = "";
			strcpy_s(nameA, MAX_PATH, vecCreatedModel[m_iCreatedSelected]->Get_Modeltag());

			ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "%s", nameA);
		}
		else
		{
			ImGui::TextUnformatted("No selection");
		}

		ImGui::Separator();

		if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem("Model"))
			{
				ShowPickedObj();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		ImGui::Spacing();
		ImGui::Separator();


		if (ImGui::Button("Copy", ImVec2(120, 30)))
		{
			if (m_iCreatedSelected >= 0 && m_iCreatedSelected < (int)vecCreatedModel.size())
			{
				if (CNonAnim* pSelected = vecCreatedModel[m_iCreatedSelected])
				{
					LEVEL eLevel = (LEVEL)CGameInstance::GetInstance()->Get_CurrentLevelIndex();
					_tchar* LayerTag = StringToTCHAR(m_stLayerTags[m_iSeletecLayerNum]);

					if (CNonAnim* pClone = m_pModel_Manager->Clone_Model(pSelected, eLevel, LayerTag, m_pDevice, m_pContext))
						pClone->Set_Picked();

					m_TempLayerTags.push_back(LayerTag);
				}
			}
		}

		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.60f, 1.00f, 1.00f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.40f, 0.70f, 1.00f, 1.00f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.50f, 0.90f, 1.00f));
		if (ImGui::Button("Save", ImVec2(120, 30))) m_bSave = true;
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 1.00f, 0.30f, 1.00f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.40f, 1.00f, 0.40f, 1.00f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.90f, 0.20f, 1.00f));
		if (ImGui::Button("Load", ImVec2(120, 30))) m_bLoad = true;
		ImGui::PopStyleColor(3);

		Show_PopupBox();

		ImGui::EndChild();
		ImGui::EndGroup();
	}

	if (m_PickingType == PICKING_MODEL && CGameInstance::GetInstance()->Key_Up(DIK_Z))
	{
		_tchar* LayerTag = StringToTCHAR(m_stLayerTags[m_iSeletecLayerNum]);
		m_iCurrentLevel = (LEVEL)CGameInstance::GetInstance()->Get_CurrentLevelIndex();
		list<CGameObject*>* plistClone = CGameInstance::GetInstance()->Get_ObjectList(m_iCurrentLevel, LayerTag);
		if (plistClone && !plistClone->empty())
		{
			auto iter = --plistClone->end();
			m_pModel_Manager->Out_CreatedModel(dynamic_cast<CNonAnim*>(*iter));
			Safe_Release(*iter);
			plistClone->erase(iter);
		}
		delete[] LayerTag;
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

void CImGui_Manager::Set_Navigation()
{
	static int selected = 0;

	{
		ImGui::BeginChild("left pane", ImVec2(150, 0), true);

		ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1), "Cell List");
		ImGui::Separator();

		if ((int)m_pNavigation_Manager->Get_CellsSize() != 0)
		{
			for (_uint i = 0; i < m_pNavigation_Manager->Get_CellsSize(); i++)
			{
				char label[64];
				sprintf(label, "Cell %d", i);

				if (ImGui::Selectable(label, m_iCellIndex == i))
					m_iCellIndex = i;
			}
		}
		else
		{
			ImGui::TextDisabled("No Cells");
		}

		ImGui::EndChild();
	}

	ImGui::SameLine();

	{
		ImGui::BeginGroup();
		ImGui::BeginChild("Cell View", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));

		ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1), "Cell");
		ImGui::Separator();

		ImGui::Text("Current Index: ");
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1, 0.7f, 0.2f, 1), "%d", m_iCellIndex);

		m_pNavigation_Manager->Set_ClickedCellIndex(m_iCellIndex);
		CCell* pCurrentCell = m_pNavigation_Manager->Get_Cell();

		static float fPointA[3], fPointB[3], fPointC[3];
		static float fClickedPosition[3];

		fClickedPosition[0] = m_fClickPoint.x;
		fClickedPosition[1] = m_fClickPoint.y;
		fClickedPosition[2] = m_fClickPoint.z;

		if (pCurrentCell != nullptr)
		{
			memcpy(fPointA, &pCurrentCell->Get_PointValue(CCell::POINT_A), sizeof(fPointA));
			memcpy(fPointB, &pCurrentCell->Get_PointValue(CCell::POINT_B), sizeof(fPointB));
			memcpy(fPointC, &pCurrentCell->Get_PointValue(CCell::POINT_C), sizeof(fPointC));

			m_iCellType = pCurrentCell->Get_CellType();
		}


		ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1), "POINT");

		if (ImGui::DragFloat3("Point A", fPointA, 0.01f))
			m_pNavigation_Manager->Set_CellPoint(m_iCellIndex, CCell::POINT_A, _float3(fPointA[0], fPointA[1], fPointA[2]));

		if (ImGui::DragFloat3("Point B", fPointB, 0.01f))
			m_pNavigation_Manager->Set_CellPoint(m_iCellIndex, CCell::POINT_B, _float3(fPointB[0], fPointB[1], fPointB[2]));

		if (ImGui::DragFloat3("Point C", fPointC, 0.01f))
			m_pNavigation_Manager->Set_CellPoint(m_iCellIndex, CCell::POINT_C, _float3(fPointC[0], fPointC[1], fPointC[2]));


		ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1), "Click Pos");
		ImGui::Text("ClickPoint:");
		ImGui::SameLine();

		if (ImGui::DragFloat3("ClickPoint", fClickedPosition, 0.01f))
		{
			m_fClickPoint = _float3(fClickedPosition[0], fClickedPosition[1], fClickedPosition[2]);
			m_pNavigation_Manager->Update_ClickedPostion(m_fClickPoint);
		}


		ImGui::Spacing();
		ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Utils");

		if (ImGui::Button("Cancel"))
			m_pNavigation_Manager->Cancle_Cell();

		if (ImGui::Button("Delete Selected"))
			m_pNavigation_Manager->Erase_Cell();

		if (ImGui::Button("Clear"))
			m_pNavigation_Manager->Clear_Cells();

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(1, 0.7f, 0.3f, 1), "Cell Type");

		if (ImGui::RadioButton("Default", &m_iCellType, 0))
			m_pNavigation_Manager->Set_CellType((CCell::CELLTYPE)m_iCellType);

		// -----------------------
		// Save / Load
		// -----------------------

		if (ImGui::Button("Save"))
			Save_Navigation();

		if (ImGui::Button("Load"))
			Load_Navigation();

		ImGui::EndChild();
		ImGui::EndGroup();
	}

	if (CGameInstance::GetInstance()->Key_Up(DIK_X))
	{
		if (CPickingMgr::GetInstance()->Picking())
		{
			_float3 fPosition = CPickingMgr::GetInstance()->Get_PickingPos();
			_vector vPosition = XMLoadFloat3(&fPosition);
			vPosition = XMVectorSetW(vPosition, 1.f);
			m_pNavigation_Manager->Click_Position(vPosition);
			m_fClickPoint = m_pNavigation_Manager->Get_ClickedPos();
		}
	}
	else if (m_bNaviPicking && CGameInstance::GetInstance()->Mouse_Up(DIMK_RBUTTON))
	{
		if (CPickingMgr::GetInstance()->Picking())
		{
			_float3 fPosition = CPickingMgr::GetInstance()->Get_PickingPos();
			_vector vPosition = XMLoadFloat3(&fPosition);
			vPosition = XMVectorSetW(vPosition, 1.f);
			m_pNavigation_Manager->Find_PickingCell(vPosition);

			m_iCellIndex = m_pNavigation_Manager->Get_CurrentCellIndex();
		}
	}

	m_pNavigation_Manager->Render();
}

void CImGui_Manager::Save_Navigation()
{
	OPENFILENAME OFN;
	TCHAR filePathName[300] = L"";
	TCHAR lpstrFile[300] = L"";
	static TCHAR filter[] = L"데이터파일\0*.dat\0텍스트 파일\0.txt";

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
		_float3 vPoints[3];
		CCell::CELLTYPE eCellType;
		m_iCurrentLevel = (LEVEL)CGameInstance::GetInstance()->Get_CurrentLevelIndex();

		iNum = (_int)m_pNavigation_Manager->Get_CellsSize();
		;
		hFile = CreateFile(OFN.lpstrFile, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (0 == hFile)
			return;

		// Cell 사이즈 갯수 저장
		WriteFile(hFile, &(iNum), sizeof(_uint), &dwByte, nullptr);

		for (_uint i = 0; i < iNum; ++i)
		{
			CCell* pCell = m_pNavigation_Manager->Get_Cell(i);
			vPoints[0] = pCell->Get_PointValue(CCell::POINT_A);
			vPoints[1] = pCell->Get_PointValue(CCell::POINT_B);
			vPoints[2] = pCell->Get_PointValue(CCell::POINT_C);
			eCellType = pCell->Get_CellType();

			WriteFile(hFile, vPoints, sizeof(_float3) * 3, &dwByte, nullptr);
			WriteFile(hFile, &eCellType, sizeof(CCell::CELLTYPE), &dwByte, nullptr);
		}
		
		CloseHandle(hFile);
	}
}

void CImGui_Manager::Load_Navigation()
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
		_uint iNum = 0;
		CCell::CELLTYPE eCellType;
		_float3 vPoints[3];

		hFile = CreateFile(OFN.lpstrFile, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (0 == hFile)
			return;

		ReadFile(hFile, &(iNum), sizeof(_uint), &dwByte, nullptr);
		
		for (_uint i = 0; i < iNum; ++i)
		{
			ReadFile(hFile, vPoints, sizeof(_float3) * 3, &dwByte, nullptr);
			ReadFile(hFile, &eCellType, sizeof(CCell::CELLTYPE), &dwByte, nullptr);
			if (0 == dwByte)
				break;

			m_pNavigation_Manager->Add_Cell(vPoints, eCellType);
		}
		CloseHandle(hFile);
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
		case MODEL_VAYNE:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Vayne/Vayne_Animation_Add.dat"));
			break;
		case MODEL_ALISTAR:
			lstrcat(pAnimationDataFilePath, TEXT("Champion/Alistar/Alistar_Animation_Add.dat"));
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
			Pivot.vScale = _float3(100.f, 100.f, 100.f);
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

void CImGui_Manager::Draw_EffectModals()
{
	/* Create Effect Modal */
	ImVec2 pCenter = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(pCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f)); // Center Window when appearing
	if (ImGui::BeginPopupModal("Create Effect", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Effect Created!");
		ImGui::NewLine();
		ImGui::Text("You can customize you effect by adding");
		ImGui::Text("Textures, Meshes and/or Particles.");
		ImGui::Separator();

		_float fButtonSize = 100.f;
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() / 2) - (fButtonSize / 2));

		if (ImGui::Button("Got it!", ImVec2(fButtonSize, 0)))
			ImGui::CloseCurrentPopup();

		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}

	/* Create Texture Curve Editor Modal */
	ImGui::SetNextWindowPos(pCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f)); // Center Window when appearing
	if (ImGui::BeginPopupModal("Texture Curve Editor", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
		if (ImGui::BeginTabBar("TextureCurveEditorTabBar", tab_bar_flags))
		{
			if (ImGui::BeginTabItem("Color Curves"))
			{
				if (ImGui::BeginTable("ColorCurvesTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame, ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 6)))
				{
					ImGui::TableSetupScrollFreeze(0, 1);
					ImGui::TableSetupColumn("ID");
					ImGui::TableSetupColumn("Color");
					ImGui::TableSetupColumn("Start");
					ImGui::TableSetupColumn("End");
					ImGui::TableHeadersRow();

					vector<array<_float, 5>> ColorCurves;

					if (m_pSelectedEffect)
						ColorCurves = m_pSelectedEffect->Get_ColorCurves();

					for (_uint i = 0; i < ColorCurves.size(); i++)
					{
						ImGui::TableNextColumn();
						string sColor = to_string(ColorCurves[i][0]) + ", " + to_string(ColorCurves[i][1]) + ", " + to_string(ColorCurves[i][2]);
						if (ImGui::Selectable(sColor.c_str(), i == m_iSelectedVelocityCurve, ImGuiSelectableFlags_SpanAllColumns)) /* Color */
							m_iSelectedVelocityCurve = i;

						ImGui::TableNextColumn();
						ImGui::Text(to_string(ColorCurves[i][3]).c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(ColorCurves[i][4]).c_str()); /* End */
					}

					ImGui::EndTable();
				}
				if (ImGui::Button("Delete"))
					if (m_pSelectedEffect->Get_ColorCurves().size() > m_iSelectedColorCurve)
						m_pSelectedEffect->Remove_ColorCurve(m_iSelectedColorCurve);

				ImGui::NewLine();

				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##ColorCurveR", &m_fCurveRed, 0.05f, 0.f, 1.f, "R: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##ColorCurveG", &m_fCurveGreen, 0.05f, 0.f, 1.f, "G: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##ColorCurveB", &m_fCurveBlue, 0.05f, 0.f, 1.f, "B: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##ColorStart", &m_fCurveStart, 0.05f, 0.f, 1.f, "Start: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##ColorEnd", &m_fCurveEnd, 0.05f, 0.f, 1.f, "End: %.02f"))
				{
				}
				ImGui::SameLine();

				if (ImGui::Button("Add"))
				{
					if (m_pSelectedEffect)
						m_pSelectedEffect->Add_ColorCurve({ m_fCurveRed, m_fCurveGreen, m_fCurveBlue, m_fCurveStart, m_fCurveEnd });
				}

				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Alpha Curves"))
			{
				if (ImGui::BeginTable("AlphaCurvesTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame, ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 6)))
				{
					ImGui::TableSetupScrollFreeze(0, 1);
					ImGui::TableSetupColumn("ID");
					ImGui::TableSetupColumn("Alpha");
					ImGui::TableSetupColumn("Start");
					ImGui::TableSetupColumn("End");
					ImGui::TableHeadersRow();

					vector<_float3> AlphaCurves;

					if (m_pSelectedEffect)
						AlphaCurves = m_pSelectedEffect->Get_AlphaCurves();

					for (_uint i = 0; i < AlphaCurves.size(); i++)
					{
						ImGui::TableNextColumn();
						if (ImGui::Selectable(to_string(i).c_str(), i == m_iSelectedAlphaCurve, ImGuiSelectableFlags_SpanAllColumns)) /* Alpha */
						{
							m_iSelectedAlphaCurve = i;
							m_fCurveValue = m_pSelectedEffect->Get_AlphaCurveIndex(m_iSelectedAlphaCurve).x;
							m_fCurveStart = m_pSelectedEffect->Get_AlphaCurveIndex(m_iSelectedAlphaCurve).y;
							m_fCurveEnd = m_pSelectedEffect->Get_AlphaCurveIndex(m_iSelectedAlphaCurve).z;
						}


						ImGui::TableNextColumn();
						ImGui::Text(to_string(AlphaCurves[i].x).c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(AlphaCurves[i].y).c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(AlphaCurves[i].z).c_str()); /* End */
					}

					ImGui::EndTable();
				}
				if (ImGui::Button("Delete"))
					if (m_pSelectedEffect->Get_AlphaCurves().size() > m_iSelectedAlphaCurve)
						m_pSelectedEffect->Remove_AlphaCurve(m_iSelectedAlphaCurve);

				ImGui::SameLine();
				if (ImGui::Button("Edit"))
					m_pSelectedEffect->Edit_AlphaCurve(m_iSelectedAlphaCurve, _float3(m_fCurveValue, m_fCurveStart, m_fCurveEnd));

				ImGui::NewLine();

				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##AlphaCurve", &m_fCurveValue, 0.05f, 0.f, 100.f, "Alpha: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##AlphaStart", &m_fCurveStart, 0.05f, 0.f, 1.f, "Start: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##AlphaEnd", &m_fCurveEnd, 0.05f, 0.f, 1.f, "End: %.02f"))
				{
				}
				ImGui::SameLine();

				if (ImGui::Button("Add"))
				{
					if (m_pSelectedEffect)
						m_pSelectedEffect->Add_AlphaCurve(_float3(m_fCurveValue, m_fCurveStart, m_fCurveEnd));

					_float fValue = m_fCurveEnd - m_fCurveStart;
					m_fCurveStart = m_fCurveEnd;
					m_fCurveEnd += fValue;
					if (m_fCurveEnd > 1.f)
						m_fCurveEnd = 1.f;
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Size Curves"))
			{
				if (ImGui::BeginTable("SizeCurvesTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame, ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 6)))
				{
					ImGui::TableSetupScrollFreeze(0, 1);
					ImGui::TableSetupColumn("ID");
					ImGui::TableSetupColumn("Size");
					ImGui::TableSetupColumn("Start");
					ImGui::TableSetupColumn("End");
					ImGui::TableHeadersRow();

					vector<_float3> SizeCurves;

					if (m_pSelectedEffect)
						SizeCurves = m_pSelectedEffect->Get_SizeCurves();

					for (_uint i = 0; i < SizeCurves.size(); i++)
					{
						ImGui::TableNextColumn();
						if (ImGui::Selectable(to_string(i).c_str(), i == m_iSelectedSizeCurve, ImGuiSelectableFlags_SpanAllColumns))
						{
							/* Size */
							m_iSelectedSizeCurve = i;
							m_fCurveValue = m_pSelectedEffect->Get_SizeCurveIndex(m_iSelectedSizeCurve).x;
							m_fCurveStart = m_pSelectedEffect->Get_SizeCurveIndex(m_iSelectedSizeCurve).y;
							m_fCurveEnd = m_pSelectedEffect->Get_SizeCurveIndex(m_iSelectedSizeCurve).z;
						}

						ImGui::TableNextColumn();
						ImGui::Text(to_string(SizeCurves[i].x).c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(SizeCurves[i].y).c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(SizeCurves[i].z).c_str()); /* End */
					}

					ImGui::EndTable();
				}
				if (ImGui::Button("Delete"))
					if (m_pSelectedEffect->Get_SizeCurves().size() > m_iSelectedSizeCurve)
						m_pSelectedEffect->Remove_SizeCurve(m_iSelectedSizeCurve);
				ImGui::SameLine();
				if (ImGui::Button("Edit"))
				{
					if (m_pSelectedEffect)
						m_pSelectedEffect->Edit_SizeCurve(m_iSelectedSizeCurve, _float3(m_fCurveValue, m_fCurveStart, m_fCurveEnd));
				}
				ImGui::NewLine();

				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##SizeCurve", &m_fCurveValue, 0.05f, 0.f, 100.f, "Size: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##SizeStart", &m_fCurveStart, 0.05f, 0.f, 1.f, "Start: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##SizeEnd", &m_fCurveEnd, 0.05f, 0.f, 1.f, "End: %.02f"))
				{
				}
				ImGui::SameLine();

				if (ImGui::Button("Add"))
				{
					if (m_pSelectedEffect)
						m_pSelectedEffect->Add_SizeCurve(_float3(m_fCurveValue, m_fCurveStart, m_fCurveEnd));

					_float fValue = m_fCurveEnd - m_fCurveStart;
					m_fCurveStart = m_fCurveEnd;
					m_fCurveEnd += fValue;
					if (m_fCurveEnd > 1.f)
						m_fCurveEnd = 1.f;
				}



				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::NewLine();
		if (ImGui::Button("Done"))
			ImGui::CloseCurrentPopup();

		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}

	/* Create Mesh Curve Editor Modal */
	ImGui::SetNextWindowPos(pCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f)); // Center Window when appearing
	if (ImGui::BeginPopupModal("Mesh Curve Editor", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
		if (ImGui::BeginTabBar("MeshCurveEditorTabBar", tab_bar_flags))
		{
			if (ImGui::BeginTabItem("Color Curves"))
			{
				if (ImGui::BeginTable("ColorCurvesTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame, ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 6)))
				{
					ImGui::TableSetupScrollFreeze(0, 1);
					ImGui::TableSetupColumn("ID");
					ImGui::TableSetupColumn("Color");
					ImGui::TableSetupColumn("Start");
					ImGui::TableSetupColumn("End");
					ImGui::TableHeadersRow();

					vector<array<_float, 5>> ColorCurves;

					if (m_pSelectedEffect)
						ColorCurves = m_pSelectedEffect->Get_ColorCurves();

					for (_uint i = 0; i < ColorCurves.size(); i++)
					{
						ImGui::TableNextColumn();
						if (ImGui::Selectable(to_string(i).c_str(), i == m_iSelectedVelocityCurve, ImGuiSelectableFlags_SpanAllColumns)) /* Color */
							m_iSelectedVelocityCurve = i;

						ImGui::TableNextColumn();
						string sColor = to_string(ColorCurves[i][0]) + ", " + to_string(ColorCurves[i][1]) + ", " + to_string(ColorCurves[i][2]);
						ImGui::Text(sColor.c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(ColorCurves[i][3]).c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(ColorCurves[i][4]).c_str()); /* End */
					}

					ImGui::EndTable();
				}
				if (ImGui::Button("Delete"))
					if (m_pSelectedEffect->Get_ColorCurves().size() > m_iSelectedColorCurve)
						m_pSelectedEffect->Remove_ColorCurve(m_iSelectedColorCurve);

				ImGui::NewLine();

				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##ColorCurveR", &m_fCurveRed, 0.05f, 0.f, 1.f, "R: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##ColorCurveG", &m_fCurveGreen, 0.05f, 0.f, 1.f, "G: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##ColorCurveB", &m_fCurveBlue, 0.05f, 0.f, 1.f, "B: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##ColorStart", &m_fCurveStart, 0.05f, 0.f, 1.f, "Start: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##ColorEnd", &m_fCurveEnd, 0.05f, 0.f, 1.f, "End: %.02f"))
				{
				}
				ImGui::SameLine();

				if (ImGui::Button("Add"))
				{
					if (m_pSelectedEffect)
						m_pSelectedEffect->Add_ColorCurve({ m_fCurveRed, m_fCurveGreen, m_fCurveBlue, m_fCurveStart, m_fCurveEnd });

					_float fValue = m_fCurveEnd - m_fCurveStart;
					m_fCurveStart = m_fCurveEnd;
					m_fCurveEnd += fValue;
					if (m_fCurveEnd > 1.f)
						m_fCurveEnd = 1.f;
				}

				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Alpha Curves"))
			{
				if (ImGui::BeginTable("AlphaCurvesTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame, ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 6)))
				{
					ImGui::TableSetupScrollFreeze(0, 1);
					ImGui::TableSetupColumn("ID");
					ImGui::TableSetupColumn("Alpha");
					ImGui::TableSetupColumn("Start");
					ImGui::TableSetupColumn("End");
					ImGui::TableHeadersRow();

					vector<_float3> AlphaCurves;

					if (m_pSelectedEffect)
						AlphaCurves = m_pSelectedEffect->Get_AlphaCurves();

					for (_uint i = 0; i < AlphaCurves.size(); i++)
					{
						ImGui::TableNextColumn();
						if (ImGui::Selectable(to_string(i).c_str(), i == m_iSelectedAlphaCurve, ImGuiSelectableFlags_SpanAllColumns)) /* Alpha */
							m_iSelectedAlphaCurve = i;

						ImGui::TableNextColumn();
						ImGui::Text(to_string(AlphaCurves[i].x).c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(AlphaCurves[i].y).c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(AlphaCurves[i].z).c_str()); /* End */
					}

					ImGui::EndTable();
				}
				if (ImGui::Button("Delete"))
					if (m_pSelectedEffect->Get_AlphaCurves().size() > m_iSelectedAlphaCurve)
						m_pSelectedEffect->Remove_AlphaCurve(m_iSelectedAlphaCurve);

				ImGui::NewLine();

				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##AlphaCurve", &m_fCurveValue, 0.05f, 0.f, 100.f, "Alpha: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##AlphaStart", &m_fCurveStart, 0.05f, 0.f, 1.f, "Start: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##AlphaEnd", &m_fCurveEnd, 0.05f, 0.f, 1.f, "End: %.02f"))
				{
				}
				ImGui::SameLine();

				if (ImGui::Button("Add"))
				{
					if (m_pSelectedEffect)
						m_pSelectedEffect->Add_AlphaCurve(_float3(m_fCurveValue, m_fCurveStart, m_fCurveEnd));

					_float fValue = m_fCurveEnd - m_fCurveStart;
					m_fCurveStart = m_fCurveEnd;
					m_fCurveEnd += fValue;
					if (m_fCurveEnd > 1.f)
						m_fCurveEnd = 1.f;
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Scale Curves"))
			{
				if (ImGui::BeginTable("ScaleCurvesTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame, ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 6)))
				{
					ImGui::TableSetupScrollFreeze(0, 1);
					ImGui::TableSetupColumn("ID");
					ImGui::TableSetupColumn("Scale");
					ImGui::TableSetupColumn("Start");
					ImGui::TableSetupColumn("End");
					ImGui::TableHeadersRow();

					vector<array<_float, 5>> ScaleCurves;

					if (m_pSelectedEffect)
						ScaleCurves = m_pSelectedEffect->Get_ScaleCurves();

					for (_uint i = 0; i < ScaleCurves.size(); i++)
					{
						ImGui::TableNextColumn();
						if (ImGui::Selectable(to_string(i).c_str(), i == m_iSelectedScaleCurve, ImGuiSelectableFlags_SpanAllColumns)) /* Scale */
						{
							m_iSelectedScaleCurve = i;
							array<_float, 5> SelectedCurve = m_pSelectedEffect->Get_ScaleCurveIndex(m_iSelectedScaleCurve);
							m_fCurveScaleX = SelectedCurve[0];
							m_fCurveScaleY = SelectedCurve[1];
							m_fCurveScaleZ = SelectedCurve[2];
							m_fCurveStart = SelectedCurve[3];
							m_fCurveEnd = SelectedCurve[4];

						}


						ImGui::TableNextColumn();
						string sScale = to_string(ScaleCurves[i][0]) + ", " + to_string(ScaleCurves[i][1]) + ", " + to_string(ScaleCurves[i][2]);
						ImGui::Text(sScale.c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(ScaleCurves[i][3]).c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(ScaleCurves[i][4]).c_str()); /* End */
					}

					ImGui::EndTable();
				}
				if (ImGui::Button("Delete"))
					if (m_pSelectedEffect->Get_ScaleCurves().size() > m_iSelectedScaleCurve)
						m_pSelectedEffect->Remove_ScaleCurve(m_iSelectedScaleCurve);
				ImGui::SameLine();

				if (ImGui::Button("Edit"))
				{
					if (m_pSelectedEffect)
						m_pSelectedEffect->Edit_ScaleCurve(m_iSelectedScaleCurve, { m_fCurveScaleX, m_fCurveScaleY, m_fCurveScaleZ, m_fCurveStart, m_fCurveEnd });
				}

				ImGui::NewLine();

				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##ScaleCurveX", &m_fCurveScaleX, 0.05f, 0.f, 100.f, "X: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##ScaleCurveY", &m_fCurveScaleY, 0.05f, 0.f, 100.f, "Y: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##ScaleCurveZ", &m_fCurveScaleZ, 0.05f, 0.f, 100.f, "Z: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##ScaleStart", &m_fCurveStart, 0.05f, 0.f, 1.f, "Start: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##ScaleEnd", &m_fCurveEnd, 0.05f, 0.f, 1.f, "End: %.02f"))
				{
				}
				ImGui::SameLine();

				if (ImGui::Button("Add"))
				{
					if (m_pSelectedEffect)
						m_pSelectedEffect->Add_ScaleCurve({ m_fCurveScaleX, m_fCurveScaleY, m_fCurveScaleZ, m_fCurveStart, m_fCurveEnd });

					_float fValue = m_fCurveEnd - m_fCurveStart;
					m_fCurveStart = m_fCurveEnd;
					m_fCurveEnd += fValue;
					if (m_fCurveEnd > 1.f)
						m_fCurveEnd = 1.f;
				}



				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Turn Velocity Curves"))
			{
				if (ImGui::BeginTable("TurnVelocityCurvesTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame, ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 6)))
				{
					ImGui::TableSetupScrollFreeze(0, 1);
					ImGui::TableSetupColumn("ID");
					ImGui::TableSetupColumn("Turn Velocity");
					ImGui::TableSetupColumn("Start");
					ImGui::TableSetupColumn("End");
					ImGui::TableHeadersRow();

					vector<_float3> TurnVelocityCurves;

					if (m_pSelectedEffect)
						TurnVelocityCurves = m_pSelectedEffect->Get_TurnVelocityCurves();

					for (_uint i = 0; i < TurnVelocityCurves.size(); i++)
					{
						ImGui::TableNextColumn();
						if (ImGui::Selectable(to_string(i).c_str(), i == m_iSelectedTurnVelocityCurve, ImGuiSelectableFlags_SpanAllColumns)) /* Alpha */
							m_iSelectedTurnVelocityCurve = i;

						ImGui::TableNextColumn();
						ImGui::Text(to_string(TurnVelocityCurves[i].x).c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(TurnVelocityCurves[i].y).c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(TurnVelocityCurves[i].z).c_str()); /* End */
					}

					ImGui::EndTable();
				}
				if (ImGui::Button("Delete"))
					if (m_pSelectedEffect->Get_TurnVelocityCurves().size() > m_iSelectedTurnVelocityCurve)
						m_pSelectedEffect->Remove_TurnVelocityCurve(m_iSelectedTurnVelocityCurve);

				ImGui::NewLine();

				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##TurnVelocityCurve", &m_fCurveValue, 0.05f, 0.f, 100.f, "Velocity: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##TurnVelocityStart", &m_fCurveStart, 0.05f, 0.f, 1.f, "Start: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##TurnVelocityEnd", &m_fCurveEnd, 0.05f, 0.f, 1.f, "End: %.02f"))
				{
				}
				ImGui::SameLine();

				if (ImGui::Button("Add"))
				{
					if (m_pSelectedEffect)
						m_pSelectedEffect->Add_TurnVelocityCurve(_float3(m_fCurveValue, m_fCurveStart, m_fCurveEnd));

					_float fValue = m_fCurveEnd - m_fCurveStart;
					m_fCurveStart = m_fCurveEnd;
					m_fCurveEnd += fValue;
					if (m_fCurveEnd > 1.f)
						m_fCurveEnd = 1.f;
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Noise Power Curves"))
			{
				if (ImGui::BeginTable("NoisePowerCurvesTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame, ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 6)))
				{
					ImGui::TableSetupScrollFreeze(0, 1);
					ImGui::TableSetupColumn("ID");
					ImGui::TableSetupColumn("Noise Power");
					ImGui::TableSetupColumn("Start");
					ImGui::TableSetupColumn("End");
					ImGui::TableHeadersRow();

					vector<_float3> NoisePowerCurves;

					if (m_pSelectedEffect)
						NoisePowerCurves = m_pSelectedEffect->Get_NoisePowerCurves();

					for (_uint i = 0; i < NoisePowerCurves.size(); i++)
					{
						ImGui::TableNextColumn();
						if (ImGui::Selectable(to_string(i).c_str(), i == m_iSelectedNoisePowerCurve, ImGuiSelectableFlags_SpanAllColumns)) /* Alpha */
							m_iSelectedNoisePowerCurve = i;

						ImGui::TableNextColumn();
						ImGui::Text(to_string(NoisePowerCurves[i].x).c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(NoisePowerCurves[i].y).c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(NoisePowerCurves[i].z).c_str()); /* End */
					}

					ImGui::EndTable();
				}
				if (ImGui::Button("Delete"))
					if (m_pSelectedEffect->Get_NoisePowerCurves().size() > m_iSelectedNoisePowerCurve)
						m_pSelectedEffect->Remove_NoisePowerCurve(m_iSelectedNoisePowerCurve);

				ImGui::NewLine();

				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##NoisePowerCurve", &m_fCurveValue, 0.05f, 0.f, 100.f, "Power: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##NoisePowerStart", &m_fCurveStart, 0.05f, 0.f, 1.f, "Start: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##NoisePowerEnd", &m_fCurveEnd, 0.05f, 0.f, 1.f, "End: %.02f"))
				{
				}
				ImGui::SameLine();

				if (ImGui::Button("Add"))
				{
					if (m_pSelectedEffect)
						m_pSelectedEffect->Add_NoisePowerCurve(_float3(m_fCurveValue, m_fCurveStart, m_fCurveEnd));

					_float fValue = m_fCurveEnd - m_fCurveStart;
					m_fCurveStart = m_fCurveEnd;
					m_fCurveEnd += fValue;
					if (m_fCurveEnd > 1.f)
						m_fCurveEnd = 1.f;
				}
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Distort Power Curves"))
			{
				if (ImGui::BeginTable("DistortPowerCurvesTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame, ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 6)))
				{
					ImGui::TableSetupScrollFreeze(0, 1);
					ImGui::TableSetupColumn("ID");
					ImGui::TableSetupColumn("Distort Power");
					ImGui::TableSetupColumn("Start");
					ImGui::TableSetupColumn("End");
					ImGui::TableHeadersRow();

					vector<_float3> DistortPowerCurves;

					if (m_pSelectedEffect)
						DistortPowerCurves = m_pSelectedEffect->Get_DistortPowerCurves();

					for (_uint i = 0; i < DistortPowerCurves.size(); i++)
					{
						ImGui::TableNextColumn();
						if (ImGui::Selectable(to_string(i).c_str(), i == m_iSelectedDistortPowerCurve, ImGuiSelectableFlags_SpanAllColumns)) /* Alpha */
							m_iSelectedDistortPowerCurve = i;

						ImGui::TableNextColumn();
						ImGui::Text(to_string(DistortPowerCurves[i].x).c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(DistortPowerCurves[i].y).c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(DistortPowerCurves[i].z).c_str()); /* End */
					}

					ImGui::EndTable();
				}
				if (ImGui::Button("Delete"))
					if (m_pSelectedEffect->Get_DistortPowerCurves().size() > m_iSelectedDistortPowerCurve)
						m_pSelectedEffect->Remove_DistortPowerCurve(m_iSelectedDistortPowerCurve);

				ImGui::NewLine();

				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##DistortPowerCurve", &m_fCurveValue, 0.05f, 0.f, 100.f, "Power: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##DistortPowerStart", &m_fCurveStart, 0.05f, 0.f, 1.f, "Start: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##DistortPowerEnd", &m_fCurveEnd, 0.05f, 0.f, 1.f, "End: %.02f"))
				{
				}
				ImGui::SameLine();

				if (ImGui::Button("Add"))
				{
					if (m_pSelectedEffect)
						m_pSelectedEffect->Add_DistortPowerCurve(_float3(m_fCurveValue, m_fCurveStart, m_fCurveEnd));

					_float fValue = m_fCurveEnd - m_fCurveStart;
					m_fCurveStart = m_fCurveEnd;
					m_fCurveEnd += fValue;
					if (m_fCurveEnd > 1.f)
						m_fCurveEnd = 1.f;
				}
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::NewLine();
		if (ImGui::Button("Done"))
			ImGui::CloseCurrentPopup();

		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}

	/* Create Particle Curve Editor Modal */
	ImGui::SetNextWindowPos(pCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f)); // Center Window when appearing
	if (ImGui::BeginPopupModal("Particle Curve Editor", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
		if (ImGui::BeginTabBar("ParticleCurveEditorTabBar", tab_bar_flags))
		{
			if (ImGui::BeginTabItem("Color Curves"))
			{
				if (ImGui::BeginTable("ColorCurvesTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame, ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 6)))
				{
					ImGui::TableSetupScrollFreeze(0, 1);
					ImGui::TableSetupColumn("ID");
					ImGui::TableSetupColumn("Color");
					ImGui::TableSetupColumn("Start");
					ImGui::TableSetupColumn("End");
					ImGui::TableHeadersRow();

					vector<array<_float, 5>> ColorCurves;

					if (m_pSelectedEffect)
						ColorCurves = m_pSelectedEffect->Get_ColorCurves();

					for (_uint i = 0; i < ColorCurves.size(); i++)
					{
						ImGui::TableNextColumn();
						if (ImGui::Selectable(to_string(i).c_str(), i == m_iSelectedVelocityCurve, ImGuiSelectableFlags_SpanAllColumns)) /* Color */
							m_iSelectedVelocityCurve = i;

						ImGui::TableNextColumn();
						string sColor = to_string(ColorCurves[i][0]) + ", " + to_string(ColorCurves[i][1]) + ", " + to_string(ColorCurves[i][2]);
						ImGui::Text(sColor.c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(ColorCurves[i][3]).c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(ColorCurves[i][4]).c_str()); /* End */
					}

					ImGui::EndTable();
				}
				if (ImGui::Button("Delete"))
					if (m_pSelectedEffect->Get_ColorCurves().size() > m_iSelectedColorCurve)
						m_pSelectedEffect->Remove_ColorCurve(m_iSelectedColorCurve);

				ImGui::NewLine();

				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##ColorCurveR", &m_fCurveRed, 0.05f, 0.f, 1.f, "R: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##ColorCurveG", &m_fCurveGreen, 0.05f, 0.f, 1.f, "G: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##ColorCurveB", &m_fCurveBlue, 0.05f, 0.f, 1.f, "B: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##ColorStart", &m_fCurveStart, 0.05f, 0.f, 1.f, "Start: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##ColorEnd", &m_fCurveEnd, 0.05f, 0.f, 1.f, "End: %.02f"))
				{
				}
				ImGui::SameLine();

				if (ImGui::Button("Add"))
				{
					if (m_pSelectedEffect)
						m_pSelectedEffect->Add_ColorCurve({ m_fCurveRed, m_fCurveGreen, m_fCurveBlue, m_fCurveStart, m_fCurveEnd });

					_float fValue = m_fCurveEnd - m_fCurveStart;
					m_fCurveStart = m_fCurveEnd;
					m_fCurveEnd += fValue;
					if (m_fCurveEnd > 1.f)
						m_fCurveEnd = 1.f;
				}

				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Alpha Curves"))
			{
				if (ImGui::BeginTable("AlphaCurvesTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame, ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 6)))
				{
					ImGui::TableSetupScrollFreeze(0, 1);
					ImGui::TableSetupColumn("ID");
					ImGui::TableSetupColumn("Alpha");
					ImGui::TableSetupColumn("Start");
					ImGui::TableSetupColumn("End");
					ImGui::TableHeadersRow();

					vector<_float3> AlphaCurves;

					if (m_pSelectedEffect)
						AlphaCurves = m_pSelectedEffect->Get_AlphaCurves();

					for (_uint i = 0; i < AlphaCurves.size(); i++)
					{
						ImGui::TableNextColumn();
						if (ImGui::Selectable(to_string(i).c_str(), i == m_iSelectedAlphaCurve, ImGuiSelectableFlags_SpanAllColumns)) /* Alpha */
							m_iSelectedAlphaCurve = i;

						ImGui::TableNextColumn();
						ImGui::Text(to_string(AlphaCurves[i].x).c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(AlphaCurves[i].y).c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(AlphaCurves[i].z).c_str()); /* End */
					}

					ImGui::EndTable();
				}
				if (ImGui::Button("Delete"))
					if (m_pSelectedEffect->Get_AlphaCurves().size() > m_iSelectedAlphaCurve)
						m_pSelectedEffect->Remove_AlphaCurve(m_iSelectedAlphaCurve);

				ImGui::NewLine();

				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##AlphaCurve", &m_fCurveValue, 0.05f, 0.f, 100.f, "Alpha: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##AlphaStart", &m_fCurveStart, 0.05f, 0.f, 1.f, "Start: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##AlphaEnd", &m_fCurveEnd, 0.05f, 0.f, 1.f, "End: %.02f"))
				{
				}
				ImGui::SameLine();

				if (ImGui::Button("Add"))
				{
					if (m_pSelectedEffect)
						m_pSelectedEffect->Add_AlphaCurve(_float3(m_fCurveValue, m_fCurveStart, m_fCurveEnd));
					_float fValue = m_fCurveEnd - m_fCurveStart;
					m_fCurveStart = m_fCurveEnd;
					m_fCurveEnd += fValue;
					if (m_fCurveEnd > 1.f)
						m_fCurveEnd = 1.f;
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Size Curves"))
			{
				if (ImGui::BeginTable("SizeCurvesTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame, ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 6)))
				{
					ImGui::TableSetupScrollFreeze(0, 1);
					ImGui::TableSetupColumn("ID");
					ImGui::TableSetupColumn("Size");
					ImGui::TableSetupColumn("Start");
					ImGui::TableSetupColumn("End");
					ImGui::TableHeadersRow();

					vector<_float3> SizeCurves;

					if (m_pSelectedEffect)
						SizeCurves = m_pSelectedEffect->Get_SizeCurves();

					for (_uint i = 0; i < SizeCurves.size(); i++)
					{
						ImGui::TableNextColumn();
						if (ImGui::Selectable(to_string(i).c_str(), i == m_iSelectedSizeCurve, ImGuiSelectableFlags_SpanAllColumns)) /* Size */
							m_iSelectedSizeCurve = i;

						ImGui::TableNextColumn();
						ImGui::Text(to_string(SizeCurves[i].x).c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(SizeCurves[i].y).c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(SizeCurves[i].z).c_str()); /* End */
					}

					ImGui::EndTable();
				}
				if (ImGui::Button("Delete"))
					if (m_pSelectedEffect->Get_SizeCurves().size() > m_iSelectedSizeCurve)
						m_pSelectedEffect->Remove_SizeCurve(m_iSelectedSizeCurve);

				ImGui::NewLine();

				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##SizeCurve", &m_fCurveValue, 0.05f, 0.f, 100.f, "Size: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##SizeStart", &m_fCurveStart, 0.05f, 0.f, 1.f, "Start: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##SizeEnd", &m_fCurveEnd, 0.05f, 0.f, 1.f, "End: %.02f"))
				{
				}
				ImGui::SameLine();

				if (ImGui::Button("Add"))
				{
					if (m_pSelectedEffect)
						m_pSelectedEffect->Add_SizeCurve(_float3(m_fCurveValue, m_fCurveStart, m_fCurveEnd));
					_float fValue = m_fCurveEnd - m_fCurveStart;
					m_fCurveStart = m_fCurveEnd;
					m_fCurveEnd += fValue;
					if (m_fCurveEnd > 1.f)
						m_fCurveEnd = 1.f;
				}

				if (ImGui::Button("Edit"))
				{
					if (m_pSelectedEffect)
						m_pSelectedEffect->Edit_SizeCurve(m_iSelectedSizeCurve, _float3(m_fCurveValue, m_fCurveStart, m_fCurveEnd));
				}


				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Velocity Curves"))
			{
				if (ImGui::BeginTable("VelocityCurvesTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame, ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 6)))
				{
					ImGui::TableSetupScrollFreeze(0, 1);
					ImGui::TableSetupColumn("ID");

					ImGui::TableSetupColumn("Velocity");
					ImGui::TableSetupColumn("Start");
					ImGui::TableSetupColumn("End");
					ImGui::TableHeadersRow();

					vector<_float3> VelocityCurves;

					if (m_pSelectedEffect)
						VelocityCurves = m_pSelectedEffect->Get_VelocityCurves();

					for (_uint i = 0; i < VelocityCurves.size(); i++)
					{
						ImGui::TableNextColumn();
						if (ImGui::Selectable(to_string(i).c_str(), i == m_iSelectedVelocityCurve, ImGuiSelectableFlags_SpanAllColumns)) /* Velocity */
							m_iSelectedVelocityCurve = i;

						ImGui::TableNextColumn();
						ImGui::Text(to_string(VelocityCurves[i].x).c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(VelocityCurves[i].y).c_str()); /* Start */

						ImGui::TableNextColumn();
						ImGui::Text(to_string(VelocityCurves[i].z).c_str()); /* End */
					}

					ImGui::EndTable();
				}
				if (ImGui::Button("Delete"))
					if (m_pSelectedEffect->Get_VelocityCurves().size() > m_iSelectedVelocityCurve)
						m_pSelectedEffect->Remove_VelocityCurve(m_iSelectedVelocityCurve);

				ImGui::NewLine();

				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##VelocityCurve", &m_fCurveValue, 0.05f, 0.f, 100.f, "Velocity: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##VelocityStart", &m_fCurveStart, 0.05f, 0.f, 1.f, "Start: %.02f"))
				{
				}
				ImGui::SameLine();
				ImGui::SetNextItemWidth(100);
				if (ImGui::DragFloat("##VelocityEnd", &m_fCurveEnd, 0.05f, 0.f, 1.f, "End: %.02f"))
				{
				}
				ImGui::SameLine();

				if (ImGui::Button("Add"))
				{
					if (m_pSelectedEffect)
						m_pSelectedEffect->Add_VelocityCurve(_float3(m_fCurveValue, m_fCurveStart, m_fCurveEnd));
					_float fValue = m_fCurveEnd - m_fCurveStart;
					m_fCurveStart = m_fCurveEnd;
					m_fCurveEnd += fValue;
					if (m_fCurveEnd > 1.f)
						m_fCurveEnd = 1.f;
				}

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::NewLine();
		if (ImGui::Button("Done"))
			ImGui::CloseCurrentPopup();

		ImGui::SetItemDefaultFocus();
		ImGui::EndPopup();
	}
}


void CImGui_Manager::Read_EffectsData()
{
	WIN32_FIND_DATA fileData;
	HANDLE hDir = FindFirstFile(TEXT("../../../Bin/Data/EffectData/*"), &fileData);

	/* No files found */
	if (hDir == INVALID_HANDLE_VALUE)
		return;

	do {
		if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		if (!lstrcmp(fileData.cFileName, TEXT(".")) || !lstrcmp(fileData.cFileName, TEXT("..")))
			continue;

		wstring wsFileName(fileData.cFileName);
		string sFileName(wsFileName.begin(), wsFileName.end());

		m_SavedEffects.push_back(sFileName);
	} while (FindNextFile(hDir, &fileData));

	FindClose(hDir);
}

void CImGui_Manager::Read_Textures(_tchar* pFolderPath)
{
	_tchar filePath[MAX_PATH] = TEXT("");
	wcscpy_s(filePath, MAX_PATH, pFolderPath); // Backup Path used for Sub-folders

	wcscat_s(pFolderPath, MAX_PATH, TEXT("*"));

	WIN32_FIND_DATA fileData;

	HANDLE hDir = FindFirstFile(pFolderPath, &fileData);

	/* No files found */
	if (hDir == INVALID_HANDLE_VALUE)
		return;

	do {
		if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) // Directory
		{
			if (lstrcmp(fileData.cFileName, TEXT(".")) == 0 || lstrcmp(fileData.cFileName, TEXT("..")) == 0)
				continue;

			_tchar subFilePath[MAX_PATH] = TEXT("");
			wcscpy_s(subFilePath, MAX_PATH, filePath);
			wcscat_s(subFilePath, MAX_PATH, fileData.cFileName);
			wcscat_s(subFilePath, MAX_PATH, TEXT("/"));

			// Recursive Function Call
			Read_Textures(subFilePath);
		}
		else // File
		{
			_tchar szFileExt[MAX_PATH];
			_tchar szFileName[MAX_PATH];
			_wsplitpath_s(fileData.cFileName, nullptr, 0, nullptr, 0, szFileName, MAX_PATH, szFileExt, MAX_PATH);

			if (!wcscmp(szFileExt, TEXT(".dds")) || !wcscmp(szFileExt, TEXT(".png")))
			{
				wstring wsFileName = wstring(szFileName);
				string sFileName = string(wsFileName.begin(), wsFileName.end());

				m_TextureNames.push_back(sFileName);
			}
		}
	} while (FindNextFile(hDir, &fileData));

	FindClose(hDir);
}

void CImGui_Manager::Read_Meshes(_tchar* pFolderPath)
{
	_tchar filePath[MAX_PATH] = TEXT("");
	wcscpy_s(filePath, MAX_PATH, pFolderPath); // Backup Path used for Sub-folders

	wcscat_s(pFolderPath, MAX_PATH, TEXT("*"));

	WIN32_FIND_DATA fileData;

	HANDLE hDir = FindFirstFile(pFolderPath, &fileData);

	/* No files found */
	if (hDir == INVALID_HANDLE_VALUE)
		return;

	do {
		if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) // Directory
		{
			if (lstrcmp(fileData.cFileName, TEXT(".")) == 0 || lstrcmp(fileData.cFileName, TEXT("..")) == 0)
				continue;

			_tchar subFilePath[MAX_PATH] = TEXT("");
			wcscpy_s(subFilePath, MAX_PATH, filePath);
			wcscat_s(subFilePath, MAX_PATH, fileData.cFileName);
			wcscat_s(subFilePath, MAX_PATH, TEXT("/"));

			// Recursive Function Call
			Read_Meshes(subFilePath);
		}
		else // File
		{
			_tchar szFileExt[MAX_PATH];
			_tchar szFileName[MAX_PATH];
			_wsplitpath_s(fileData.cFileName, nullptr, 0, nullptr, 0, szFileName, MAX_PATH, szFileExt, MAX_PATH);

			if (!wcscmp(szFileExt, TEXT(".dat")))
			{
				wstring wsFileName = wstring(szFileName);
				string sFileName = string(wsFileName.begin(), wsFileName.end());

				m_MeshNames.push_back(sFileName);
			}
		}
	} while (FindNextFile(hDir, &fileData));

	FindClose(hDir);
}

void CImGui_Manager::Set_Effect()
{
#pragma region Create Effect
	ImGuiTreeNodeFlags TreeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen;
	if (ImGui::CollapsingHeader("Create", TreeNodeFlags))
	{
		/* Create New Effect */
		ImGui::Text("Effect Name");
		static char cEffectName[MAX_PATH];
		ImGui::SetNextItemWidth(175);
		ImGui::InputText("##InputEffect", cEffectName, MAX_PATH);
		ImGui::SameLine();
		if (ImGui::Button("Create Effect"))
		{
			if (strcmp(cEffectName, ""))
			{
				m_sCurrentEffect = cEffectName; // Set Current Effect
				m_sCurrentEffect += ".dat";
				memset(cEffectName, 0, MAX_PATH);

				ImGui::OpenPopup("Create Effect");
			}
		}
		ImGui::NewLine();
		ImGui::Separator();
		ImGui::Text("Saved Effects");

		if (ImGui::BeginListBox("##SavedEffects", ImVec2(-FLT_MIN, 10 * ImGui::GetTextLineHeightWithSpacing())))
		{
			for (int i = 0; i < m_SavedEffects.size(); i++)
			{
				if (ImGui::Selectable(m_SavedEffects[i].c_str(), m_iSavedEffect == i))
					m_iSavedEffect = i;

				if (m_iSavedEffect == i)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndListBox();
		}

		if (ImGui::Button("Load Effect"))
			Load_Effect();

		//ImGui::SameLine();
		//if (ImGui::Button("Upgrade Effects"))
		//	Upgrade_Effects();

		ImGui::NewLine();
		ImGui::Separator();

		ImGui::Text("Current Effect:");
		ImGui::SameLine();
		ImGui::Text(m_sCurrentEffect.c_str());

		if (ImGui::Button("Save Effect"))
			Save_Effect();

		ImGui::NewLine();
	}
#pragma endregion Create

#pragma region Add
	if (ImGui::CollapsingHeader("Add"))
	{
		ImGui::Text("Available Resources");
		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
		if (ImGui::BeginTabBar("EffectTabBar", tab_bar_flags))
		{
			if (ImGui::BeginTabItem("Effect Textures"))
			{
				/* Available Textures (Effect) */
				if (ImGui::BeginListBox("##Texture List", ImVec2(-FLT_MIN, ImGui::GetWindowHeight() / 6)))
				{
					for (string sTextureName : m_TextureNames)
					{
						if (ImGui::Selectable(sTextureName.c_str(), m_sSelectedResource == sTextureName))
							m_sSelectedResource = sTextureName;

						if (m_sSelectedResource == sTextureName)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndListBox();

					if (ImGui::Button("Add Texture", ImVec2(100, 0)))
					{
						if (!m_sSelectedResource.empty())
						{
							CEffectTexture::TEXTUREEFFECTDESC tTextureEffectDesc;

							wstring wsTexturePrototype = wstring(m_sSelectedResource.begin(), m_sSelectedResource.end());
							wcscpy_s(tTextureEffectDesc.wcPrototypeId, MAX_PATH, wsTexturePrototype.c_str());

							CEffect* pEffect = nullptr;
							CGameInstance::GetInstance()->Add_GameObject_Out(TEXT("Prototype_GameObject_EffectTexture"), LEVEL_GAMEPLAY, TEXT("Layer_Effect"), (CGameObject*&)pEffect, &tTextureEffectDesc);

							m_pEffectManager->Add_Effect(pEffect);
							pEffect->Set_EffectType(CEffect::EFFECT_TYPE::TYPE_TEXTURE);
						}
					}
					ImGui::SameLine();

					if (ImGui::Button("Add Particle", ImVec2(100, 0)))
					{
						if (!m_sSelectedResource.empty())
						{
							PARTICLEDESC tParticleDesc;

							wstring wsTexturePrototype = wstring(m_sSelectedResource.begin(), m_sSelectedResource.end());
							wcscpy_s(tParticleDesc.wcPrototypeId, MAX_PATH, wsTexturePrototype.c_str());

							CEffect* pEffect = nullptr;
							CGameInstance::GetInstance()->Add_GameObject_Out(TEXT("Prototype_GameObject_ParticleSystem"), LEVEL_GAMEPLAY, TEXT("Layer_Effect"), (CGameObject*&)pEffect, &tParticleDesc);

							m_pEffectManager->Add_Effect(pEffect);
							pEffect->Set_EffectType(CEffect::EFFECT_TYPE::TYPE_PARTICLE);
						}
					}
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Effect Meshes"))
			{
				/* Available Meshes (Effect) */
				if (ImGui::BeginListBox("##Mesh List", ImVec2(-FLT_MIN, ImGui::GetWindowHeight() / 6)))
				{
					for (string sMeshName : m_MeshNames)
					{
						if (ImGui::Selectable(sMeshName.c_str(), m_sSelectedResource == sMeshName))
							m_sSelectedResource = sMeshName;

						if (m_sSelectedResource == sMeshName)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndListBox();

					if (ImGui::Button("Add Mesh", ImVec2(100, 0)))
					{
						if (!m_sSelectedResource.empty())
						{
							CEffectMesh::MESHEFFECTDESC tMeshEffectDesc;

							wstring wsMeshPrototype = wstring(m_sSelectedResource.begin(), m_sSelectedResource.end());
							wcscpy_s(tMeshEffectDesc.wcPrototypeId, MAX_PATH, wsMeshPrototype.c_str());

							CEffect* pEffect = nullptr;
							CGameInstance::GetInstance()->Add_GameObject_Out(TEXT("Prototype_GameObject_EffectMesh"), LEVEL_GAMEPLAY, TEXT("Layer_Effect"), (CGameObject*&)pEffect, &tMeshEffectDesc);

							m_pEffectManager->Add_Effect(pEffect);
							pEffect->Set_EffectType(CEffect::EFFECT_TYPE::TYPE_MESH);
						}
					}
				}
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		ImGui::NewLine();
		ImGui::Separator();

		/* Instanced Effects */
		ImGui::Text("Instanced Effects");

		if (ImGui::BeginListBox("##Instanced Effect List", ImVec2(-FLT_MIN, ImGui::GetWindowHeight() / 6)))
		{
			vector<CEffect*> Effects = CEffectManager::GetInstance()->Get_InstancedEffects();
			for (_uint i = 0; i < Effects.size(); i++)
			{
				_tchar wcEffectName[MAX_PATH];
				wcscpy_s(wcEffectName, MAX_PATH, Effects[i]->Get_PrototypeId()); /* Copy the Effect PrototypeId to a local _tchar variable. */

				_tchar wcCounter[MAX_PATH] = TEXT("_");
				wsprintf(wcCounter, TEXT("_%d"), i); /* Cast the Counter to _tchar. */

				wcscat_s(wcEffectName, MAX_PATH, wcCounter); /* Concatenate Effect Name and Counter. */

				wstring wsEffectName = wstring(wcEffectName);
				string sEffectName = string(wsEffectName.begin(), wsEffectName.end());

				Effects[i]->Set_EffectName(wcEffectName); /* Set the Effect Name with the _*, so that the delete function can address it properly. */

				if (ImGui::Selectable(sEffectName.c_str(), Effects[i] == m_pSelectedEffect, ImGuiSelectableFlags_AllowItemOverlap, ImVec2(0, 22)))
				{
					if (m_pSelectedEffect)
					{
						/* Backup the Effect Description in the Effect Class. */
						switch (m_pSelectedEffect->Get_EffectType())
						{
						case CEffect::EFFECT_TYPE::TYPE_TEXTURE:
						{
							((CEffectTexture*)m_pSelectedEffect)->Set_TextureEffectDescTool(m_tTextureEffectDesc);
							break;
						}
						case CEffect::EFFECT_TYPE::TYPE_MESH:
						{
							((CEffectMesh*)m_pSelectedEffect)->Set_MeshEffectDescTool(m_tMeshEffectDesc);
							break;
						}
						}
					}

					m_sSelectedEffect = sEffectName;
					m_pSelectedEffect = Effects[i];
					m_bIsPlaying = false;

					/* Fetch the Backed up Effect Description. */
					switch (m_pSelectedEffect->Get_EffectType())
					{
					case CEffect::EFFECT_TYPE::TYPE_TEXTURE:
					{
						m_tTextureEffectDesc = ((CEffectTexture*)m_pSelectedEffect)->Get_TextureEffectDescTool();
						wcscpy_s(m_tTextureEffectDesc.wcPrototypeId, MAX_PATH, m_pSelectedEffect->Get_PrototypeId());
						break;
					}
					case CEffect::EFFECT_TYPE::TYPE_MESH:
					{
						m_tMeshEffectDesc = ((CEffectMesh*)m_pSelectedEffect)->Get_MeshEffectDescTool();

						wstring wsMaskTexture = wstring(m_tMeshEffectDesc.wcMaskTexture);
						string sMaskTexture = string(wsMaskTexture.begin(), wsMaskTexture.end());
						m_sSelectedMaskTexture = sMaskTexture;

						wstring wsNoiseTexture = wstring(m_tMeshEffectDesc.wcNoiseTexture);
						string sNoiseTexture = string(wsNoiseTexture.begin(), wsNoiseTexture.end());
						m_sSelectedNoiseTexture = sNoiseTexture;

						wcscpy_s(m_tMeshEffectDesc.wcPrototypeId, MAX_PATH, m_pSelectedEffect->Get_PrototypeId());
						break;
					}
					case CEffect::EFFECT_TYPE::TYPE_PARTICLE:
					{
						m_tParticleDesc = ((CParticleSystem*)m_pSelectedEffect)->Get_ParticleDesc();
						m_sCurrentSpawnType = m_SpawnTypes[m_tParticleDesc.m_eSpawnType];
						break;
					}
					}

					// Set Transform of the Selected Effect
					CComponent* pComponent = Effects[i]->Find_Component(TEXT("Com_Transform"));
					if (!pComponent)
						return;
					CTransform* pTransform = dynamic_cast<CTransform*>(pComponent);
					if (!pTransform)
						return;

					m_pSelectedEffectTransform = pTransform;
				}
				ImGui::SameLine();
				ImGui::PushID(Effects[i]);
				ImGui::Checkbox("Show", &Effects[i]->m_bIsSelected);
				ImGui::PopID();

				if (m_sSelectedEffect == sEffectName)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndListBox();
		}

		if (ImGui::Button("Delete", ImVec2(60, 0)))
		{
			if (m_pSelectedEffect)
			{
				m_pEffectManager->Remove_Effect(m_pSelectedEffect);
				m_pSelectedEffect = nullptr;
				m_sSelectedEffect = "";
			}
		}

		ImGui::NewLine();
	}
#pragma endregion Add

#pragma region Customize
	if (ImGui::CollapsingHeader("Customize"))
	{
		if (ImGui::Button(m_bIsPlaying ? "Stop" : "Play", ImVec2(120, 0)))
		{
			if (m_pSelectedEffect)
			{
				m_bIsPlaying = !m_bIsPlaying;

				for (auto& pEffect : m_pEffectManager->Get_InstancedEffects())
				{
					if (pEffect->m_bIsSelected)
						pEffect->Set_Play(m_bIsPlaying);
				}

				if (m_bIsPlaying)
				{
					switch (m_pSelectedEffect->Get_EffectType())
					{
					case CEffect::EFFECT_TYPE::TYPE_TEXTURE:
					{
						((CEffectTexture*)m_pSelectedEffect)->Set_TextureEffectDesc(m_tTextureEffectDesc);
						break;
					}
					case CEffect::EFFECT_TYPE::TYPE_MESH:
					{
						((CEffectMesh*)m_pSelectedEffect)->Set_MeshEffectDesc(m_tMeshEffectDesc);
						break;
					}
					case CEffect::EFFECT_TYPE::TYPE_PARTICLE:
					{
						((CParticleSystem*)m_pSelectedEffect)->ResetParticleSystem();
						break;
					}
					}
				}
			}
		}

		if (m_pSelectedEffect)
		{
			switch (m_pSelectedEffect->Get_EffectType())
			{
			case CEffect::EFFECT_TYPE::TYPE_TEXTURE:
			{
				Show_TextureCustomization();

				ImGui::NewLine();
				ImGui::Separator();

				if (ImGui::Button("Curve Editor", ImVec2(100, 0)))
				{
					if (m_pSelectedEffect)
						ImGui::OpenPopup("Texture Curve Editor");
				}
				break;
			}
			case CEffect::EFFECT_TYPE::TYPE_MESH:
			{
				Show_MeshCustomization();

				ImGui::NewLine();
				ImGui::Separator();

				if (ImGui::Button("Curve Editor", ImVec2(100, 0)))
				{
					if (m_pSelectedEffect)
						ImGui::OpenPopup("Mesh Curve Editor");
				}
				break;
			}
			case CEffect::EFFECT_TYPE::TYPE_PARTICLE:
			{
				Show_ParticleCustomization();

				ImGui::NewLine();
				ImGui::Separator();

				if (ImGui::Button("Curve Editor", ImVec2(100, 0)))
				{
					if (m_pSelectedEffect)
						ImGui::OpenPopup("Particle Curve Editor");
				}
				break;
			}
			}
		}
	}
#pragma endregion Customize

	Draw_EffectModals();
}

_bool CImGui_Manager::Save_Effect()
{
	/* Backup the (selected) Effect Description in the Effect Class.
	Since the backup gets done when selecting effects, it's possible that the current one is not back upped. */
	if (m_pSelectedEffect)
	{
		switch (m_pSelectedEffect->Get_EffectType())
		{
		case CEffect::EFFECT_TYPE::TYPE_TEXTURE:
		{
			((CEffectTexture*)m_pSelectedEffect)->Set_TextureEffectDescTool(m_tTextureEffectDesc);
			break;
		}
		case CEffect::EFFECT_TYPE::TYPE_MESH:
		{
			((CEffectMesh*)m_pSelectedEffect)->Set_MeshEffectDescTool(m_tMeshEffectDesc);
			break;
		}
		}
	}

	wstring wsCurrentEffect = wstring(m_sCurrentEffect.begin(), m_sCurrentEffect.end());

	/* Create Effect Data File */
	HANDLE hFileEffect = nullptr;
	_tchar LoadPathEffect[MAX_PATH] = TEXT("../../../Bin/Data/EffectData/");
	wcscat_s(LoadPathEffect, MAX_PATH, wsCurrentEffect.c_str());

	hFileEffect = CreateFile(LoadPathEffect, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFileEffect == INVALID_HANDLE_VALUE)
		return false;

	DWORD dwByte = 0;

	/* Write how many Effects are in this File (needed when Loading). */
	_uint iEffectsCount = (_uint)m_pEffectManager->Get_InstancedEffects().size();
	WriteFile(hFileEffect, &iEffectsCount, sizeof(_uint), &dwByte, nullptr);

	vector<CEffect*> Effects = m_pEffectManager->Get_InstancedEffects();
	/* For every Effect in this File: */
	for (_uint i = 0; i < Effects.size(); i++)
	{
		/* Write Effect Type (needed when Loading). */
		CEffect::EFFECT_TYPE eType = Effects[i]->Get_EffectType();
		WriteFile(hFileEffect, &eType, sizeof(CEffect::EFFECT_TYPE), &dwByte, nullptr);

		/* Write Effect Type-specific Description. */
		switch (Effects[i]->Get_EffectType())
		{
		case CEffect::EFFECT_TYPE::TYPE_TEXTURE:
		{
			CEffectTexture::TEXTUREEFFECTDESC tTextureEffectDesc = ((CEffectTexture*)Effects[i])->Get_TextureEffectDescTool();
			WriteFile(hFileEffect, &tTextureEffectDesc, sizeof(CEffectTexture::TEXTUREEFFECTDESC), &dwByte, nullptr);
			break;
		}
		case CEffect::EFFECT_TYPE::TYPE_MESH:
		{
			CEffectMesh::MESHEFFECTDESC tMeshEffectDesc = ((CEffectMesh*)Effects[i])->Get_MeshEffectDescTool();
			WriteFile(hFileEffect, &tMeshEffectDesc, sizeof(CEffectMesh::MESHEFFECTDESC), &dwByte, nullptr);
			break;
		}
		case CEffect::EFFECT_TYPE::TYPE_PARTICLE:
		{
			PARTICLEDESC tParticleDesc = static_cast<CParticleSystem*>(Effects[i])->Get_ParticleDesc();
			WriteFile(hFileEffect, &tParticleDesc, sizeof(PARTICLEDESC), &dwByte, nullptr);
			break;
		}
		}

		/* COLOR */
		/* Write how many Color Curves there are for this Effect (needed when Loading). */
		_uint iColorCurvesCount = (_uint)(Effects[i]->Get_ColorCurves().size());
		WriteFile(hFileEffect, &iColorCurvesCount, sizeof(_uint), &dwByte, nullptr);
		/* Write Color Curves. */
		for (_uint j = 0; j < iColorCurvesCount; j++)
			WriteFile(hFileEffect, &Effects[i]->Get_ColorCurves()[j], sizeof(array<_float, 5>), &dwByte, nullptr);

		/* ALPHA */
		/* Write how many Alpha Curves there are for this Effect (needed when Loading). */
		_uint iAlphaCurvesCount = (_uint)(Effects[i]->Get_AlphaCurves().size());
		WriteFile(hFileEffect, &iAlphaCurvesCount, sizeof(_uint), &dwByte, nullptr);
		/* Write Alpha Curves. */
		for (_uint j = 0; j < iAlphaCurvesCount; j++)
			WriteFile(hFileEffect, &Effects[i]->Get_AlphaCurves()[j], sizeof(_float3), &dwByte, nullptr);

		/* SIZE */
		/* Write how many Size Curves there are for this Effect (needed when Loading). */
		_uint iSizeCurvesCount = (_uint)(Effects[i]->Get_SizeCurves().size());
		WriteFile(hFileEffect, &iSizeCurvesCount, sizeof(_uint), &dwByte, nullptr);
		/* Write Size Curves. */
		for (_uint j = 0; j < iSizeCurvesCount; j++)
			WriteFile(hFileEffect, &Effects[i]->Get_SizeCurves()[j], sizeof(_float3), &dwByte, nullptr);

		/* SCALE */
		/* Write how many Scale Curves there are for this Effect (needed when Loading). */
		_uint iScaleCurvesCount = (_uint)(Effects[i]->Get_ScaleCurves().size());
		WriteFile(hFileEffect, &iScaleCurvesCount, sizeof(_uint), &dwByte, nullptr);
		/* Write Scale Curves. */
		for (_uint j = 0; j < iScaleCurvesCount; j++)
			WriteFile(hFileEffect, &Effects[i]->Get_ScaleCurves()[j], sizeof(array<_float, 5>), &dwByte, nullptr);

		/* VELOCITY */
		/* Write how many Velocity Curves there are for this Effect (needed when Loading). */
		_uint iVelocityCurvesCount = (_uint)(Effects[i]->Get_VelocityCurves().size());
		WriteFile(hFileEffect, &iVelocityCurvesCount, sizeof(_uint), &dwByte, nullptr);
		/* Write Velocity Curves. */
		for (_uint j = 0; j < iVelocityCurvesCount; j++)
			WriteFile(hFileEffect, &Effects[i]->Get_VelocityCurves()[j], sizeof(_float3), &dwByte, nullptr);

		/* TURN VELOCITY */
		/* Write how many Turn Velocity Curves there are for this Effect (needed when Loading). */
		_uint iTurnVelocityCurvesCount = (_uint)(Effects[i]->Get_TurnVelocityCurves().size());
		WriteFile(hFileEffect, &iTurnVelocityCurvesCount, sizeof(_uint), &dwByte, nullptr);
		/* Write Turn Velocity Curves. */
		for (_uint j = 0; j < iTurnVelocityCurvesCount; j++)
			WriteFile(hFileEffect, &Effects[i]->Get_TurnVelocityCurves()[j], sizeof(_float3), &dwByte, nullptr);

		/* NOISE POWER */
		/* Write how many Noise Power Curves there are for this Effect (needed when Loading). */
		_uint iNoisePowerCurvesCount = (_uint)(Effects[i]->Get_NoisePowerCurves().size());
		WriteFile(hFileEffect, &iNoisePowerCurvesCount, sizeof(_uint), &dwByte, nullptr);
		/* Write Noise Power Curves. */
		for (_uint j = 0; j < iNoisePowerCurvesCount; j++)
			WriteFile(hFileEffect, &Effects[i]->Get_NoisePowerCurves()[j], sizeof(_float3), &dwByte, nullptr);

		/* DISTORT POWER */
		/* Write how many Distort Power Curves there are for this Effect (needed when Loading). */
		_uint iDistortPowerCurvesCount = (_uint)(Effects[i]->Get_DistortPowerCurves().size());
		WriteFile(hFileEffect, &iDistortPowerCurvesCount, sizeof(_uint), &dwByte, nullptr);
		/* Write Noise Power Curves. */
		for (_uint j = 0; j < iDistortPowerCurvesCount; j++)
			WriteFile(hFileEffect, &Effects[i]->Get_DistortPowerCurves()[j], sizeof(_float3), &dwByte, nullptr);
	}

	CloseHandle(hFileEffect);

	for (auto& sSavedEffect : m_SavedEffects)
	{
		if (sSavedEffect == m_sCurrentEffect)
		{
			sSavedEffect = m_sCurrentEffect;
			return true;
		}
	}

	m_SavedEffects.push_back(m_sCurrentEffect);

	return true;
}

_bool CImGui_Manager::Load_Effect(_bool bUpgrade)
{
	wstring wsSelectedSavedEffect = wstring(m_SavedEffects[m_iSavedEffect].begin(), m_SavedEffects[m_iSavedEffect].end());

	/* Load Effect File. */
	HANDLE hFileEffect = nullptr;
	_tchar LoadPathEffect[MAX_PATH] = TEXT("../../../Bin/Data/EffectData/");
	wcscat_s(LoadPathEffect, MAX_PATH, wsSelectedSavedEffect.c_str());

	hFileEffect = CreateFile(LoadPathEffect, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFileEffect == INVALID_HANDLE_VALUE)
		return false;

	CGameInstance* pGameInstance = CGameInstance::GetInstance();
	pGameInstance->AddRef();

	DWORD dwByte = 0;
	_uint iEffectsCount = 0;

	/* Read how many Effects there are in this File. */
	ReadFile(hFileEffect, &iEffectsCount, sizeof(_uint), &dwByte, nullptr);

	while (true)
	{
		if (!dwByte)
			break;

		/* For every Effect in this File: */
		for (_uint i = 0; i < iEffectsCount; i++)
		{
			/* Read Effect Type. */
			CEffect::EFFECT_TYPE eType;
			ReadFile(hFileEffect, &eType, sizeof(CEffect::EFFECT_TYPE), &dwByte, nullptr);

			CEffectTexture::TEXTUREEFFECTDESC tTextureEffectDesc;
			CEffectMesh::MESHEFFECTDESC tMeshEffectDesc; // Actual Logic
			CEffectMesh::MESHEFFECTDESC_OLD tMeshEffectDescOld; // Upgrade Logic
			PARTICLEDESC tParticleDesc;
			CEffect* pEffect = nullptr;

			/* Read Effect Type-specific Description. */
			switch (eType)
			{
			case CEffect::EFFECT_TYPE::TYPE_TEXTURE:
			{
				ReadFile(hFileEffect, &tTextureEffectDesc, sizeof(CEffectTexture::TEXTUREEFFECTDESC), &dwByte, nullptr);

				if (!dwByte)
					break;

				pGameInstance->Add_GameObject_Out(TEXT("Prototype_GameObject_EffectTexture"), LEVEL_GAMEPLAY, TEXT("Layer_Effects"), (CGameObject*&)pEffect, &tTextureEffectDesc);

				m_tTextureEffectDesc = tTextureEffectDesc;
				m_pEffectManager->Add_Effect(pEffect);
				((CEffectTexture*)pEffect)->Set_TextureEffectDescTool(tTextureEffectDesc);
				pEffect->Set_EffectType(CEffect::EFFECT_TYPE::TYPE_TEXTURE);
				break;
			}
			case CEffect::EFFECT_TYPE::TYPE_MESH:
			{
				if (bUpgrade)
					ReadFile(hFileEffect, &tMeshEffectDescOld, sizeof(CEffectMesh::MESHEFFECTDESC_OLD), &dwByte, nullptr); // Upgrade Logic
				else
					ReadFile(hFileEffect, &tMeshEffectDesc, sizeof(CEffectMesh::MESHEFFECTDESC), &dwByte, nullptr); // Actual Logic

				if (!dwByte)
					break;

				if (bUpgrade)
					tMeshEffectDesc = UpgradeMeshStruct(tMeshEffectDescOld); // Upgrade Logic

				pGameInstance->Add_GameObject_Out(TEXT("Prototype_GameObject_EffectMesh"), LEVEL_GAMEPLAY, TEXT("Layer_Effects"), (CGameObject*&)pEffect, &tMeshEffectDesc);

				m_tMeshEffectDesc = tMeshEffectDesc;
				m_pEffectManager->Add_Effect(pEffect);
				((CEffectMesh*)pEffect)->Set_MeshEffectDescTool(tMeshEffectDesc);
				pEffect->Set_EffectType(CEffect::EFFECT_TYPE::TYPE_MESH);

				wstring wsMaskTexture = wstring(m_tMeshEffectDesc.wcMaskTexture);
				string sMaskTexture = string(wsMaskTexture.begin(), wsMaskTexture.end());
				m_sSelectedMaskTexture = sMaskTexture;

				wstring wsNoiseTexture = wstring(m_tMeshEffectDesc.wcNoiseTexture);
				string sNoiseTexture = string(wsNoiseTexture.begin(), wsNoiseTexture.end());
				m_sSelectedNoiseTexture = sNoiseTexture;
				break;
			}
			case CEffect::EFFECT_TYPE::TYPE_PARTICLE:
			{
				ReadFile(hFileEffect, &tParticleDesc, sizeof(PARTICLEDESC), &dwByte, nullptr);

				if (!dwByte)
					break;

				pGameInstance->Add_GameObject_Out(TEXT("Prototype_GameObject_ParticleSystem"), LEVEL_GAMEPLAY, TEXT("Layer_Effects"), (CGameObject*&)pEffect, &tParticleDesc);

				m_tParticleDesc = tParticleDesc;
				m_pEffectManager->Add_Effect(pEffect);
				pEffect->Set_EffectType(CEffect::EFFECT_TYPE::TYPE_PARTICLE);

				m_sCurrentSpawnType = m_SpawnTypes[m_tParticleDesc.m_eSpawnType];
				break;
			}
			}

			/* COLOR */
			/* Read how many Color Curves there are for this Effect. */
			_uint iColorCurvesCount = 0;
			ReadFile(hFileEffect, &iColorCurvesCount, sizeof(_uint), &dwByte, nullptr);
			/* Read Color Curves. */
			vector<array<_float, 5>> ColorCurves;
			array<_float, 5> ColorCurve;
			for (_uint j = 0; j < iColorCurvesCount; j++)
			{
				ReadFile(hFileEffect, &ColorCurve, sizeof(array<_float, 5>), &dwByte, nullptr);
				ColorCurves.push_back(ColorCurve);
			}
			if (!ColorCurves.empty())
				pEffect->Set_ColorCurves(ColorCurves);

			/* ALPHA */
			/* Read how many Alpha Curves there are for this Effect. */
			_uint iAlphaCurvesCount = 0;
			ReadFile(hFileEffect, &iAlphaCurvesCount, sizeof(_uint), &dwByte, nullptr);
			/* Read Alpha Curves. */
			vector<_float3> AlphaCurves;
			_float3 AlphaCurve;
			for (_uint j = 0; j < iAlphaCurvesCount; j++)
			{
				ReadFile(hFileEffect, &AlphaCurve, sizeof(_float3), &dwByte, nullptr);
				AlphaCurves.push_back(AlphaCurve);
			}
			if (!AlphaCurves.empty())
				pEffect->Set_AlphaCurves(AlphaCurves);

			/* SIZE */
			/* Read how many Size Curves there are for this Effect. */
			_uint iSizeCurvesCount = 0;
			ReadFile(hFileEffect, &iSizeCurvesCount, sizeof(_uint), &dwByte, nullptr);
			/* Read Size Curves. */
			vector<_float3> SizeCurves;
			_float3 SizeCurve;
			for (_uint j = 0; j < iSizeCurvesCount; j++)
			{
				ReadFile(hFileEffect, &SizeCurve, sizeof(_float3), &dwByte, nullptr);
				SizeCurves.push_back(SizeCurve);
			}
			if (!SizeCurves.empty())
				pEffect->Set_SizeCurves(SizeCurves);

			/* SCALE */
			/* Read how many Scale Curves there are for this Effect. */
			_uint iScaleCurvesCount = 0;
			ReadFile(hFileEffect, &iScaleCurvesCount, sizeof(_uint), &dwByte, nullptr);
			/* Read Scale Curves. */
			vector<array<_float, 5>> ScaleCurves;
			array<_float, 5> ScaleCurve;
			for (_uint j = 0; j < iScaleCurvesCount; j++)
			{
				ReadFile(hFileEffect, &ScaleCurve, sizeof(array<_float, 5>), &dwByte, nullptr);
				ScaleCurves.push_back(ScaleCurve);
			}
			if (!ScaleCurves.empty())
				pEffect->Set_ScaleCurves(ScaleCurves);

			/* VELOCITY */
			/* Read how many Velocity Curves there are for this Effect. */
			_uint iVelocityCurvesCount = 0;
			ReadFile(hFileEffect, &iVelocityCurvesCount, sizeof(_uint), &dwByte, nullptr);
			/* Read Velocity Curves. */
			vector<_float3> VelocityCurves;
			_float3 VelocityCurve;
			for (_uint j = 0; j < iVelocityCurvesCount; j++)
			{
				ReadFile(hFileEffect, &VelocityCurve, sizeof(_float3), &dwByte, nullptr);
				VelocityCurves.push_back(VelocityCurve);
			}
			if (!VelocityCurves.empty())
				pEffect->Set_VelocityCurves(VelocityCurves);

			/* TURN VELOCITY */
			/* Read how many Turn Velocity Curves there are for this Effect. */
			_uint iTurnVelocityCurvesCount = 0;
			ReadFile(hFileEffect, &iTurnVelocityCurvesCount, sizeof(_uint), &dwByte, nullptr);
			/* Read Turn Velocity Curves. */
			vector<_float3> TurnVelocityCurves;
			_float3 TurnVelocityCurve;
			for (_uint j = 0; j < iTurnVelocityCurvesCount; j++)
			{
				ReadFile(hFileEffect, &TurnVelocityCurve, sizeof(_float3), &dwByte, nullptr);
				TurnVelocityCurves.push_back(TurnVelocityCurve);
			}
			if (!TurnVelocityCurves.empty())
				pEffect->Set_TurnVelocityCurves(TurnVelocityCurves);

			/* NOISE POWER */
			/* Read how many Noise Power Curves there are for this Effect. */
			_uint iNoisePowerCurvesCount = 0;
			ReadFile(hFileEffect, &iNoisePowerCurvesCount, sizeof(_uint), &dwByte, nullptr);
			/* Read Noise Power Curves. */
			vector<_float3> NoisePowerCurves;
			_float3 NoisePowerCurve;
			for (_uint j = 0; j < iNoisePowerCurvesCount; j++)
			{
				ReadFile(hFileEffect, &NoisePowerCurve, sizeof(_float3), &dwByte, nullptr);
				NoisePowerCurves.push_back(NoisePowerCurve);
			}
			if (!NoisePowerCurves.empty())
				pEffect->Set_NoisePowerCurves(NoisePowerCurves);

			/* DISTORT POWER */
			/* Read how many Distort Power Curves there are for this Effect. */
			_uint iDistortPowerCurvesCount = 0;
			ReadFile(hFileEffect, &iDistortPowerCurvesCount, sizeof(_uint), &dwByte, nullptr);
			/* Read Distort Power Curves. */
			vector<_float3> DistortPowerCurves;
			_float3 DistortPowerCurve;
			for (_uint j = 0; j < iDistortPowerCurvesCount; j++)
			{
				ReadFile(hFileEffect, &DistortPowerCurve, sizeof(_float3), &dwByte, nullptr);
				DistortPowerCurves.push_back(DistortPowerCurve);
			}
			if (!DistortPowerCurves.empty())
				pEffect->Set_DistortPowerCurves(DistortPowerCurves);
		}
	}


	pGameInstance->Release();

	CloseHandle(hFileEffect);


	m_sCurrentEffect = m_SavedEffects[m_iSavedEffect];

	return true;
}

_bool CImGui_Manager::Upgrade_Effects()
{
	for (_uint i = 0; i < m_SavedEffects.size(); i++)
	{
		m_iSavedEffect = i;

		Load_Effect(true);
		Save_Effect();

		m_pEffectManager->Clear_Effects();
	}

	m_sCurrentEffect.clear();

	return true;
}

CEffectMesh::MESHEFFECTDESC CImGui_Manager::UpgradeMeshStruct(CEffectMesh::MESHEFFECTDESC_OLD tMeshEffectDescOld)
{
	CEffectMesh::MESHEFFECTDESC tMeshEffectDesc;

	wcscpy_s(tMeshEffectDesc.wcPrototypeId, MAX_PATH, tMeshEffectDescOld.wcPrototypeId);
	wcscpy_s(tMeshEffectDesc.wcMaskTexture, MAX_PATH, tMeshEffectDescOld.wcMaskTexture);
	wcscpy_s(tMeshEffectDesc.wcNoiseTexture, MAX_PATH, tMeshEffectDescOld.wcNoiseTexture);
	tMeshEffectDesc.vColorInitial = tMeshEffectDescOld.vColorInitial;
	tMeshEffectDesc.vColor = tMeshEffectDescOld.vColor;
	tMeshEffectDesc.fAlphaInitial = tMeshEffectDescOld.fAlphaInitial;
	tMeshEffectDesc.fAlpha = tMeshEffectDescOld.fAlpha;
	tMeshEffectDesc.bGlow = tMeshEffectDescOld.bGlow;
	tMeshEffectDesc.vGlowColor = tMeshEffectDescOld.vGlowColor;
	tMeshEffectDesc.fGlowPower = tMeshEffectDescOld.fGlowPower;
	tMeshEffectDesc.bDistort = tMeshEffectDescOld.bDistort;
	tMeshEffectDesc.fDistortPowerInitial = tMeshEffectDescOld.fDistortPowerInitial;
	tMeshEffectDesc.fDistortPower = tMeshEffectDescOld.fDistortPower;
	tMeshEffectDesc.fDistortSpeed = tMeshEffectDescOld.fDistortSpeed;
	tMeshEffectDesc.fLifetime = tMeshEffectDescOld.fLifetime;
	tMeshEffectDesc.fStartAfter = tMeshEffectDescOld.fStartAfter;
	tMeshEffectDesc.vPosition = tMeshEffectDescOld.vPosition;
	tMeshEffectDesc.vRotation = tMeshEffectDescOld.vRotation;
	tMeshEffectDesc.vScaleInitial = tMeshEffectDescOld.vScaleInitial;
	tMeshEffectDesc.vScale = tMeshEffectDescOld.vScale;
	tMeshEffectDesc.vTurn = tMeshEffectDescOld.vTurn;
	tMeshEffectDesc.fTurnVelocityInitial = tMeshEffectDescOld.fTurnVelocityInitial;
	tMeshEffectDesc.fTurnVelocity = tMeshEffectDescOld.fTurnVelocity;
	tMeshEffectDesc.fMaskSpeed = tMeshEffectDescOld.fMaskSpeed;
	tMeshEffectDesc.fMaskDirectionX = tMeshEffectDescOld.fMaskDirectionX;
	tMeshEffectDesc.fMaskDirectionY = tMeshEffectDescOld.fMaskDirectionY;
	tMeshEffectDesc.fNoiseSpeed = tMeshEffectDescOld.fNoiseSpeed;
	tMeshEffectDesc.fNoiseDirectionX = tMeshEffectDescOld.fNoiseDirectionX;
	tMeshEffectDesc.fNoiseDirectionY = tMeshEffectDescOld.fNoiseDirectionY;
	tMeshEffectDesc.fNoisePowerInitial = tMeshEffectDescOld.fNoisePowerInitial;
	tMeshEffectDesc.fNoisePower = tMeshEffectDescOld.fNoisePower;

	/* The "new" Data will get automatically populated since its default initialized. */

	return tMeshEffectDesc;
}

void CImGui_Manager::Show_TextureCustomization()
{
	ImGui::NewLine();
	ImGui::Separator();

	ImGui::Text("Texture Customization");
	ImGui::NewLine();

	/*ImGui::Text("Shader");
	ImGui::SameLine();
	if (ImGui::BeginCombo("##Shader", m_sCurrentShaderTexture.c_str()))
	{
		_uint iCounter = 0;
		for (auto& iter = m_ShadersTexture.begin(); iter != m_ShadersTexture.end(); iter++)
		{
			if (ImGui::Selectable(iter->c_str(), iter->c_str() == m_sCurrentShaderTexture))
			{
				m_sCurrentShaderTexture = *iter;
				m_pSelectedEffect->Set_ShaderId(iCounter);
			}

			if (iter->c_str() == m_sCurrentShaderTexture)
				ImGui::SetItemDefaultFocus();

			iCounter++;
		}
		ImGui::EndCombo();
	}
	ImGui::NewLine();*/

	ImGui::Text("Color");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##TextureColorR", &m_tTextureEffectDesc.vColor.x, 0.05f, 0, 1, "R: %.02f"))
	{
		CEffectTexture* pEffectTexture = dynamic_cast<CEffectTexture*>(m_pSelectedEffect);
		if (pEffectTexture)
		{
			m_tTextureEffectDesc.vColorInitial = m_tTextureEffectDesc.vColor;
			pEffectTexture->Set_TextureEffectDesc(m_tTextureEffectDesc);
		}
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##TextureColorG", &m_tTextureEffectDesc.vColor.y, 0.05f, 0, 1, "G: %.02f"))
	{
		CEffectTexture* pEffectTexture = dynamic_cast<CEffectTexture*>(m_pSelectedEffect);
		if (pEffectTexture)
		{
			m_tTextureEffectDesc.vColorInitial = m_tTextureEffectDesc.vColor;
			pEffectTexture->Set_TextureEffectDesc(m_tTextureEffectDesc);
		}
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##TextureColorB", &m_tTextureEffectDesc.vColor.z, 0.05f, 0, 1, "B: %.02f"))
	{
		CEffectTexture* pEffectTexture = dynamic_cast<CEffectTexture*>(m_pSelectedEffect);
		if (pEffectTexture)
		{
			m_tTextureEffectDesc.vColorInitial = m_tTextureEffectDesc.vColor;
			pEffectTexture->Set_TextureEffectDesc(m_tTextureEffectDesc);
		}
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##TextureAlpha", &m_tTextureEffectDesc.fAlpha, 0.05f, 0, 1, "A: %.02f"))
	{
		CEffectTexture* pEffectTexture = dynamic_cast<CEffectTexture*>(m_pSelectedEffect);
		if (pEffectTexture)
		{
			m_tTextureEffectDesc.fAlphaInitial = m_tTextureEffectDesc.fAlpha;
			pEffectTexture->Set_TextureEffectDesc(m_tTextureEffectDesc);
		}
	}
	ImGui::Text("Alpha Discard");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##TextureAlphaDiscard", &m_tTextureEffectDesc.m_fAlphaDiscard, 0.05f, 0, 1, "A: %.02f"))
	{
		CEffectTexture* pEffectTexture = dynamic_cast<CEffectTexture*>(m_pSelectedEffect);
		if (pEffectTexture)
			pEffectTexture->Set_TextureEffectDesc(m_tTextureEffectDesc);
	}
	ImGui::NewLine();

	ImGui::Checkbox("Glow", &m_tTextureEffectDesc.m_bGlow);
	{
		CEffectTexture* pEffectTexture = dynamic_cast<CEffectTexture*>(m_pSelectedEffect);
		if (pEffectTexture)
			pEffectTexture->Set_TextureEffectDesc(m_tTextureEffectDesc);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##TextureGlowColorR", &m_tTextureEffectDesc.vGlowColor.x, 0.05f, 0, 1, "R: %.02f"))
	{
		CEffectTexture* pEffectTexture = dynamic_cast<CEffectTexture*>(m_pSelectedEffect);
		if (pEffectTexture)
			pEffectTexture->Set_TextureEffectDesc(m_tTextureEffectDesc);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##TextureGlowColorG", &m_tTextureEffectDesc.vGlowColor.y, 0.05f, 0, 1, "G: %.02f"))
	{
		CEffectTexture* pEffectTexture = dynamic_cast<CEffectTexture*>(m_pSelectedEffect);
		if (pEffectTexture)
			pEffectTexture->Set_TextureEffectDesc(m_tTextureEffectDesc);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##TextureGlowColorB", &m_tTextureEffectDesc.vGlowColor.z, 0.05f, 0, 1, "B: %.02f"))
	{
		CEffectTexture* pEffectTexture = dynamic_cast<CEffectTexture*>(m_pSelectedEffect);
		if (pEffectTexture)
			pEffectTexture->Set_TextureEffectDesc(m_tTextureEffectDesc);
	}
	ImGui::Checkbox("Distortion", &m_tTextureEffectDesc.m_bDistortion);
	{
		CEffectTexture* pEffectTexture = dynamic_cast<CEffectTexture*>(m_pSelectedEffect);
		if (pEffectTexture)
			pEffectTexture->Set_TextureEffectDesc(m_tTextureEffectDesc);
	}
	ImGui::NewLine();

	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##TextureLifetime", &m_tTextureEffectDesc.fLifetime, 0.05f, 0, 0, "Lifetime: %.02f"))
	{
		CEffectTexture* pEffectTexture = dynamic_cast<CEffectTexture*>(m_pSelectedEffect);
		if (pEffectTexture)
			pEffectTexture->Set_TextureEffectDesc(m_tTextureEffectDesc);
	}
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##TextureStartAfter", &m_tTextureEffectDesc.fStartAfter, 0.05f, 0, 0, "Start After: %.02f"))
	{
		CEffectTexture* pEffectTexture = dynamic_cast<CEffectTexture*>(m_pSelectedEffect);
		if (pEffectTexture)
			pEffectTexture->Set_TextureEffectDesc(m_tTextureEffectDesc);
	}
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##TextureSize", &m_tTextureEffectDesc.fSize, 0.05f, .1f, 100.f, "Size: %.02f"))
	{
		CEffectTexture* pEffectTexture = dynamic_cast<CEffectTexture*>(m_pSelectedEffect);
		if (pEffectTexture)
		{
			m_tTextureEffectDesc.fInitialSize = m_tTextureEffectDesc.fSize;
			pEffectTexture->Set_TextureEffectDesc(m_tTextureEffectDesc);
		}
	}
}

void CImGui_Manager::Show_MeshCustomization()
{
	ImGui::NewLine();
	ImGui::Separator();

	ImGui::Text("Mesh Customization");
	ImGui::NewLine();

	/*ImGui::Text("Shader");
	ImGui::SameLine();
	if (ImGui::BeginCombo("##Shader", m_sCurrentShaderMesh.c_str()))
	{
		_uint iCounter = 0;
		for (auto& iter = m_ShadersMesh.begin(); iter != m_ShadersMesh.end(); iter++)
		{
			if (ImGui::Selectable(iter->c_str(), iter->c_str() == m_sCurrentShaderMesh))
			{
				m_sCurrentShaderMesh = *iter;
				m_pSelectedEffect->Set_ShaderId(iCounter);
			}

			if (iter->c_str() == m_sCurrentShaderMesh)
				ImGui::SetItemDefaultFocus();

			iCounter++;
		}
		ImGui::EndCombo();
	}
	ImGui::NewLine();*/

	ImGui::Text("Color");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##MeshColorR", &m_tMeshEffectDesc.vColor.x, 0.05f, 0, 1, "R: %.02f"))
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
		{
			m_tMeshEffectDesc.vColorInitial = m_tMeshEffectDesc.vColor;
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
		}
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##MeshColorG", &m_tMeshEffectDesc.vColor.y, 0.05f, 0, 1, "G: %.02f"))
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
		{
			m_tMeshEffectDesc.vColorInitial = m_tMeshEffectDesc.vColor;
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
		}
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##MeshColorB", &m_tMeshEffectDesc.vColor.z, 0.05f, 0, 1, "B: %.02f"))
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
		{
			m_tMeshEffectDesc.vColorInitial = m_tMeshEffectDesc.vColor;
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
		}
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##MeshAlpha", &m_tMeshEffectDesc.fAlpha, 0.05f, 0, 1, "A: %.02f"))
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
		{
			m_tMeshEffectDesc.fAlphaInitial = m_tMeshEffectDesc.fAlpha;
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
		}
	}
	ImGui::NewLine();

	ImGui::Checkbox("Glow", &m_tMeshEffectDesc.bGlow);
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##MeshGlowColorR", &m_tMeshEffectDesc.vGlowColor.x, 0.05f, 0, 1, "R: %.02f"))
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##MeshGlowColorG", &m_tMeshEffectDesc.vGlowColor.y, 0.05f, 0, 1, "G: %.02f"))
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##MeshGlowColorB", &m_tMeshEffectDesc.vGlowColor.z, 0.05f, 0, 1, "B: %.02f"))
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
	}
	if (ImGui::DragFloat("##MeshGlowPower", &m_tMeshEffectDesc.fGlowPower, 0.05f, 0, 1, "Glow Power: %.02f"))
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
	}
	ImGui::NewLine();

	ImGui::Checkbox("Distort", &m_tMeshEffectDesc.bDistort);
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
	}
	ImGui::SetNextItemWidth(200.f);
	if (ImGui::DragFloat("##DistortionSpeed", &m_tMeshEffectDesc.fDistortSpeed, 0.05f, 0, 0, "Distortion Speed: %.02f"))
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200.f);
	if (ImGui::DragFloat("##DistortionPower", &m_tMeshEffectDesc.fDistortPower, 0.05f, 0, 0, "Distortion Power: %.02f"))
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
		{
			m_tMeshEffectDesc.fDistortPowerInitial = m_tMeshEffectDesc.fDistortPower;
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
		}
	}
	ImGui::NewLine();

	_float3 vScale, vRotation, vTranslation;
	if (ImGui::RadioButton("Scale", m_eCurrentTransformation == TRANS_SCALE))
	{
		m_eCurrentTransformation = TRANS_SCALE;

		if (m_pSelectedEffectTransform)
		{
			m_fX = m_pSelectedEffectTransform->Get_Scale(CTransform::STATE::STATE_RIGHT);
			m_fY = m_pSelectedEffectTransform->Get_Scale(CTransform::STATE::STATE_UP);
			m_fZ = m_pSelectedEffectTransform->Get_Scale(CTransform::STATE::STATE_LOOK);
		}
	}
	ImGui::SameLine();

	if (ImGui::RadioButton("Rotation", m_eCurrentTransformation == TRANS_ROTATION))
	{
		m_eCurrentTransformation = TRANS_ROTATION;

		if (m_pSelectedEffectTransform)
		{
			m_fX = m_pSelectedEffectTransform->Get_CurrentRotationX();
			m_fY = m_pSelectedEffectTransform->Get_CurrentRotationY();
			m_fZ = m_pSelectedEffectTransform->Get_CurrentRotationZ();
		}
	}
	ImGui::SameLine();

	if (ImGui::RadioButton("Translation", m_eCurrentTransformation == TRANS_TRANSLATION))
	{
		m_eCurrentTransformation = TRANS_TRANSLATION;

		if (m_pSelectedEffectTransform)
		{
			_float3 vCurrentTranslation;
			XMStoreFloat3(&vCurrentTranslation, m_pSelectedEffectTransform->Get_State(CTransform::STATE::STATE_TRANSLATION));
			m_fX = vCurrentTranslation.x;
			m_fY = vCurrentTranslation.y;
			m_fZ = vCurrentTranslation.z;
		}
	}

	ImGui::SetNextItemWidth(80);
	if (ImGui::DragFloat("##X", &m_fX, m_eCurrentTransformation == TRANS_ROTATION ? 1 : 0.05f, m_eCurrentTransformation == TRANS_ROTATION ? -360.f : 0.f, m_eCurrentTransformation == TRANS_ROTATION ? 360.f : 0.f, "X: %.03f"))
	{
		switch (m_eCurrentTransformation)
		{
		case TRANS_SCALE:
		{
			if (m_pSelectedEffectTransform)
			{
				m_tMeshEffectDesc.vScaleInitial.x = m_fX;
				m_tMeshEffectDesc.vScale.x = m_fX;

				CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
				if (pEffectMesh)
					pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
			}
			break;
		}
		case TRANS_ROTATION:
		{
			if (m_pSelectedEffectTransform)
			{
				m_tMeshEffectDesc.vRotation = _float3(m_fX, m_fY, m_fZ);

				CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
				if (pEffectMesh)
					pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);

				m_pSelectedEffectTransform->Set_Rotation(_float3(m_fX, m_fY, m_fZ));
			}
			break;
		}
		case TRANS_TRANSLATION:
		{
			if (m_pSelectedEffectTransform)
			{
				m_tMeshEffectDesc.vPosition = _float3(m_fX, m_fY, m_fZ);

				CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
				if (pEffectMesh)
					pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);

				XMStoreFloat3(&vTranslation, m_pSelectedEffectTransform->Get_State(CTransform::STATE::STATE_TRANSLATION));
				_vector vNewTranslation = XMVectorSet(m_fX, vTranslation.y, vTranslation.z, 1.f);

				m_pSelectedEffectTransform->Set_State(CTransform::STATE::STATE_TRANSLATION, vNewTranslation);
			}
			break;
		}
		}
	}
	ImGui::SameLine();

	ImGui::SetNextItemWidth(80);
	if (ImGui::DragFloat("##Y", &m_fY, m_eCurrentTransformation == TRANS_ROTATION ? 1 : 0.05f, m_eCurrentTransformation == TRANS_ROTATION ? -360.f : 0.f, m_eCurrentTransformation == TRANS_ROTATION ? 360.f : 0.f, "Y: %.03f"))
	{
		switch (m_eCurrentTransformation)
		{
		case TRANS_SCALE:
		{
			if (m_pSelectedEffectTransform)
			{
				m_tMeshEffectDesc.vScaleInitial.y = m_fY;
				m_tMeshEffectDesc.vScale.y = m_fY;

				CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
				if (pEffectMesh)
					pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
			}
			break;
		}
		case TRANS_ROTATION:
		{
			if (m_pSelectedEffectTransform)
			{
				m_tMeshEffectDesc.vRotation = _float3(m_fX, m_fY, m_fZ);

				CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
				if (pEffectMesh)
					pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);

				m_pSelectedEffectTransform->Set_Rotation(_float3(m_fX, m_fY, m_fZ));
			}
			break;
		}
		case TRANS_TRANSLATION:
		{
			if (m_pSelectedEffectTransform)
			{
				m_tMeshEffectDesc.vPosition = _float3(m_fX, m_fY, m_fZ);

				CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
				if (pEffectMesh)
					pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);

				XMStoreFloat3(&vTranslation, m_pSelectedEffectTransform->Get_State(CTransform::STATE::STATE_TRANSLATION));
				_vector vNewTranslation = XMVectorSet(vTranslation.x, m_fY, vTranslation.z, 1.f);

				m_pSelectedEffectTransform->Set_State(CTransform::STATE::STATE_TRANSLATION, vNewTranslation);
			}
			break;
		}
		}
	}
	ImGui::SameLine();

	ImGui::SetNextItemWidth(80);
	if (ImGui::DragFloat("##Z", &m_fZ, m_eCurrentTransformation == TRANS_ROTATION ? 1 : 0.05f, m_eCurrentTransformation == TRANS_ROTATION ? -360.f : 0.f, m_eCurrentTransformation == TRANS_ROTATION ? 360.f : 0.f, "Z: %.03f"))
	{
		switch (m_eCurrentTransformation)
		{
		case TRANS_SCALE:
		{
			if (m_pSelectedEffectTransform)
			{
				m_tMeshEffectDesc.vScaleInitial.z = m_fZ;
				m_tMeshEffectDesc.vScale.z = m_fZ;

				CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
				if (pEffectMesh)
					pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
			}
			break;
		}
		case TRANS_ROTATION:
		{
			if (m_pSelectedEffectTransform)
			{
				m_tMeshEffectDesc.vRotation = _float3(m_fX, m_fY, m_fZ);

				CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
				if (pEffectMesh)
					pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);

				m_pSelectedEffectTransform->Set_Rotation(_float3(m_fX, m_fY, m_fZ));
			}
			break;
		}
		case TRANS_TRANSLATION:
		{
			if (m_pSelectedEffectTransform)
			{
				m_tMeshEffectDesc.vPosition = _float3(m_fX, m_fY, m_fZ);

				CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
				if (pEffectMesh)
					pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);

				XMStoreFloat3(&vTranslation, m_pSelectedEffectTransform->Get_State(CTransform::STATE::STATE_TRANSLATION));
				_vector vNewTranslation = XMVectorSet(vTranslation.x, vTranslation.y, m_fZ, 1.f);

				m_pSelectedEffectTransform->Set_State(CTransform::STATE::STATE_TRANSLATION, vNewTranslation);
			}
			break;
		}
		}
	}
	ImGui::NewLine();

	ImGui::Text("Turn");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##TurnX", &m_tMeshEffectDesc.vTurn.x, 0.05f, 0, 1, "X: %.02f"))
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##TurnY", &m_tMeshEffectDesc.vTurn.y, 0.05f, 0, 1, "Y: %.02f"))
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##TurnZ", &m_tMeshEffectDesc.vTurn.z, 0.05f, 0, 1, "Z: %.02f"))
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
	}
	ImGui::SetNextItemWidth(140.f);
	if (ImGui::DragFloat("##TurnVelocity", &m_tMeshEffectDesc.fTurnVelocity, 0.05f, 0, 360, "Turn Velocity: %.02f"))
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
		{
			m_tMeshEffectDesc.fTurnVelocityInitial = m_tMeshEffectDesc.fTurnVelocity;
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
		}

	}
	ImGui::NewLine();

	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##MeshLifetime", &m_tMeshEffectDesc.fLifetime, 0.05f, 0, 0, "Lifetime: %.02f"))
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
	}
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##MeshStartAfter", &m_tMeshEffectDesc.fStartAfter, 0.05f, 0, 0, "Start After: %.02f"))
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
	}
	ImGui::NewLine();

	/* Pseudo Diffuse Map. */
	ImGui::Text("Texture");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200.f);
	if (ImGui::BeginCombo("##Texture", m_sSelectedMaskTexture.c_str()))
	{
		_uint iCounter = 0;
		for (auto iter = m_TextureNames.begin(); iter != m_TextureNames.end(); iter++)
		{
			if (ImGui::Selectable(iter->c_str(), iter->c_str() == m_sSelectedMaskTexture))
			{
				m_sSelectedMaskTexture = *iter;
				wstring wsMaskTexture = wstring(m_sSelectedMaskTexture.begin(), m_sSelectedMaskTexture.end());

				CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
				if (pEffectMesh)
				{
					wcscpy_s(m_tMeshEffectDesc.wcMaskTexture, MAX_PATH, wsMaskTexture.c_str());
					pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
					m_pSelectedEffect->Add_MaskTexture();
				}
			}

			if (iter->c_str() == m_sSelectedMaskTexture)
				ImGui::SetItemDefaultFocus();

			iCounter++;
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(120.f);
	if (ImGui::DragFloat("##MaskSpeed", &m_tMeshEffectDesc.fMaskSpeed, 0.05f, 0, 10, "Mask Speed: %.02f"))
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
	}
	ImGui::SetNextItemWidth(200.f);
	if (ImGui::DragFloat("##MaskDirectionX", &m_tMeshEffectDesc.fMaskDirectionX, 0.05f, -1, 1, "Mask Direction X: %.02f"))
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200.f);
	if (ImGui::DragFloat("##MaskDirectionY", &m_tMeshEffectDesc.fMaskDirectionY, 0.05f, -1, 1, "Mask Direction Y: %.02f"))
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
	}
	ImGui::NewLine();

	ImGui::Text("Noise Texture");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200.f);
	if (ImGui::BeginCombo("##NoiseTexture", m_sSelectedNoiseTexture.c_str()))
	{
		_uint iCounter = 0;
		for (auto iter = m_TextureNames.begin(); iter != m_TextureNames.end(); iter++)
		{
			if (ImGui::Selectable(iter->c_str(), iter->c_str() == m_sSelectedNoiseTexture))
			{
				m_sSelectedNoiseTexture = *iter;
				wstring wsNoiseTexture = wstring(m_sSelectedNoiseTexture.begin(), m_sSelectedNoiseTexture.end());

				CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
				if (pEffectMesh)
				{
					wcscpy_s(m_tMeshEffectDesc.wcNoiseTexture, MAX_PATH, wsNoiseTexture.c_str());
					pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
					m_pSelectedEffect->Add_NoiseTexture();
				}
			}

			if (iter->c_str() == m_sSelectedNoiseTexture)
				ImGui::SetItemDefaultFocus();

			iCounter++;
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(120.f);
	if (ImGui::DragFloat("##NoiseSpeed", &m_tMeshEffectDesc.fNoiseSpeed, 0.05f, 0, 10, "Noise Speed: %.02f"))
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(120.f);
	if (ImGui::DragFloat("##NoisePower", &m_tMeshEffectDesc.fNoisePower, 0.05f, 0, 100, "Noise Power: %.02f"))
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
		{
			m_tMeshEffectDesc.fNoisePowerInitial = m_tMeshEffectDesc.fNoisePower;
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
		}
	}
	ImGui::SetNextItemWidth(200.f);
	if (ImGui::DragFloat("##NoiseDirectionX", &m_tMeshEffectDesc.fNoiseDirectionX, 0.05f, -1, 1, "Noise Direction X: %.02f"))
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200.f);
	if (ImGui::DragFloat("##NoiseDirectionY", &m_tMeshEffectDesc.fNoiseDirectionY, 0.05f, -1, 1, "Noise Direction Y: %.02f"))
	{
		CEffectMesh* pEffectMesh = dynamic_cast<CEffectMesh*>(m_pSelectedEffect);
		if (pEffectMesh)
			pEffectMesh->Set_MeshEffectDesc(m_tMeshEffectDesc);
	}
}

void CImGui_Manager::Show_ParticleCustomization()
{
	ImGui::NewLine();
	ImGui::Separator();

	ImGui::Text("Particles Customization");
	ImGui::NewLine();

	/*ImGui::Text("Shader");
	ImGui::SameLine();
	if (ImGui::BeginCombo("##Shader", m_sCurrentShaderParticle.c_str()))
	{
		_uint iCounter = 0;
		for (auto& iter = m_ShadersParticle.begin(); iter != m_ShadersParticle.end(); iter++)
		{
			if (ImGui::Selectable(iter->c_str(), iter->c_str() == m_sCurrentShaderParticle))
			{
				m_sCurrentShaderParticle = *iter;
				m_pSelectedEffect->Set_ShaderId(iCounter);
			}

			if (iter->c_str() == m_sCurrentShaderParticle)
				ImGui::SetItemDefaultFocus();

			iCounter++;
		}
		ImGui::EndCombo();
	}
	ImGui::NewLine();*/

	ImGui::Text("Spawn Type");
	ImGui::SameLine();
	if (ImGui::BeginCombo("##SpawnType", m_sCurrentSpawnType.c_str()))
	{
		_uint iCounter = 0;
		for (auto iter = m_SpawnTypes.begin(); iter != m_SpawnTypes.end(); iter++)
		{
			if (ImGui::Selectable(iter->c_str(), iter->c_str() == m_sCurrentSpawnType))
			{
				m_sCurrentSpawnType = *iter;

				CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
				if (pParticleSystem)
				{
					m_tParticleDesc.m_eSpawnType = iCounter;
					pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
				}
			}

			if (iter->c_str() == m_sCurrentSpawnType)
				ImGui::SetItemDefaultFocus();

			iCounter++;
		}
		ImGui::EndCombo();
	}

	ImGui::Text("Color");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##ParticlesColorR", &m_tParticleDesc.m_vColor.x, 0.05f, 0, 1, "R: %.02f"))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##ParticlesColorG", &m_tParticleDesc.m_vColor.y, 0.05f, 0, 1, "G: %.02f"))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##ParticlesColorB", &m_tParticleDesc.m_vColor.z, 0.05f, 0, 1, "B: %.02f"))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##ParticleAlpha", &m_tParticleDesc.m_fAlpha, 0.05f, 0, 1, "A: %.02f"))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}
	ImGui::Text("Alpha Discard");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##ParticleAlphaDiscard", &m_tParticleDesc.m_fAlphaDiscard, 0.05f, 0, 1, "A: %.02f"))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}
	ImGui::NewLine();
	ImGui::Checkbox("Glow", &m_tParticleDesc.m_bGlow);
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##ParticlesGlowColorR", &m_tParticleDesc.vGlowColor.x, 0.05f, 0, 1, "R: %.02f"))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##ParticlesGlowColorG", &m_tParticleDesc.vGlowColor.y, 0.05f, 0, 1, "G: %.02f"))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	if (ImGui::DragFloat("##ParticlesGlowColorB", &m_tParticleDesc.vGlowColor.z, 0.05f, 0, 1, "B: %.02f"))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}
	ImGui::NewLine();
	if (ImGui::DragInt("##ParticlesNum", &m_tParticleDesc.m_iMaxParticles, 1, 0, 1000, "Max Particles: %d"))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}
	if (m_sCurrentSpawnType == "BURST")
		ImGui::BeginDisabled();
	if (ImGui::DragFloat("##ParticlesPerSecond", &m_tParticleDesc.m_fParticleLifeTime, 0.05f, 0, m_tParticleDesc.m_iMaxParticles, "Particles/Sec: %.02f"))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}
	if (m_sCurrentSpawnType == "BURST")
		ImGui::EndDisabled();
	if (ImGui::DragFloat("##ParticlesLifetime", &m_tParticleDesc.m_fParticleLifeTime, 0.05f, 0, 0, "Particles Lifetime: %.02f"))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}
	if (ImGui::DragFloat("##ParticlesStartAfter", &m_tParticleDesc.m_fParticleStartAfter, 0.05f, 0, 0, "Particles Start After: %.02f"))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}
	if (ImGui::DragFloat("##ParticleDeviationX", &m_tParticleDesc.m_fParticleDeviationX, 0.05f, 0, 0, "X Variation: %.02f"))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}
	if (ImGui::DragFloat("##ParticleDeviationY", &m_tParticleDesc.m_fParticleDeviationY, 0.05f, 0, 0, "Y Variation: %.02f"))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}
	if (ImGui::DragFloat("##ParticleDeviationZ", &m_tParticleDesc.m_fParticleDeviationZ, 0.05f, 0, 0, "Z Variation: %.02f"))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}
	if (m_tParticleDesc.m_bRandomDirectionX)
		ImGui::BeginDisabled();
	if (ImGui::DragFloat("##ParticleDirectionX", &m_tParticleDesc.m_vParticleDirection.x, 0.05f, -1, 1, "Direction X: %.02f"))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}
	if (m_tParticleDesc.m_bRandomDirectionX)
		ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::Text("Random");
	ImGui::SameLine();
	if (ImGui::Checkbox("##RandomDirectionX", &m_tParticleDesc.m_bRandomDirectionX))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}

	if (m_tParticleDesc.m_bRandomDirectionY)
		ImGui::BeginDisabled();
	if (ImGui::DragFloat("##ParticleDirectionY", &m_tParticleDesc.m_vParticleDirection.y, 0.05f, -1, 1, "Direction Y: %.02f"))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}
	if (m_tParticleDesc.m_bRandomDirectionY)
		ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::Text("Random");
	ImGui::SameLine();
	if (ImGui::Checkbox("##RandomDirectionY", &m_tParticleDesc.m_bRandomDirectionY))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}

	if (m_tParticleDesc.m_bRandomDirectionZ)
		ImGui::BeginDisabled();
	if (ImGui::DragFloat("##ParticleDirectionZ", &m_tParticleDesc.m_vParticleDirection.z, 0.05f, -1, 1, "Direction Z: %.02f"))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}
	if (m_tParticleDesc.m_bRandomDirectionZ)
		ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::Text("Random");
	ImGui::SameLine();
	if (ImGui::Checkbox("##RandomDirectionZ", &m_tParticleDesc.m_bRandomDirectionZ))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}

	if (ImGui::DragFloat("##Velocity", &m_tParticleDesc.m_fParticleVelocity, 0.05f, 0, 100.f, "Velocity: %.02f"))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}
	if (ImGui::DragFloat("##VelocityVariation", &m_tParticleDesc.m_fParticleVelocityVariation, 0.05f, 0, 100.f, "Velocity Variation: %.02f"))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}
	if (ImGui::DragFloat("##Size", &m_tParticleDesc.m_fParticleSize, 0.05f, .1f, 100.f, "Size: %.02f"))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}
	if (ImGui::DragFloat("##SizeVariation", &m_tParticleDesc.m_fParticleSizeVariation, 0.05f, 0, 100.f, "Size Variation: %.02f"))
	{
		CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(m_pSelectedEffect);
		if (pParticleSystem)
			pParticleSystem->Set_ParticleDesc(m_tParticleDesc);
	}
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
	Safe_Release(m_pNavigation_Manager);
	Safe_Release(m_pTerrianManager);
	Safe_Release(m_pModel_Manager);
	Safe_Release(m_pEffectManager);

	CModelManager::GetInstance()->DestroyInstance();
	CNavigation_Manager::GetInstance()->DestroyInstance();
	CEffectManager::GetInstance()->DestroyInstance();
	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);
}
