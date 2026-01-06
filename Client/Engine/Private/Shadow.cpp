#include "Shadow.h"

CShadow::CShadow()
{
}

HRESULT CShadow::Add_ShadowLight(ID3D11DeviceContext* pContext, const SHADOW_DESC& ShadowDesc)
{
	XMStoreFloat4x4(&m_LightTransformMatrices[ENUM_TO_UINT(D3DTS::VIEW)],
		XMMatrixLookAtLH(XMLoadFloat4(&ShadowDesc.vPosition), XMLoadFloat4(&ShadowDesc.vAt), XMVectorSet(0.f, 1.f, 0.f, 0.f)));

	D3D11_VIEWPORT          ViewportDesc{};
	_uint           iNumViewports = { 1 };

	pContext->RSGetViewports(&iNumViewports, &ViewportDesc);
	XMStoreFloat4x4(&m_LightTransformMatrices[ENUM_TO_UINT(D3DTS::PROJ)],
		XMMatrixPerspectiveFovLH(ShadowDesc.fFovy, static_cast<_float>(ViewportDesc.Width) / ViewportDesc.Height, ShadowDesc.fNearZ, ShadowDesc.fFarZ));

	return S_OK;
}

CShadow* CShadow::Create()
{
	return new CShadow();
}

void CShadow::Free()
{
	__super::Free();


}
