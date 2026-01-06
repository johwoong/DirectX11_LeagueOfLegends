#include "Target_Manager.h"
#include "RenderTarget.h"

IMPLEMENT_SINGLETON(CTarget_Manager)

CTarget_Manager::CTarget_Manager()
{
}

HRESULT CTarget_Manager::Ready_ShadowDepthStencilRenderTargetView(ID3D11Device* pDevice, _uint iWinCX, _uint iWinCY)
{
	if (!pDevice)
		return E_FAIL;

	m_ShadowCX = iWinCX;
	m_ShadowCY = iWinCY;

	Safe_Release(m_pShadowDeptheStencil);

	ID3D11Texture2D* pDepthStencilTexture = nullptr;

	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = iWinCX;
	desc.Height = iWinCY;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	if (FAILED(pDevice->CreateTexture2D(&desc, nullptr, &pDepthStencilTexture)))
		return E_FAIL;

	if (FAILED(pDevice->CreateDepthStencilView(pDepthStencilTexture, nullptr, &m_pShadowDeptheStencil)))
	{
		Safe_Release(pDepthStencilTexture);
		return E_FAIL;
	}

	Safe_Release(pDepthStencilTexture);
	return S_OK;
}

HRESULT CTarget_Manager::Ready_BackBufferCopyTexture(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _uint iWinCX, _uint iWinCY)
{
	if (!pDevice)
		return E_FAIL;

	/* Get Current Render Target (BackBuffer). */
	ID3D11RenderTargetView* pBackBufferRTV = nullptr;
	pContext->OMGetRenderTargets(1, &pBackBufferRTV, nullptr);

	/* Get BackBuffer Render Target Description. */
	D3D11_RENDER_TARGET_VIEW_DESC pBackBufferDesc;
	pBackBufferRTV->GetDesc(&pBackBufferDesc);

	/* Create the Texture where the copy of the BackBuffer will be copied to. */
	D3D11_TEXTURE2D_DESC tBackBufferCopyDesc;
	ZeroMemory(&tBackBufferCopyDesc, sizeof(D3D11_TEXTURE2D_DESC));

	tBackBufferCopyDesc.Width = iWinCX;
	tBackBufferCopyDesc.Height = iWinCY;
	tBackBufferCopyDesc.MipLevels = 1;
	tBackBufferCopyDesc.ArraySize = 1;
	tBackBufferCopyDesc.Format = pBackBufferDesc.Format;
	tBackBufferCopyDesc.SampleDesc.Quality = 0;
	tBackBufferCopyDesc.SampleDesc.Count = 1;
	tBackBufferCopyDesc.Usage = D3D11_USAGE_DEFAULT;
	tBackBufferCopyDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	tBackBufferCopyDesc.CPUAccessFlags = 0;
	tBackBufferCopyDesc.MiscFlags = 0;
	m_WinCX = iWinCX;
	m_WinCY = iWinCY;

	if (FAILED(pDevice->CreateTexture2D(&tBackBufferCopyDesc, nullptr, &m_pBackBufferTextureCopy)))
		return E_FAIL;

	Safe_Release(pBackBufferRTV);

	return S_OK;
}

HRESULT CTarget_Manager::Add_RenderTarget(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _tchar* pTargetTag, _uint iSizeX, _uint iSizeY, DXGI_FORMAT eFormat, const _float4& pColor)
{
	if (Find_RenderTarget(pTargetTag))
		return E_FAIL;

	CRenderTarget* pRenderTarget = CRenderTarget::Create(pDevice, pContext, iSizeX, iSizeY, eFormat, pColor);
	if (!pRenderTarget)
		return E_FAIL;

	m_RenderTargets.emplace(pTargetTag, pRenderTarget);

	return S_OK;
}

HRESULT CTarget_Manager::Add_MRT(const _tchar* pMRTTag, const _tchar* pTargetTag)
{
	CRenderTarget* pRenderTarget = Find_RenderTarget(pTargetTag);
	if (!pRenderTarget)
		return E_FAIL;

	list<CRenderTarget*>* pMRTList = Find_MRT(pMRTTag);
	if (!pMRTList)
	{
		list<CRenderTarget*> MRTList;
		MRTList.push_back(pRenderTarget);
		m_MRTs.emplace(pMRTTag, MRTList);
	}
	else
		pMRTList->push_back(pRenderTarget);

	Safe_AddRef(pRenderTarget);

	return S_OK;
}

HRESULT CTarget_Manager::Begin_MRT(ID3D11DeviceContext* pContext, const _tchar* pMRTTag)
{
	list<CRenderTarget*>* pMRTList = Find_MRT(pMRTTag);
	if (!pMRTList)
		return E_FAIL;

	pContext->OMGetRenderTargets(1, &m_pOldRTV, &m_pOldDSV);

	ID3D11RenderTargetView* pRTVs[8] = { nullptr };
	_uint iNumRenderTargets = 0;

	for (auto& pRenderTarget : *pMRTList)
	{
		pRenderTarget->Clear();
		pRTVs[iNumRenderTargets++] = pRenderTarget->Get_RTV();
	}

	pContext->OMSetRenderTargets(iNumRenderTargets, pRTVs, m_pOldDSV);
	m_OldNumViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
	pContext->RSGetViewports(&m_OldNumViewports, m_OldViewports);
	return S_OK;
}

