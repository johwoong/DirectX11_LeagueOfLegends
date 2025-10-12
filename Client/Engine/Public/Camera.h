#pragma once

#include "GameObject.h"
#include "Transform.h"

NS_BEGIN(Engine)

class ENGINE_DLL CCamera abstract: public CGameObject 
{
public:
	typedef struct tagCameraDesc
	{
		_float4		vEye;
		_float4		vAt;

		_float		fAspect;
		_float		fFov;
		_float		fNear;
		_float		fFar;

		CTransform::TRANSFORMDESC	TransformDesc;
	}CAMDESC;
protected:
	CCamera(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CCamera(const CCamera& rhs);
	virtual ~CCamera() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual int Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);
	virtual HRESULT Render();
	CAMDESC Get_CameraDesc() { return m_CameraDesc; }

protected:
	static const _tchar* m_pTransformTag;
	class CTransform* m_pTransformCom = { nullptr };
	CAMDESC			m_CameraDesc;
	class CPipeLine* m_pPipeLine = { nullptr };

protected:
	HRESULT Bind_OnPipeLine();

public:
	virtual CGameObject* Clone(void* pArg = nullptr) = 0;
	virtual void Free() override;

};
NS_END


