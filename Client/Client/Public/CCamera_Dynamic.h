#pragma once

#include "Client_Defines.h"
#include "Camera.h"

NS_BEGIN(Client)

class CActor;
class CCamera_Dynamic final : public CCamera
{
public:
	enum CAM_MODE { CAM_DEBUG, CAM_GAME };

	typedef struct tagCameraDesc_Derived
	{
		_float4 InitPostion = _float4(0.f, 0.f, 0.f, 1.f);
		CCamera::CAMDESC CameraDesc;
	} CAMERADESC_DERIVED;

private:
	CCamera_Dynamic(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CCamera_Dynamic(const CCamera_Dynamic& rhs);
	virtual ~CCamera_Dynamic() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;
	virtual int Update(_float fTimeDelta) override;
	virtual void Late_Update(_float fTimeDelta) override;
	virtual HRESULT Render() override;

private:
	void Game_Camera(_float fTimeDelta);
	void Debug_Camera(_float fTimeDelta);

public:
	void Set_Target(class CActor* pTarget) { m_pTarget = pTarget; }
	void Set_CamMode(CAM_MODE eCamMode);

public:
	static CCamera_Dynamic* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr) override;
	virtual void Free() override;

private:
	class CActor* m_pTarget = nullptr;
	CAM_MODE m_eCamMode = CAM_GAME;
	CAM_MODE m_ePrevMode = CAM_DEBUG;

	// Zoom 관련
	_float m_fZoomDistance = 35.f; // 기본 거리
	_float m_fMinZoom = 25.f;
	_float m_fMaxZoom = 55.f;
	_float m_fZoomSpeed = 5.f;
	_float m_fZoomTarget = 35.f;

	// 카메라 구성
	_float m_fCameraHeight = 35.f;
	_float m_fCameraAngle = 55.f;
	_float m_fZoomFOV = 0.f;

	_long m_lWheel = 0;
};

NS_END