HRESULT CTarget_Manager::Begin_ShadowMRT(ID3D11DeviceContext* pContext, const _tchar* pMRTTag)
{
	auto* pMRTList = Find_MRT(pMRTTag);
	if (!pMRTList)
		return E_FAIL;

	// 1) 기존 RTV/DSV 저장
	pContext->OMGetRenderTargets(1, &m_pOldRTV, &m_pOldDSV);

	// 2) 기존 뷰포트 저장 (Shadow도 반드시 저장해야 함)
	m_OldNumViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
	pContext->RSGetViewports(&m_OldNumViewports, m_OldViewports);

	ID3D11RenderTargetView* pRTVs[8] = {};
	_uint iNum = 0;

	for (auto& pRenderTarget : *pMRTList)
	{
		pRenderTarget->Clear();
		pRTVs[iNum++] = pRenderTarget->Get_RTV();
	}

	// 3) Shadow DSV clear
	pContext->ClearDepthStencilView(m_pShadowDeptheStencil,
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	// 4) ? Shadow RTV + ? Shadow DSV 바인딩 (여기서 끝! 다시 oldDSV로 바꾸면 안 됨)
	pContext->OMSetRenderTargets(iNum, pRTVs, m_pShadowDeptheStencil);

	// 5) Shadow 뷰포트 설정
	D3D11_VIEWPORT vp{};
	vp.TopLeftX = 0.f;
	vp.TopLeftY = 0.f;
	vp.Width = (FLOAT)m_ShadowCX;
	vp.Height = (FLOAT)m_ShadowCY;
	vp.MinDepth = 0.f;
	vp.MaxDepth = 1.f;
	pContext->RSSetViewports(1, &vp);

	return S_OK;
}

HRESULT CTarget_Manager::End_MRT(ID3D11DeviceContext* pContext)
{
	pContext->OMSetRenderTargets(1, &m_pOldRTV, m_pOldDSV);

	// ? 뷰포트 복구는 "저장한 것"만 복구
	if (m_OldNumViewports > 0)
		pContext->RSSetViewports(m_OldNumViewports, m_OldViewports);

	Safe_Release(m_pOldRTV);
	Safe_Release(m_pOldDSV);
	return S_OK;
}

HRESULT CTarget_Manager::Bind_ShaderResource(const _tchar* pTargetTag, CShader* pShader, const char* pConstantName)
{
	CRenderTarget* pRenderTarget = Find_RenderTarget(pTargetTag);
	if (nullptr == pRenderTarget)
		return E_FAIL;

	return pRenderTarget->Bind_ShaderResource(pShader, pConstantName);
}

HRESULT CTarget_Manager::Copy_BackBufferTexture(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	/* Get Current Render Target (BackBuffer). */
	ID3D11RenderTargetView* pBackBufferRTV = nullptr;
	pContext->OMGetRenderTargets(1, &pBackBufferRTV, nullptr);

	/* Get BackBuffer RenderTarget Texture. */
	ID3D11Resource* pBackBufferResource = nullptr;
	pBackBufferRTV->GetResource(&pBackBufferResource);

	/* Copy the BackBuffer Texture into "m_pBackBufferTextureCopy". */
	pContext->CopyResource(m_pBackBufferTextureCopy, pBackBufferResource);

	Safe_Release(m_pBackBufferSRV);

	/* Make a Shader Resource View based on the copied "m_pBackBufferTextureCopy". */
	if (FAILED(pDevice->CreateShaderResourceView(m_pBackBufferTextureCopy, nullptr, &m_pBackBufferSRV)))
		return E_FAIL;

	Safe_Release(pBackBufferResource);
	Safe_Release(pBackBufferRTV);

	return S_OK;
}

HRESULT CTarget_Manager::Ready_Debug(const _tchar* pTargetTag, _float fX, _float fY, _float fSizeX, _float fSizeY)
{
	CRenderTarget* pRenderTarget = Find_RenderTarget(pTargetTag);
	if (!pRenderTarget)
		return E_FAIL;

	return pRenderTarget->Ready_Debug(fX, fY, fSizeX, fSizeY);
}

HRESULT CTarget_Manager::Render_Debug(const _tchar* pMRTTag, CShader* pShader, CVIBuffer_Rect* pVIBuffer)
{
	list<class CRenderTarget*>* pMRTList = Find_MRT(pMRTTag);

	for (auto& pTarget : *pMRTList)
	{
		if (pTarget)
			pTarget->Render_Debug(pShader, pVIBuffer);
	}

	return S_OK;
}

CRenderTarget* CTarget_Manager::Find_RenderTarget(const _tchar* pTargetTag)
{
	auto iter = find_if(m_RenderTargets.begin(), m_RenderTargets.end(), CTag_Finder(pTargetTag));

	if (iter == m_RenderTargets.end())
		return nullptr;

	return iter->second;
}

list<class CRenderTarget*>* CTarget_Manager::Find_MRT(const _tchar* pMRTTag)
{
	auto iter = find_if(m_MRTs.begin(), m_MRTs.end(), CTag_Finder(pMRTTag));

	if (iter == m_MRTs.end())
		return nullptr;

	return &iter->second;
}

void CTarget_Manager::Free()
{
	Safe_Release(m_pShadowDeptheStencil);

	for (auto& Pair : m_MRTs)
	{
		for (auto& pRenderTarget : Pair.second)
		{
			Safe_Release(pRenderTarget);
		}
		Pair.second.clear();
	}
	m_MRTs.clear();

	for (auto& Pair : m_RenderTargets)
		Safe_Release(Pair.second);

	m_RenderTargets.clear();

	Safe_Release(m_pOldRTV);
	Safe_Release(m_pOldDSV);
	Safe_Release(m_pBackBufferTextureCopy);
	Safe_Release(m_pBackBufferSRV);
}
