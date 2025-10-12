#pragma once
#include "Base.h"
// 카메라에서 갱신하는 뷰, 투영 행렬 보관

NS_BEGIN(Engine)
class CPipeLine final : public CBase
{
	DECLARE_SINGLETON(CPipeLine)
public:
	enum TRANSFORMSTATE{D3DTS_VIEW, D3DTS_PROJ, D3DTS_END};

private:
	CPipeLine();
	virtual ~CPipeLine() = default;

public:
	void Set_Transform(TRANSFORMSTATE eState, _fmatrix TransformMatrix) {
		XMStoreFloat4x4(&m_pipeLineMatrix[eState], TransformMatrix);
	}

	_matrix Get_TransformMatirx(TRANSFORMSTATE eState) {
		return XMLoadFloat4x4(&m_pipeLineMatrix[eState]);
	}
	_float4x4 Get_TransformFloat4x4(TRANSFORMSTATE eState) {
		return m_pipeLineMatrix[eState];
	}

	_float4x4& Get_TransformFloat4x4_TP(TRANSFORMSTATE eState) {
		return m_pipeLineMatrix_TP[eState];
	}

	_float4x4 Get_TransformFlaot4x4_Inverse(TRANSFORMSTATE eState) {
		return m_pipeLineMatrix_Inverse[eState];
	}

	_float4x4 Get_TransformFloat4x4_Inverse_TP(TRANSFORMSTATE eState) {
		return m_pipeLineMatrix_Inverse_TP[eState];
	}

	_float4 Get_CamPosition() {
		return m_vCamPosition;
	}

	_float4x4 Get_CamWorldMatrix() {
		return m_camWorldMatrix;
	}

public:
	void Update();


private:
	_float4x4 m_pipeLineMatrix[D3DTS_END];
	_float4x4 m_pipeLineMatrix_TP[D3DTS_END];
	_float4x4 m_pipeLineMatrix_Inverse[D3DTS_END];
	_float4x4 m_pipeLineMatrix_Inverse_TP[D3DTS_END];
	_float4	  m_vCamPosition;
	_float4x4 m_camWorldMatrix;
	bool m_bHasView = false;
	bool m_bHasProj = false;

public:
	virtual void  Free() override;
};
NS_END

