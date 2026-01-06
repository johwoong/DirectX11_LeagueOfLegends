#pragma once
#include "Base.h"

NS_BEGIN(Engine)
class CLightManager final : public CBase
{
	DECLARE_SINGLETON(CLightManager)

private:
	CLightManager();
	virtual ~CLightManager() = default;

public:
	const LIGHTDESC* Get_LightDesc(_uint iIndex);
	list<class CLight*>* GetLights() { return &m_Lights; }
	_uint Get_LightSize() { return (_uint)m_Lights.size(); }
	void Set_LightDesc(_uint iIndex, LIGHTDESC* pLightDesc);
public:
	_float4x4	Get_ShadowLightView();
	HRESULT		Set_ShadowLightView(_float4 vEye, _float4 vAt);
public:
	_float4x4 Get_ShadowLightProj()
	{
		_float4x4 m;
		XMStoreFloat4x4(&m, XMMatrixTranspose(XMLoadFloat4x4(&m_matShadowLightProj)));
		return m;
	}

	HRESULT Set_ShadowLightProj_Ortho(float w, float h, float zn, float zf)
	{
		XMStoreFloat4x4(&m_matShadowLightProj, XMMatrixOrthographicLH(w, h, zn, zf));
		return S_OK;
	}
public:
	HRESULT Add_Light(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const LIGHTDESC& LightDesc);
	HRESULT Render_Lights(class CShader* pShader, class CVIBuffer_Rect* pVIBuffer);
	void Clear_AllLight();
	void Clear_Light(_uint iIndex);


private:
	list<class CLight*>				m_Lights;
	_float4x4						m_matShadowLightView;
private:
	_float4x4 m_matShadowLightProj;
public:
	virtual void Free() override;
};
NS_END
