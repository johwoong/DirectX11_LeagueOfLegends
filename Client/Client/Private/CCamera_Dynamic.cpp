#include "CCamera_Dynamic.h"
#include "GameInstance.h"
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
	return S_OK;
}

HRESULT CCamera_Dynamic::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(&((CAMERADESC_DERIVED*)pArg)->CameraDesc)))
		return E_FAIL;

	// 초기 카메라 위치 설정
	m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION,
		XMLoadFloat4(&((CAMERADESC_DERIVED*)pArg)->InitPostion));

	return S_OK;
}

int CCamera_Dynamic::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);

	if (CGameInstance::GetInstance()->Key_Up(DIK_F1))
		m_eCamMode = (m_eCamMode == CAM_GAME) ? CAM_DEBUG : CAM_GAME;

	switch (m_eCamMode)
	{
	case CAM_GAME:	Game_Camera(fTimeDelta);	break;
	case CAM_DEBUG:	Debug_Camera(fTimeDelta);	break;
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
	return S_OK;
}

void CCamera_Dynamic::Game_Camera(_float fTimeDelta)
{
	if (!m_pTarget)
		return;

	_vector vLook = XMVector3Normalize(XMVectorSet(0.057f, -0.864f, -0.500f, 0.f));
	_vector vUp = XMVector3Normalize(XMVectorSet(-0.064f, 0.497f, -0.865f, 0.f));
	_vector vRight = XMVector3Normalize(XMVectorSet(-0.996f, -0.081f, 0.027f, 0.f));

	static float fZoom = 0.f;
	const float baseDist = 14.f;

	long wheel = m_pGameInstance->Get_DIMMoveState(DIMM_WHEEL);
	if (wheel != 0)
	{
		fZoom -= wheel * 0.3f;
		fZoom = max(-8.f, min(0.f, fZoom));
	}

	_vector vPlayerPos = m_pTarget->Get_TransformState(CTransform::STATE_TRANSLATION);
	_vector vCamPos =
		vPlayerPos
		- vLook * (baseDist + fZoom)
		+ vUp * 1.4f
		+ vRight * 0.f;     
	_vector vCurCam = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
	_vector vNewCam = XMVectorLerp(vCurCam, vCamPos, fTimeDelta * 5.f);


	_matrix matView = XMMatrixLookToLH(vNewCam, vLook, vUp);
	_matrix matWorld = XMMatrixInverse(nullptr, matView);

	m_pTransformCom->Set_State(CTransform::STATE_RIGHT, vRight);
	m_pTransformCom->Set_State(CTransform::STATE_UP, vUp);
	m_pTransformCom->Set_State(CTransform::STATE_LOOK, vLook);
	m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vNewCam);
	m_pTransformCom->Set_WorldMatrix(matWorld);

	float FOV = XMConvertToRadians(30.f);
	_matrix matProj = XMMatrixPerspectiveFovLH(
		FOV,
		m_CameraDesc.fAspect,
		m_CameraDesc.fNear,
		m_CameraDesc.fFar
	);

}

void CCamera_Dynamic::Debug_Camera(_float fTimeDelta)
{
	if (m_pGameInstance->Key_Pressing(DIK_LSHIFT))
	{
		if (m_pGameInstance->Key_Pressing(DIK_W))
			m_pTransformCom->Move_Straight(fTimeDelta * 2);
		if (m_pGameInstance->Key_Pressing(DIK_S))
			m_pTransformCom->Move_BackWard(fTimeDelta * 2);
		if (m_pGameInstance->Key_Pressing(DIK_A))
			m_pTransformCom->Move_Left(fTimeDelta * 2);
		if (m_pGameInstance->Key_Pressing(DIK_D))
			m_pTransformCom->Move_Right(fTimeDelta * 2);

		_long MouseMove = 0;
		if (MouseMove = m_pGameInstance->Get_DIMMoveState(DIMM_X))
			m_pTransformCom->Turn(XMVectorSet(0.f, 1.f, 0.f, 0.f), fTimeDelta * MouseMove * 0.1f);
		if (MouseMove = m_pGameInstance->Get_DIMMoveState(DIMM_Y))
			m_pTransformCom->Turn(m_pTransformCom->Get_State(CTransform::STATE_RIGHT), fTimeDelta * MouseMove * 0.1f);


		_vector vPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
		_vector vLook = m_pTransformCom->Get_State(CTransform::STATE_LOOK);
		_vector vRight = m_pTransformCom->Get_State(CTransform::STATE_RIGHT);
		_vector vUp = m_pTransformCom->Get_State(CTransform::STATE_UP);

		// 월드 좌표 출력
		_float3 vPos3;
		XMStoreFloat3(&vPos3, vPos);

		// 회전(혹은 방향벡터) 출력
		_float3 vLook3, vUp3, vRight3;
		XMStoreFloat3(&vLook3, XMVector3Normalize(vLook));
		XMStoreFloat3(&vUp3, XMVector3Normalize(vUp));
		XMStoreFloat3(&vRight3, XMVector3Normalize(vRight));

		// 콘솔에 출력 (디버그 창에서 확인)
		printf("Cam Pos : X=%.3f, Y=%.3f, Z=%.3f\n", vPos3.x, vPos3.y, vPos3.z);
		printf("Look    : X=%.3f, Y=%.3f, Z=%.3f\n", vLook3.x, vLook3.y, vLook3.z);
		printf("Up      : X=%.3f, Y=%.3f, Z=%.3f\n", vUp3.x, vUp3.y, vUp3.z);
		printf("Right   : X=%.3f, Y=%.3f, Z=%.3f\n", vRight3.x, vRight3.y, vRight3.z);
	}
}

void CCamera_Dynamic::Set_CamMode(CAM_MODE eCamMode)
{
	m_eCamMode = eCamMode;
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
	Safe_Release(m_pTarget);
	__super::Free();
}




