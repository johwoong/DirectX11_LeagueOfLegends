#include "Actor.h"
#include "GameInstance.h"

CActor::CActor(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CGameObject(pDevice, pContext)
{
}

CActor::CActor(const CActor& rhs)
	: CGameObject(rhs)
{

}

HRESULT CActor::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CActor::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	return S_OK;
}

int CActor::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);

	if (!m_bDead)
		return OBJ_DEAD;

	return OBJ_NOEVENT;
}

void CActor::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);
}

HRESULT CActor::Render()
{
	return S_OK;
}

void CActor::SetUp_BillBoard()
{
	_float4x4 ViewMatrix;

	ViewMatrix = CGameInstance::GetInstance()->Get_TransformFloat4x4(CPipeLine::D3DTS_VIEW);
	_matrix matViewInv = XMMatrixInverse(nullptr, XMLoadFloat4x4(&ViewMatrix));

	_vector vRight = (_vector)matViewInv.r[0];
	_vector vUp = (_vector)matViewInv.r[1];
	_vector vLook = (_vector)matViewInv.r[2];
	m_pTransformCom->Set_State(CTransform::STATE_RIGHT, XMVector3Normalize(vRight) * Get_Scale().x);
	m_pTransformCom->Set_State(CTransform::STATE_UP, XMVector3Normalize(vUp) * Get_Scale().y);
	m_pTransformCom->Set_State(CTransform::STATE_RIGHT, XMVector3Normalize(vLook) * Get_Scale().z);
}

_vector CActor::Get_TransformState(CTransform::STATE eState)
{
	if (m_pTransformCom == nullptr)
		return _vector();

	if (eState == CTransform::STATE_TRANSLATION)
		return XMVectorSetW(m_pTransformCom->Get_State(eState), 1.f);

	return m_pTransformCom->Get_State(eState);
}

void CActor::Set_State(CTransform::STATE eState, _fvector vState)
{
	if (m_pTransformCom == nullptr)
		return;

	m_pTransformCom->Set_State(eState, vState);
}

void CActor::Set_Scale(_float3 vScale)
{
	m_vScale = vScale;

	m_pTransformCom->Set_Scale(CTransform::STATE_RIGHT, m_vScale.x);
	m_pTransformCom->Set_Scale(CTransform::STATE_UP, m_vScale.y);
	m_pTransformCom->Set_Scale(CTransform::STATE_LOOK, m_vScale.z);
}

void CActor::Free()
{
	__super::Free();

	Safe_Release(m_pTransformCom);
	Safe_Release(m_pRendererCom);
	Safe_Release(m_pShaderCom);
}