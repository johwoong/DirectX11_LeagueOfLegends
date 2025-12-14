#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#ifndef Engine_Define_h__
#define Engine_Define_h__

#pragma warning (disable : 4996)
#pragma warning (disable : 4005)
#pragma warning (disable : 4251)
#pragma warning (disable : 4819)
#pragma warning(disable:4099)

#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <Effects11/d3dx11effect.h> 
#include <DirectXCollision.h>

#include <DirectXTK/DDSTextureLoader.h>
#include <DirectXTK/WICTextureLoader.h>

#include <DirectXTK/VertexTypes.h>
#include <DirectXTK/PrimitiveBatch.h>
#include <DirectXTK/Effects.h>
#include <directXTK/SpriteFont.h>
#include <directXTK/SpriteBatch.h>


// ===== Assimp =====
#include <assimp/types.h>
#include <assimp/scene.h>
#include <assimp/material.h>   // ★ aiTextureType 등 선언 여기 있음
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>


#pragma comment(lib, "d3dcompiler.lib")


using namespace DirectX;
#include <vector>
#include <list>
#include <queue>
#include <map>
#include <array>
#include <algorithm>
#include <functional>
#include <string>
#include <iostream>
#include <typeinfo>
#include <sstream>
using namespace std;

#include <unordered_map>
#include <ctime>

#include "Engine_Enum.h"
#include "Engine_Macro.h"
#include "Engine_Struct.h"
#include "Engine_Typedef.h"
#include "Engine_Function.h"


#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

enum DIMK { DIMK_LBUTTON, DIMK_RBUTTON, DIMK_WHEEL, DIMK_END };
enum DIMM { DIMM_X, DIMM_Y, DIMM_WHEEL, DIMM_END };


#ifdef _DEBUG

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#ifndef DBG_NEW 

#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ ) 
#define new DBG_NEW 

#endif

#endif

#define OBJ_POOL 2
#define OBJ_DEAD 1
#define OBJ_NOEVENT 0
#define OBJ_NOSHOW 3


using namespace Engine;

#endif // Engine_Define_h__
