#include "pch.h"

#include "Camera_Dynamic.h"
#include "GameInstance.h"
#include "Camera_Manager.h"
#include "Imgui_Manager.h"
#include "Actor.h"

CCamera_Dynamic::CCamera_Dynamic(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CCamera(pDevice, pContext)
{
}

CCamera_Dynamic::CCamera_Dynamic(const CCamera_Dynamic& rhs)
	: CCamera(rhs)
{
}

HRESULT CCamera_Dynamic::Initialize_Prototype()
{
	if (FAILED(__super::Initialize_Prototype()))
		return E_FAIL;

	return S_OK;
}

HRESULT CCamera_Dynamic::Initialize(void* pArg)
{


	if (FAILED(__super::Initialize(&((CAMERADESC_DERIVED*)pArg)->CameraDesc)))
		return E_FAIL;


	return S_OK;
}

int CCamera_Dynamic::Update(_float fTimeDelta)
{

	__super::Update(fTimeDelta);

	switch (m_eCamMode)
	{
	case CAM_DEBUG:
		DebugVCamera(fTimeDelta);
		break;
	case CAM_TARGET:
		PlayCamera(fTimeDelta);
		break;
	default:
		break;
	}


	if (FAILED(Bind_OnPipeLine()))
		return OBJ_NOEVENT;

	return OBJ_NOEVENT;
}

void CCamera_Dynamic::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);

}

HRESULT CCamera_Dynamic::Render()
{
	if (FAILED(__super::Render()))
		return E_FAIL;


	return S_OK;
}

int CCamera_Dynamic::PlayCamera(_float fTimeDelta)
{
	return OBJ_NOEVENT;
}

int CCamera_Dynamic::DebugVCamera(_float fTimeDelta)
{
	/* Debug Camera*/
	CGameInstance* pGameInstance = CGameInstance::GetInstance();

	if (pGameInstance->Key_Pressing(DIK_LSHIFT))
	{
		/* Camera Inputs */
		if (pGameInstance->Key_Pressing(DIK_W))
			m_pTransformCom->Move_Straight(fTimeDelta * 2);
		if (pGameInstance->Key_Pressing(DIK_S))
			m_pTransformCom->Move_BackWard(fTimeDelta * 2);
		if (pGameInstance->Key_Pressing(DIK_A))
			m_pTransformCom->Move_Left(fTimeDelta * 2);
		if (pGameInstance->Key_Pressing(DIK_D))
			m_pTransformCom->Move_Right(fTimeDelta * 2);

		_long MouseMove = 0;
		if (MouseMove = pGameInstance->Get_DIMMoveState(DIMM_X))
			m_pTransformCom->Turn(XMVectorSet(0.f, 1.f, 0.f, 0.f), fTimeDelta * MouseMove * 0.1f);
		if (MouseMove = pGameInstance->Get_DIMMoveState(DIMM_Y))
			m_pTransformCom->Turn(m_pTransformCom->Get_State(CTransform::STATE_RIGHT), fTimeDelta * MouseMove * 0.1f);
	}

	return 0;
}

void CCamera_Dynamic::Set_Play(_bool type)
{
	m_bPlay = type;

	if (m_bPlay)
	{
		m_vInitPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);

		_vector vCameraLook = XMVector3Normalize(m_pTransformCom->Get_State(CTransform::STATE_LOOK));
		_vector vCameraCurrentPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
		_vector vCurrentAt = vCameraCurrentPos + vCameraLook;

		m_vInitAt = vCurrentAt;
	}

}



CCamera_Dynamic* CCamera_Dynamic::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CCamera_Dynamic* pInstance = new CCamera_Dynamic(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CCamera_Dynamic");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CCamera_Dynamic::Clone(void* pArg)
{
	CCamera_Dynamic* pInstance = new CCamera_Dynamic(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CCamera_Dynamic");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CCamera_Dynamic::Free()
{
	__super::Free();


}
