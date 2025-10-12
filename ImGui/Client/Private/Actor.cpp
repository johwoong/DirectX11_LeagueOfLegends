#include "pch.h"
#include "Actor.h"

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

	if (m_bDead)
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

_vector CActor::Get_Position()
{
	if (m_pTransformCom == nullptr)
		return _vector();

	return m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
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

	if (m_pTransformCom == nullptr)
		return;

	m_pTransformCom->Set_Scale(CTransform::STATE_RIGHT, m_vScale.x);
	m_pTransformCom->Set_Scale(CTransform::STATE_UP, m_vScale.y);
	m_pTransformCom->Set_Scale(CTransform::STATE_LOOK, m_vScale.z);
}

_vector CActor::Get_TransformState(CTransform::STATE eState)
{
	if (m_pTransformCom == nullptr)
		return _vector();

	if (eState == CTransform::STATE_TRANSLATION)
		return XMVectorSetW(m_pTransformCom->Get_State(eState), 1.f);

	return m_pTransformCom->Get_State(eState);
}


void CActor::Free()
{
	__super::Free();
}
