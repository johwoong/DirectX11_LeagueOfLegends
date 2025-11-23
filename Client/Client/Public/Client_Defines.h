#pragma once
#pragma warning (disable : 4996)
namespace Client
{
	const unsigned int g_WinSizeX = 1920;
	const unsigned int g_WinSizeY = 1080;
	enum DIRID { DIR_STRAIGHT, DIR_BACKWARD, DIR_LEFT, DIR_RIGHT, DIR_STRAIGHT_LEFT, DIR_STRAIGHT_RIGHT, DIR_BACKWARD_LEFT, DIR_BACKWARD_RIGHT, DIR_END };
	enum LEVEL { LEVEL_STATIC, LEVEL_LOADING, LEVEL_LOGO, LEVEL_GAMEPLAY, LEVEL_END };

	// SHADER 
	enum SHADER_NONANIMID {SHADER_NOANIM_DEFAULT};
	enum SHADER_ANIMID {SHADER_ANIMDEFAULT,};
	enum OBJECT_ID {PLAYER, ENEMY, NPC, TEAM, OBJECT_END};

	enum STATETYPE
	{
		STATETYPE_START,
		STATETYPE_MAIN,
		STATETYPE_END,
		STATETYPE_DEFAULT
	};

	enum TEAMTYPE
	{
		TEAM_BLUE,
		TEAM_RED,
	};

	enum LANE_TYPE
	{
		LANE_TOP,
		LANE_MID,
		LANE_BOT,
		LANE_END
	};

	enum MINION_TYPE
	{
		MINION_MELEE,
		MINION_CASTER,
		MINION_SIEGE,   // ´ëÆ÷
		MINION_SUPER,
	};
}

#include <process.h>
#include <time.h>
#include "Windows.h"
extern HWND g_hWnd;
extern HINSTANCE g_hInst;

using namespace Client;
using namespace std;




















