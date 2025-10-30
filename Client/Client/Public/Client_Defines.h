#pragma once
#pragma warning (disable : 4996)
namespace Client
{
	const unsigned int g_WinSizeX = 1920;
	const unsigned int g_WinSizeY = 1080;

	enum LEVEL { LEVEL_STATIC, LEVEL_LOADING, LEVEL_LOGO, LEVEL_GAMEPLAY, LEVEL_END };

	// SHADER 
	enum SHADER_NONANIMID {SHADER_NOANIM_DEFAULT};

	
}

#include <process.h>
#include <time.h>
#include "Windows.h"
extern HWND g_hWnd;
extern HINSTANCE g_hInst;

using namespace Client;
using namespace std;




















