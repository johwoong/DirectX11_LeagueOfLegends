#include "CursorManager.h"
#include "GameInstance.h"

IMPLEMENT_SINGLETON(CCursorManager)

void CCursorManager::Update_Ray()
{
	CGameInstance* GI = CGameInstance::GetInstance();
	float mouseX = (float)m_tMousePos.x;
	float mouseY = (float)m_tMousePos.y;
	float winX = g_WinSizeX;
	float winY = g_WinSizeY;
	// ---------- 1) NDC ----------
	float ndcX = (mouseX / winX) * 2.f - 1.f;
	float ndcY = 1.f - (mouseY / winY) * 2.f;
	_vector vNDC = XMVectorSet(ndcX, ndcY, 1.f, 1.f);
	// ---------- 2) 역투영하여 View Space ----------
	_matrix proj = GI->Get_TransformMatrix(CPipeLine::D3DTS_PROJ);
	_matrix invProj = XMMatrixInverse(nullptr, proj);
	_vector vView = XMVector3TransformCoord(vNDC, invProj);
	vView = XMVectorSetW(vView, 0.f);
	_matrix view = GI->Get_TransformMatrix(CPipeLine::D3DTS_VIEW);
	_matrix invView = XMMatrixInverse(nullptr, view);
	m_vRayDir = XMVector3Normalize(XMVector3TransformNormal(vView, invView));
	m_vRayPos = XMVector3TransformCoord(XMVectorZero(), invView); // 카메라 위치
}

void CCursorManager::WorldToScreen(_fvector vWorldPos, _float2* pOutScreen)
{
	CGameInstance* pGI = CGameInstance::GetInstance();

	_float4x4 View = pGI->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_VIEW);
	_float4x4 Proj = pGI->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_PROJ);

	_vector clip = XMVector3TransformCoord(vWorldPos, XMLoadFloat4x4(&View));
	clip = XMVector3TransformCoord(clip, XMLoadFloat4x4(&Proj));

	// NDC → Screen
	float x = XMVectorGetX(clip);
	float y = XMVectorGetY(clip);

	pOutScreen->x = (x * 0.5f + 0.5f) * g_WinSizeX;
	pOutScreen->y = (-y * 0.5f + 0.5f) * g_WinSizeY;
}


