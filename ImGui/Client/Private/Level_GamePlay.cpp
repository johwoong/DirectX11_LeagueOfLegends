#include "pch.h"
#include "Level_GamePlay.h"

#include "GameInstance.h"
#include "Camera_Dynamic.h"
#include "Imgui_Manager.h"

CLevel_GamePlay::CLevel_GamePlay(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CLevel(pDevice, pContext)
{
}

HRESULT CLevel_GamePlay::Initialize()
{
	if (FAILED(__super::Initialize()))
		return E_FAIL;

	if (FAILED(Ready_Lights()))
		return E_FAIL;

	if (FAILED(Ready_Layer_Camera(TEXT("Layer_Camera"))))
		return E_FAIL;

	if (FAILED(Ready_Layer_BackGround(TEXT("Layer_BackGround"))))
		return E_FAIL;

	
	return S_OK;
}

void CLevel_GamePlay::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);

	CImGui_Manager::GetInstance()->Update(fTimeDelta);

}

void CLevel_GamePlay::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);
	CImGui_Manager::GetInstance()->Render();
	SetWindowText(g_hWnd, TEXT("게임플레이레벨입니다/"));
}

HRESULT CLevel_GamePlay::Ready_Lights()
{
	return S_OK;
}



HRESULT CLevel_GamePlay::Ready_Layer_BackGround(const _tchar* pLayerTag)
{
	CGameInstance* pGameInstance = CGameInstance::GetInstance();
	Safe_AddRef(pGameInstance);

	NONANIMDESC desc{};
	strcpy_s(desc.pModeltag, MAX_PATH, "Prototype_Component_Model_Map01");

	desc.vPosition = _float3(0.f, 0.f, 0.f);
	desc.vScale = _float3(0.01f, 0.01f, 0.01f);
	desc.vPosition = _float3(0.f, 0.f, 0.f);
	desc.m_fAngle = 0.f;
	XMStoreFloat4x4(&desc.WorldMatrix, XMMatrixIdentity());

	if (FAILED(pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_NonAnim"), LEVEL_GAMEPLAY, pLayerTag, &desc)))
		return E_FAIL;


	Safe_Release(pGameInstance);
	return S_OK;
}

HRESULT CLevel_GamePlay::Ready_Layer_Camera(const _tchar* pLayerTag)
{
	CGameInstance* pGameInstance = CGameInstance::GetInstance();
	Safe_AddRef(pGameInstance);

	CCamera_Dynamic::CAMERADESC_DERIVED				CameraDesc;
	ZeroMemory(&CameraDesc, sizeof(CCamera_Dynamic::CAMERADESC_DERIVED));


	CameraDesc.CameraDesc.vEye = _float4(0.f, 10.0f, -10.f, 1.f);
	CameraDesc.CameraDesc.vAt = _float4(0.f, 0.f, 0.f, 1.f);

	CameraDesc.CameraDesc.fFov = XMConvertToRadians(60.0f);
	CameraDesc.CameraDesc.fAspect= (_float)g_WinSizeX / g_WinSizeY;
	CameraDesc.CameraDesc.fNear = 0.2f;
	CameraDesc.CameraDesc.fFar = 60000.f;

	CameraDesc.CameraDesc.TransformDesc.fSpeedPerSec = 30.f;
	CameraDesc.CameraDesc.TransformDesc.fRotationPerSec = XMConvertToRadians(90.0f);

	if (FAILED(pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_Camera_Dynamic"), LEVEL_GAMEPLAY, pLayerTag, &CameraDesc)))
		return E_FAIL;

	Safe_Release(pGameInstance);
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
