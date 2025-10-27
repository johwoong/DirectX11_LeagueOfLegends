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
		_float4						InitPostion = _float4(0.f, 0.f, 0.f, 1.f);
		_float4						vDistance = _float4(0, 10, -10, 0.f);
		CCamera::CAMDESC			CameraDesc;
	}CAMERADESC_DERIVED;
private:
	CCamera_Dynamic(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CCamera_Dynamic(const CCamera_Dynamic& rhs);
	virtual ~CCamera_Dynamic() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg)override;
	virtual int Update(_float fTimeDelta)override;
	virtual void Late_Update(_float fTimeDelta)override;
	virtual HRESULT Render() override;

public:
	int						PlayCamera(_float fTimeDelta);


public:
	void Set_Target(CActor* pTarget) { m_pTarget = pTarget; }
	void Set_CamMode(CAM_MODE eCamMode);
	void Set_Position(_vector vPosition);
	void Set_TargetPosition(_vector vPosition) { m_vTargetPos = vPosition; }
	void Set_Zoom(_bool type, _float fZoomDistance = 0.f, _float fZoomSpeed = 0.f);
	CActor* Get_Target() { return m_pTarget; }
	void					Set_CamMode(CAM_MODE eMode);
	_uint					Get_CamMode() { return m_eCamMode; }


public:
	void Debug_Camera(_float fTimeDelta);
	void Game_Camera(_float fTimeDelta);

private:
	class CActor* m_pTarget = { nullptr };
	CAM_MODE m_eCamMode = CAM_GAME;
	CAM_MODE m_ePreCamMode = CAM_GAME;
	_vector m_vDistance = { 0.f, 0.f, 0.f, 0.f };
	_long	m_lMouseWheel = 0;
	_vector m_vTargetPos = { 0.f, 0.f, 0.f, 0.f };
	_float m_fLookOffsetY = 4.f;
	_float m_fCameraOffsetY = 4.f;
	_float m_fAngle = 0.f;
	_vector			m_OriginPos = { 0.f,0.f,0.f,0.f };

	_vector		m_vInitPos = { 0.f, 0.f, 0.f, 0.f };


public:
	static CCamera_Dynamic* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr) override;
	virtual void Free() override;
};
NS_END

