#pragma once
#include "Base.h"
#include "Client_Defines.h"
class CCursorManager :  public CBase
{
    DECLARE_SINGLETON(CCursorManager)

private:
    CCursorManager() = default;
    virtual ~CCursorManager() = default;

public:
    HRESULT Initialize(HWND hWnd)
    {
        m_hWnd = hWnd;
        m_eState = CURSOR_DEFAULT;
        m_tMousePos = { 0,0 };
        return S_OK;
    }

    // 매 프레임 호출
    void Update()
    {
        ::GetCursorPos(&m_tMousePos);
        ::ScreenToClient(m_hWnd, &m_tMousePos);
    }

    void   SetState(CURSOR_TYPE eState) { m_eState = eState; }
    CURSOR_TYPE GetState() const { return m_eState; }

    POINT GetCursorPosClient() const { return m_tMousePos; }

private:
    HWND        m_hWnd = nullptr;
    CURSOR_TYPE m_eState = CURSOR_DEFAULT;
    POINT       m_tMousePos{};
public:
    virtual void Free() override {}
};

