#pragma once

#include "Base.h"

NS_BEGIN(Engine)

/* DX9 */
/* 1. 내 장치의 성능을 조사하는기능 */
/* 2. 지원해주는 범위내에서 내가 셋팅하고싶은 데이터를 전달해서 초기화할 준비를 할 수 있도록 */
/* 3. 디바이스 객체를 생성해준다.(Idirect3DDevice9(장치객체 + 백버퍼+ 깊이버퍼) 객체를 만들었다.) */
/* 3_1. Idirect3DDevice9객체는 상당히 많은 기능을 가진다*/

/* 1. ID3D11Device, ID3D11DeviceContext라는 객체를 우선 생성한다. */
/* 2. IDXGISwapChain를 생성한다.(백버퍼(ID3D11Texture2D)도 같이 생성한거야.) */
/* 3. 백버퍼뷰를 생성한다. */
/* 4. 깊이버퍼 텍스쳐를 생성하고 깊이버퍼 뷰를 생성한다. */

class CGraphic_Device final : public CBase
{		
	DECLARE_SINGLETON(CGraphic_Device)
public:
	CGraphic_Device();
	virtual ~CGraphic_Device() = default;

public:
	HRESULT Ready_Graphic_Device(HWND hWnd, GRAPHIC_DESC::WINMODE WinMode, _uint iWinCX, _uint iWinCY, ID3D11Device** ppDeviceOut, ID3D11DeviceContext** ppDeviceContextOut);
	HRESULT Clear_BackBuffer_View(_float4 vClearColor);
	HRESULT Clear_DepthStencil_View();
	HRESULT Present();
private:

	/* 메모리 할당. (정점버퍼, 인덱스버퍼, 텍스쳐로드) */
	ID3D11Device* m_pDevice = nullptr;

	/* 바인딩작업. 기능실행 (정점버퍼를 SetStreamSource(), SetIndices(), SetTransform(), SetTexture() */
	ID3D11DeviceContext* m_pDeviceContext = nullptr;

	IDXGISwapChain* m_pSwapChain = nullptr;

	// ID3D11ShaderResourceView*
	ID3D11RenderTargetView* m_pBackBufferRTV = nullptr;
	ID3D11DepthStencilView* m_pDepthStencilView = nullptr;

private:
	HRESULT Ready_SwapChain(HWND hWnd, GRAPHIC_DESC::WINMODE WinMode, _uint iWinCX, _uint iWinCY);
	HRESULT Ready_BackBufferRenderTargetView();
	HRESULT Ready_DepthStencilRenderTargetView(_uint iWinCX, _uint iWinCY);

public:
	virtual void Free() override;
};

NS_END