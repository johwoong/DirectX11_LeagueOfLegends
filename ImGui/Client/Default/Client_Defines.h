#pragma once

namespace Client
{
	const unsigned int g_WinSizeX = 1280;
	const unsigned int g_WinSizeY = 720;

	enum LEVEL { LEVEL_STATIC, LEVEL_LOADING, LEVEL_LOGO, LEVEL_GAMEPLAY, LEVEL_END };
}

#include <process.h>

extern HWND g_hWnd;
extern HINSTANCE g_hInst;

using namespace Client;

