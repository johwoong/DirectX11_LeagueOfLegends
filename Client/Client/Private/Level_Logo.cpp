#include "Level_Logo.h"
#include "GameInstance.h"

#include "Level_Loading.h"



CLevel_Logo::CLevel_Logo(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CLevel { pDevice, pContext } 
{
}

HRESULT CLevel_Logo::Initialize()
{
	if (FAILED(Ready_Layer_BackGround(TEXT("Layer_BackGround"))))
		return E_FAIL;


    return S_OK;
}

void CLevel_Logo::Update(_float fTimeDelta)
{
	if (GetKeyState(VK_SPACE) & 0x8000)
	{
		if (FAILED(m_pGameInstance->Open_Level(LEVEL_LOADING, CLevel_Loading::Create(m_pDevice, m_pContext, LEVEL_GAMEPLAY))))
			return;
	}

}

void CLevel_Logo::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);
	
	SetWindowText(g_hWnd, TEXT("로고 레벨"));
}

HRESULT CLevel_Logo::Ready_Layer_BackGround(const _tchar* pLayerTag)
{
	if (FAILED(m_pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_UI_BackGround"), LEVEL_LOGO, pLayerTag, nullptr)))
		return E_FAIL;


	return S_OK;
}

CLevel_Logo* CLevel_Logo::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CLevel_Logo* pInstance = new CLevel_Logo(pDevice, pContext);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevel_Logo");
		Safe_Release(pInstance);
	}
	return pInstance;
}


void CLevel_Logo::Free()
{
    __super::Free();
}
