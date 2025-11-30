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
	int iEvent = __super::Update(fTimeDelta);

	if (m_bDead)
		return OBJ_DEAD; 

	return OBJ_NOEVENT;
}

void CActor::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);

#ifdef _DEBUG
	if (m_pAABBCom != nullptr)
		m_pRendererCom->Add_Debug(m_pAABBCom);
	if (m_pOBBCom != nullptr)
		m_pRendererCom->Add_Debug(m_pOBBCom);
	if (m_pSphereCom != nullptr)
		m_pRendererCom->Add_Debug(m_pSphereCom);
#endif
}

HRESULT CActor::Render()
{
	return S_OK;
}

_bool CActor::Is_Frustum(_float fOffset)
{
	_float3 vScale = Get_Scale();
	_float fCullingRadius = max(max(vScale.x, vScale.y), vScale.z);
	return m_pGameInstance->isIn_WorldFrustum(m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION), fCullingRadius + fOffset);
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

void CActor::Update_Collider()
{
	if (m_pAABBCom != nullptr)
		m_pAABBCom->Update(m_pTransformCom->Get_WorldMatrix());
	if (m_pOBBCom != nullptr)
		m_pOBBCom->Update(m_pTransformCom->Get_WorldMatrix());
	if (m_pSphereCom != nullptr)
		m_pSphereCom->Update(m_pTransformCom->Get_WorldMatrix());
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

CColider* CActor::Get_Collider()
{
	if (m_pAABBCom != nullptr)
		return m_pAABBCom;
	if (m_pOBBCom != nullptr)
		return m_pOBBCom;
	if (m_pSphereCom != nullptr)
		return m_pSphereCom;
	else
		return nullptr;
}

void CActor::Free()
{
	__super::Free();

	Safe_Release(m_pAABBCom);
	Safe_Release(m_pOBBCom);

	Safe_Release(m_pRendererCom);
}