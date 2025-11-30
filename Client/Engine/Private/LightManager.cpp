#include "LightManager.h"
#include "Light.h"

IMPLEMENT_SINGLETON(CLightManager)

CLightManager::CLightManager()
{

}

const LIGHTDESC* CLightManager::Get_LightDesc(_uint iIndex)
{
	if (iIndex >= m_Lights.size())
		return nullptr;

	auto iter = m_Lights.begin();

	for (_uint i = 0; i < iIndex; ++i)
		++iter;

	return (*iter)->Get_LightDesc();
}

void CLightManager::Set_LightDesc(_uint iIndex, LIGHTDESC* pLightDesc)
{
	if (iIndex >= m_Lights.size())
		return;

	auto	iter = m_Lights.begin();

	for (_uint i = 0; i < iIndex; ++i)
		++iter;

	(*iter)->Set_LightDesc(pLightDesc);
}

_float4x4 CLightManager::Get_ShadowLightView()
{
	_float4x4 mMatrix;

	XMStoreFloat4x4(&mMatrix, XMMatrixTranspose(XMLoadFloat4x4(&m_matShadowLightView)));

	return mMatrix;
}

HRESULT CLightManager::Set_ShadowLightView(_float4 vEye, _float4 vAt)
{
	_vector vLightEye = XMLoadFloat4(&vEye);
	_vector vLightAt = XMLoadFloat4(&vAt);
	_vector	vLightUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	XMStoreFloat4x4(&m_matShadowLightView, XMMatrixLookAtLH(vLightEye, vLightAt, vLightUp));

	return S_OK;
}


HRESULT CLightManager::Add_Light(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const LIGHTDESC& LightDesc)
{
	CLight* pLight = CLight::Create(pDevice, pContext, LightDesc);
	if (nullptr == pLight)
		return E_FAIL;

	m_Lights.push_back(pLight);

	return S_OK;
}

HRESULT CLightManager::Render_Lights(CShader* pShader, CVIBuffer_Rect* pVIBuffer)
{
	for (auto& pLight : m_Lights)
	{
		if (nullptr != pLight)
			pLight->Render(pShader, pVIBuffer);
	}

	return S_OK;
}


void CLightManager::Clear_AllLight()
{
	for (auto& pLight : m_Lights)
		Safe_Release(pLight);

	m_Lights.clear();
}

void CLightManager::Clear_Light(_uint iIndex)
{
	if (iIndex >= m_Lights.size())
		return;

	auto	iter = m_Lights.begin();

	for (_uint i = 0; i < iIndex; ++i)
		++iter;

	Safe_Release(*iter);

	iter = m_Lights.erase(iter);
}



void CLightManager::Free()
{
	for (auto& pLight : m_Lights)
		Safe_Release(pLight);

	m_Lights.clear();
}