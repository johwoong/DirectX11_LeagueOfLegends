#pragma once

#include <windows.h>
#include <process.h>

extern HWND g_hWnd;
extern HINSTANCE g_hInst;

namespace Client
{
	static const unsigned int g_iWinSizeX = 1280;
	static const unsigned int g_iWinSizeY = 720;

	enum class LEVEL { STATIC, LOADING, LOGO, GAMEPLAY, END };
}

using namespace std;
using namespace Client;































/* 1. �˽ѳ�(��). */
/* 1. �����ǽİ�.(��) */
/* 1. ��õ����.(��) */
/* 1. ���̻�, ���̻� */
/* 1. �߰׸�����2��.(��) */
/* 1. ���ѻڶ���.(��) */
/* 1. ���չ��� ���̵�(����) */
/* 1. ����Ŀ(��) */
/* 1. �ڽ�����.(��, ��) */
/* 1. ���Ʈ��. */
/* 1. ������Ӿ�ī���� */
/* 1. ����� ���� */