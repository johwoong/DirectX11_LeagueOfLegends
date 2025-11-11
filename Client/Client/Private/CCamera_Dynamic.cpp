#include "CCamera_Dynamic.h"
#include "GameInstance.h"
#include "Player.h"


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

	m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, XMLoadFloat4(&((CAMERADESC_DERIVED*)pArg)->InitPostion));
	m_OriginPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);

	return S_OK;
}

int CCamera_Dynamic::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);
	
	_float fTime = CGameInstance::GetInstance()->Get_TimeDelta(TEXT("Timer_Camera"));

	__super::Update(fTime);

	if (CGameInstance::GetInstance()->Key_Up(DIK_F1))
	{
		if (m_eCamMode != CAM_DEBUG)
		{
			m_ePreCamMode = m_eCamMode;
			m_eCamMode = CAM_DEBUG;
		}
		else
		{
			m_eCamMode = m_ePreCamMode;
			m_ePreCamMode = CAM_DEBUG;
		}
	}

	switch (m_eCamMode)
	{
	case Client::CCamera_Dynamic::CAM_GAME:
		Game_Camera(fTime);
		break;
	case Client::CCamera_Dynamic::CAM_DEBUG:
		Debug_Camera(fTime);
		break;
	}

	if (FAILED(Bind_OnPipeLine()))
		return OBJ_NOEVENT;
}

void CCamera_Dynamic::Late_Update(_float fTimeDelta)
{
	_float fTime = CGameInstance::GetInstance()->Get_TimeDelta(TEXT("Timer_Camera"));


	__super::Late_Update(fTime);
}

HRESULT CCamera_Dynamic::Render()
{
	if (FAILED(__super::Render()))
		return E_FAIL;


	return S_OK;
}

void CCamera_Dynamic::Set_CamMode(CAM_MODE eCamMode)
{
	if (m_eCamMode == eCamMode)
		return;

	if (m_ePreCamMode != m_eCamMode)
		m_ePreCamMode = m_eCamMode;

	// 플레이어와 카메라의 위치 계산
	m_eCamMode = eCamMode;
	m_vInitPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
}

void CCamera_Dynamic::Debug_Camera(_float fTimeDelta)
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
}

void CCamera_Dynamic::Game_Camera(_float fTimeDelta)
{
	if (!m_pTarget)
		return;

	CTransform* pTargetTransform = m_pTarget->Get_Transform();
	_vector vTargetPos = pTargetTransform->Get_State(CTransform::STATE_TRANSLATION);


	_vector vOffset = XMVectorSet(0.f, 28.f, -32.f, 0.f);

	_vector vDesiredPos = vTargetPos + vOffset;

	_vector vCurPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
	_vector vNewPos = XMVectorLerp(vCurPos, vDesiredPos, fTimeDelta * 5.f);
	m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vNewPos);

	_vector vAt = vTargetPos + XMVectorSet(0.f, 5.f, 5.f, 0.f);
	_vector vEye = vNewPos;
	_vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	_matrix matView = XMMatrixLookAtLH(vEye, vAt, vUp);
	m_pGameInstance->Set_Transform(CPipeLine::D3DTS_VIEW, matView);

	m_pGameInstance->Set_Transform(CPipeLine::D3DTS_PROJ,
		XMMatrixPerspectiveFovLH(XMConvertToRadians(38.f),
			(_float)g_WinSizeX / g_WinSizeY, 0.1f, 1000.f));
}

void CCamera_Dynamic::Set_Position(_vector vPosition)
{
	m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vPosition);
	m_OriginPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
}

void CCamera_Dynamic::Set_Zoom(_bool type, _float fZoomDistance, _float fZoomSpeed)
{
	m_bZoom = type;
	
	if (m_bZoom)
	{
		m_fZoomDistance = fZoomDistance;
		m_fZoomSpeed = fZoomSpeed;
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




