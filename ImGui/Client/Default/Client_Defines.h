#pragma once
#pragma warning (disable : 4996)
namespace Client
{
	const unsigned int g_WinSizeX = 1280;
	const unsigned int g_WinSizeY = 720;

	enum LEVEL { LEVEL_STATIC, LEVEL_LOADING, LEVEL_LOGO, LEVEL_GAMEPLAY, LEVEL_END };
	enum OBJID {OBJ_BACKGROUND, OBJ_BLOCK, OBJ_END};
	enum SHADER_ID {SHADER_DEFAULT, SHADER_WIREFRAME, SHADER_PICKED, SHADER_SYMBOL};
	enum MODEL_ID {MODEL_NEXUS, MODEL_EZREAL, MODEL_END};
}

#include <process.h>

#include "Windows.h"
extern HWND g_hWnd;
extern HINSTANCE g_hInst;

using namespace Client;
using namespace std;

