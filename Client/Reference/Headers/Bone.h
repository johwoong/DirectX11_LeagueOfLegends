#pragma once

// 뼈
#include "Base.h"

NS_BEGIN(Engine)
class CBone final :public CBase
{
private:
	CBone();
	virtual ~CBone() = default;

public:
	const char* Get_Name() const { return m_szName; }
	const char* Get_ParentName() const {
		if (nullptr == m_pParent)
			return "";
		return m_pParent->Get_Name();
	}

	void Set_OffsetMatrix(_fmatrix OffsetMatrix) {
		XMStoreFloat4x4(&m_OffsetMatrix, OffsetMatrix);
	}

	void Set_TransformationMatrix(_fmatrix TransformationMatrix);

	_matrix Get_OffsetMatrix() const {
		return XMLoadFloat4x4(&m_OffsetMatrix);
	}
	_matrix Get_TransformationMatrix(void) const { return XMLoadFloat4x4(&m_TransformationMatrix); }
	_matrix Get_CombinedTransformationMatrix() const {
		return XMLoadFloat4x4(&m_CombinedTransformationMatirx);
	}

	XMFLOAT4X4 Get_OffsetMatrixXM() {
		return m_OffsetMatrix;
	}
	_float4x4 Get_OriTransformationMatrix() { return m_OriTransfomationMatrix; }
	CBone* Get_Parent() { return m_pParent; }

	// 루트노드 함수 - 이동 행렬, 회전 행렬 반환
	_vector Get_Translation(void) { return m_vecTranslation; }
	_float	Get_RotationRadian(void) { return m_fRotationRaidan; }
	void	Set_Translation(_vector vTranslation) { m_vecTranslation = vTranslation; }

public:
	HRESULT Initialize(BINBONE tBone, CBone* pParent);
	HRESULT Bin_Initialize(DATA_BINNODE* pNode);
	void	Invaliddate_CombiendTransformationmatrix(const char* pBoneName);
	void	Set_FindParent(CBone* pNode);

private:
	char	m_szName[MAX_PATH] = "";
	char	m_szParentName[MAX_PATH] = "";
	CBone* m_pParent = nullptr;

	// 기본은 항등 메시 안에서 aiBoneㄱ밧을 채움
	_float4x4	m_OffsetMatrix;
	_float4x4	m_OriTransfomationMatrix;

	_float4x4	m_TransformationMatrix;

	_float4x4	m_CombinedTransformationMatirx;

	// 이동, 회전갑 살리기
	_vector m_vecTranslation;
	_float m_fRotationRaidan;
	_matrix m_PreTransformationMatrix;
	_matrix m_FirstTransformMatrix;

private:
	// 루트 본 이동 값제거, 루트 본이동, 회전행렬 저장 함수
	void Set_RootMotionMatrix(const char* pBoneName);

public:
	static CBone* Create(BINBONE tBone, CBone* pParent);
	static CBone* Bin_Create(DATA_BINNODE* pNode);
	virtual void Free();

};
NS_END

