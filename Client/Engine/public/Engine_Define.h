#ifndef Engine_Define_h__
#define Engine_Define_h__
#define NOMINMAX   // Windows 헤더의 min/max 매크로 비활성화
#include <d3d11.h>
#include <DirectXMath.h>
#include <Effects11/d3dx11effect.h>

#include <DirectXTK/DDSTextureLoader.h>
#include <DirectXTK/WICTextureLoader.h>


#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <d3dcompiler.h>

using namespace DirectX;
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <functional>
#include <string>
using namespace std;

#include <unordered_map>
#include <ctime>

#include "Engine_Enum.h"
#include "Engine_Macro.h"
#include "Engine_Struct.h"
#include "Engine_Typedef.h"
#include "Engine_Function.h"


#define DIRECTINPUT_VERSION	0x0800
#include <dinput.h>

#pragma warning(disable : 4251)

#ifdef _DEBUG

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#ifndef DBG_NEW 

#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ ) 
#define new DBG_NEW 

#endif
#endif


using namespace Engine;

#endif // Engine_Define_h__
