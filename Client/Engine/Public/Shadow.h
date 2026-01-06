#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CShadow final : public CBase
{
public:
	typedef struct tagShadowDesc
	{
		_float4		vPosition;
		_float4		vAt;

		_float		fFovy;
		_float		fNearZ;
		_float		fFarZ;
	}SHADOW_DESC;

private:
	CShadow();
	virtual ~CShadow() = default;

public:
	const _float4x4* Get_Transform(D3DTS eTransformMatrix) {
		return &m_LightTransformMatrices[ENUM_TO_UINT(eTransformMatrix)];
	}
	const _float4x4* Get_InverseTransform(D3DTS eTransformMatrix) {
		return &m_LightTransformMatrices[ENUM_TO_UINT(eTransformMatrix)];
	}

public:
	HRESULT Add_ShadowLight(ID3D11DeviceContext* pContext, const SHADOW_DESC& ShadowDesc);

private:
	_float4x4			m_LightTransformMatrices[ENUM_TO_UINT(D3DTS::END)];

public:
	static CShadow* Create();
	virtual void Free() override;
};

NS_END