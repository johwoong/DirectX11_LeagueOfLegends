
#include "Level_GamePlay.h"

CLevel_GamePlay::CLevel_GamePlay(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CLevel { pDevice, pContext } 
{
}

HRESULT CLevel_GamePlay::Initialize()
{
	if(FAILED(Ready_Layer_BackGround(TEXT("Layer_BackGround"))))
		return E_FAIL;

    return S_OK;
}

void CLevel_GamePlay::Update(_float fTimeDelta)
{
}

void CLevel_GamePlay::Late_Update(_float fTimeDelta)
{
	SetWindowText(g_hWnd, TEXT("¼ÒÈ¯»çÀÇ Çù°î"));
}

HRESULT CLevel_GamePlay::Ready_Layer_BackGround(const _tchar* pLayerTag)
{
	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Player(const _tchar* pLayerTag)
{
	return S_OK;
}



CLevel_GamePlay* CLevel_GamePlay::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CLevel_GamePlay* pInstance = new CLevel_GamePlay(pDevice, pContext);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevel_GamePlay");
		Safe_Release(pInstance);
	}
	return pInstance;
}


void CLevel_GamePlay::Free()
{
    __super::Free();

}
