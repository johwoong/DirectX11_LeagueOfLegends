#include "PipeLine.h"

IMPLEMENT_SINGLETON(CPipeLine)

CPipeLine::CPipeLine()
{
 	for (int i = 0; i < D3DTS_END; ++i) {
		XMStoreFloat4x4(&m_pipeLineMatrix[i], XMMatrixIdentity());
		XMStoreFloat4x4(&m_pipeLineMatrix_TP[i], XMMatrixIdentity());
		XMStoreFloat4x4(&m_pipeLineMatrix_Inverse[i], XMMatrixIdentity());
		XMStoreFloat4x4(&m_pipeLineMatrix_Inverse_TP[i], XMMatrixIdentity());
	}
	XMStoreFloat4x4(&m_camWorldMatrix, XMMatrixIdentity());
	m_vCamPosition = { 0,0,0,1 };
	m_bHasView = false;
	m_bHasProj = false;
}

void CPipeLine::Update()
{
  	XMStoreFloat4(&m_vCamPosition, XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_pipeLineMatrix[D3DTS_VIEW])).r[3]);

	XMStoreFloat4x4(&m_camWorldMatrix, XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_pipeLineMatrix[D3DTS_VIEW])));

	for (_uint i = 0; i < 2; ++i)
	{
		XMStoreFloat4x4(&m_pipeLineMatrix_TP[i], XMMatrixTranspose(XMLoadFloat4x4(&m_pipeLineMatrix[i])));

		XMStoreFloat4x4(&m_pipeLineMatrix_Inverse[i], XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_pipeLineMatrix[i])));

		XMStoreFloat4x4(&m_pipeLineMatrix_Inverse_TP[i], XMMatrixTranspose(XMLoadFloat4x4( & m_pipeLineMatrix_Inverse[i])));
	}
}

void CPipeLine::Free()
{
}
