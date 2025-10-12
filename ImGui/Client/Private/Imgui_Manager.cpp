#include "pch.h"
#include "Imgui_Manager.h"
#include <windows.h>
#ifdef new
#undef new
#endif

#include <shlobj.h> 

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "GameInstance.h"

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

string CutOnSpecificDecimalPt(string num, int pos)
{
    return num.substr(0, num.find('.') + pos + 1);
}

CImGui_Manager::CImGui_Manager()
{
}

HRESULT CImGui_Manager::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    if (m_pDevice != nullptr) // 이미 초기화됨 방지
        return S_OK;

    m_pDevice = pDevice;
    m_pContext = pContext;
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);

    // === IMGUI 초기화 ===
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // Win32 + DX11 바인딩
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

    // DrawData 준비까지만
    ImGui::Render();
}

void CImGui_Manager::Update_Imgui()
{
    ImGui::Begin("Model Tool"); // 창 제목
    ImGui::Text("IMGUI Initialized Successfully!");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Separator();
    ImGui::Text("ToolSuccessFul");
    ImGui::End();
}

void CImGui_Manager::Render()
{
    ImDrawData* pData = ImGui::GetDrawData();
    if (!pData)
        return;

    ImGui_ImplDX11_RenderDrawData(pData);
}

void CImGui_Manager::ShowSimpleMousePos(bool* p_open)
{
}

void CImGui_Manager::ShowPickedObj()
{
}

void CImGui_Manager::Show_PopupBox()
{
}

void CImGui_Manager::Set_Object_Map()
{
}

void CImGui_Manager::Set_LayerTag()
{
}

void CImGui_Manager::Set_Macro()
{
}

void CImGui_Manager::Show_ModelList()
{
}

void CImGui_Manager::Show_CurrentModelList()
{
}

void CImGui_Manager::BrowseForFolder()
{
}

void CImGui_Manager::Set_TrasureBox()
{
}

void CImGui_Manager::Save_TrasureBox()
{
}

void CImGui_Manager::Load_TrasureBox()
{
}

void CImGui_Manager::Create_Model(const _tchar* pPrototypeTag, const _tchar* pLayerTag, _bool bCreatePrototype)
{
}

void CImGui_Manager::Free()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    Safe_Release(m_pDevice);
    Safe_Release(m_pContext);
}
