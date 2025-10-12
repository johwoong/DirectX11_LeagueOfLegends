#include "Bone.h"

CBone::CBone()
{
}

void CBone::Set_TransformationMatrix(_fmatrix TransformationMatrix)
{
	XMStoreFloat4x4(&m_TransformationMatrix, TransformationMatrix);
}

HRESULT CBone::Initialize(BINBONE tBone, CBone* pParent)
{
	memcpy(&m_szName, tBone.szName, sizeof(char) * MAX_PATH);

	// m_OffsetMatrix 뼈의 로컬영역에서 정점의 로컬영역으로 변환해주기 위한 행렬
	// 우선 항등으로 초기화
	XMStoreFloat4x4(&m_OffsetMatrix, XMMatrixIdentity());

	// 애니메이션이 재생되면 언제든 바뀔수 있음
	// m_TransformationMatrix : 이 뼈만의 상태 : 부모기준으로 표현된, 부모의 상태를 제외한
	m_TransformationMatrix = tBone.Transformation;
	XMStoreFloat4x4(&m_TransformationMatrix, XMMatrixTranspose(XMLoadFloat4x4(&m_TransformationMatrix)));

	// 최초 변환, 이전 변환 행렬 저장
	m_PreTransformationMatrix = XMLoadFloat4x4(&m_TransformationMatrix);
	m_FirstTransformMatrix = m_PreTransformationMatrix;

	// m_CombinedTransformationMatrix : 뼈 + 부모의 상태 (원점기준으로 표현된) (부모의 상태를 포함)
	XMStoreFloat4x4(&m_CombinedTransformationMatirx, XMMatrixIdentity());

	// 자식의 주소를 가진다.
	// 부모의 주소를 가진다?? : 나의 m_CombinedTransformationMatrix를 만들기 위해서는 부모의 m_CombinedTransformationMatrix가 필요
	m_pParent = pParent;
	Safe_AddRef(m_pParent);

	return S_OK;
}

HRESULT CBone::Bin_Initialize(DATA_BINNODE* pNode)
{
	strcpy_s(m_szName, pNode->cName);
	strcpy_s(m_szParentName, pNode->cParent);

	XMStoreFloat4x4(&m_OffsetMatrix, XMMatrixIdentity());

	if (!isfinite(pNode->mTransform._11) || fabs(pNode->mTransform._11) > 100000.f)
		XMStoreFloat4x4(&m_TransformationMatrix, XMMatrixIdentity());
	else
		memcpy(&m_TransformationMatrix, &pNode->mTransform, sizeof(_float4x4));

	XMStoreFloat4x4(&m_CombinedTransformationMatirx, XMMatrixIdentity());
	return E_NOTIMPL;
}

void CBone::Invaliddate_CombiendTransformationmatrix(const char* pBoneName)
{
	Set_RootMotionMatrix(pBoneName);

	if (nullptr != m_pParent)
		XMStoreFloat4x4(&m_CombinedTransformationMatirx, XMLoadFloat4x4(&m_TransformationMatrix) * XMLoadFloat4x4(&m_CombinedTransformationMatirx));
	else
		m_CombinedTransformationMatirx = m_TransformationMatrix;
}

void CBone::Set_FindParent(CBone* pNode)
{
	const char* pName = pNode->Get_Name();
	if (!strcmp(m_szParentName, pName))
	{
		m_pParent = pNode;
		Safe_AddRef(m_pParent);
		return;
	}
	m_pParent = nullptr;
}

void CBone::Set_RootMotionMatrix(const char* pBoneName)
{
	// 루트 본 이름 비교로 찾기
	if ((nullptr != pBoneName) && !strcmp(m_szName, pBoneName))
	{
		_matrix AfterTransformMatrix = XMLoadFloat4x4(&m_TransformationMatrix);

		_vector vPreQuat, vAfterQuat;
		vPreQuat = XMQuaternionNormalize(XMQuaternionRotationMatrix(m_PreTransformationMatrix));
		vAfterQuat = XMQuaternionNormalize(XMQuaternionRotationMatrix(AfterTransformMatrix));

		// 쿼터니언 내적을 통한 두 쿼터니언 사이 각 구함
		m_fRotationRaidan = acosf(XMVectorGetX(XMQuaternionDot(vPreQuat, vAfterQuat))) * 2.f;

		// 이전 변환 행렬 저장
		m_PreTransformationMatrix = AfterTransformMatrix;

		// 루트 본 이동, 회전 값 제거
		AfterTransformMatrix = m_FirstTransformMatrix;
		AfterTransformMatrix.r[3] = XMVectorSet(0.f, 0.f, 0.f, 1.f);
		XMStoreFloat4x4(&m_TransformationMatrix, AfterTransformMatrix);
	}
}

CBone* CBone::Create(BINBONE tBone, CBone* pParent)
{
	CBone* pInstance = new CBone();

	if (FAILED(pInstance->Initialize(tBone, pParent)))
	{
		MSG_BOX("Failed to Created : CHierarchyNode");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CBone* CBone::Bin_Create(DATA_BINNODE* pNode)
{
	CBone* pInstance = new CBone();

	if (FAILED(pInstance->Bin_Initialize(pNode)))
	{
		MSG_BOX("Failed To Created : CHierarchyNode_Bin");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CBone::Free()
{
	Safe_Release(m_pParent);
}
