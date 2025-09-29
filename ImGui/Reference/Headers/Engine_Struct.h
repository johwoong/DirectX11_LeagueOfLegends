#ifndef Engine_Struct_h__
#define Engine_Struct_h__

namespace Engine
{
	typedef struct tagEngineDesc
	{
		HWND			hWnd;
		unsigned int	iWinSizeX, iWinSizeY;
		WINMODE			eWindowMode;
	}ENGINE_DESC;
	
}


#endif // Engine_Struct_h__
