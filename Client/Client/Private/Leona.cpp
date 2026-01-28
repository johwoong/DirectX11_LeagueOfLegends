#include "Leona.h"
#include "LeonaState.h"
#include "LeonaIdleState.h"

using namespace Leona;

_bool CLeona::Is_AnimationLoop(_uint eAnimId)
{
	switch ((ANIM)eAnimId)
	{
	case IDLE:
	case IDLE2:
	case IDLE3:
	case RUN:
		return true;
	}

	return false;
}

void CLeona::Ready_Animation_Event()
{
}



CLeona::CLeona(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CEnemy(pDevice, pContext)
{
}

CLeona::CLeona(const CLeona& rhs)
	: CEnemy(rhs)
{
}

HRESULT CLeona::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CLeona::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	CLeonaState* pState = new CIdleState(this);
	m_pState = m_pState->ChangeState(m_pState, pState);
	m_eEnemyType = LEONA;
	Set_Scale(_float3(0.012f, 0.012f, 0.012f));
	m_tStats.MaxHp = 3000.f;
	m_tStats.Hp = m_tStats.MaxHp;
	m_tStats.MoveSpeed = 4.f;
	m_tStats.AttackRange = 8.f;
	m_tStats.AttackDamage = 60.f;
	m_tStats.AttackSpeed = 0.4f;
	m_tAggro.fAttackRange = 8.f;
	m_tAggro.fChaseRange = 10.f;   // 감지 + 추적
	m_tAggro.fGiveUpRange = 16.f;   // 이 이상이면 완전 포기
	return S_OK;
}

int CLeona::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);

	AI_Behavior(fTimeDelta);
	Update_State(fTimeDelta);

	m_fTimeDeltaAcc += fTimeDelta;


	return OBJ_NOEVENT;
}

void CLeona::Late_Update(_float fTimeDelta)
{

	//if (!Is_Frustum(2.f))
	//	return;
	__super::Late_Update(fTimeDelta);
	if (m_pRendererCom)
		m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_NOALPHABLEND, this);

	LateUpdate_State(fTimeDelta);
}

HRESULT CLeona::Render()
{
	__super::Render();
	return S_OK;
}

HRESULT CLeona::Render_Glow()
{
	return S_OK;
}

void CLeona::AI_Behavior(_float fTimeDelta)
{
	CLeonaState* pNewState = m_pState->AI_Behaviour(fTimeDelta);
	if (pNewState)
		m_pState = m_pState->ChangeState(m_pState, pNewState);
}

void CLeona::Update_State(_float fTimeDelta)
{
	CLeonaState* pNewState = m_pState->Update(fTimeDelta);
	if (pNewState)
		m_pState = m_pState->ChangeState(m_pState, pNewState);
}

void CLeona::LateUpdate_State(_float fTimeDelta)
{
	CLeonaState* pNewState = m_pState->LateUpdate(fTimeDelta);
	if (pNewState)
		m_pState = m_pState->ChangeState(m_pState, pNewState);
}

void CLeona::Check_Navigation()
{
	_vector vPosition = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
	_float m_fWalkingHeight = m_pNavigationCom->Compute_Height(vPosition, 0.f);

	vPosition = XMVectorSetY(vPosition, m_fWalkingHeight);
	m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vPosition);
}

HRESULT CLeona::Ready_Components(void* pArg)
{
	/* For.Com_Renderer */
	if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRendererCom)))
		return E_FAIL;

	/* For.Com_Transform */
	CTransform::TRANSFORMDESC TransformDesc;
	ZeroMemory(&TransformDesc, sizeof(CTransform::TRANSFORMDESC));
	TransformDesc.fSpeedPerSec = 5.f;
	TransformDesc.fRotationPerSec = XMConvertToRadians(90.0f);
	if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC, TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom, &TransformDesc)))
		return E_FAIL;

	/* For.Com_Shader */
	if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxAnimModel"), (CComponent**)&m_pShaderCom)))
		return E_FAIL;

	/* For.Com_Model*/
	if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Leona"), (CComponent**)&m_pModelCom)))
		return E_FAIL;

	CNavigation::NAVDESC NaviDesc;
	ZeroMemory(&NaviDesc, sizeof NaviDesc);
	if (FAILED(__super::Add_Components(TEXT("Com_Navigation"), LEVEL_STATIC, TEXT("Prototype_Component_ChampionNavigation"), (CComponent**)&m_pNavigationCom, &NaviDesc)))
		return E_FAIL;


	CColider::COLLIDERDESC ColliderDesc;
	ZeroMemory(&ColliderDesc, sizeof(CColider::COLLIDERDESC));
	ColliderDesc.vScale = _float3(1.f, 1.f, 1.f);
	ColliderDesc.vRotation = _float3(0.f, 0.f, 0.f);
	ColliderDesc.vPosition = _float3(0.f, 30.f, 0.f);
	if (FAILED(__super::Add_Components(TEXT("Com_SPHERE"), LEVEL_STATIC, TEXT("Prototype_Component_Colider_Sphere"), (CComponent**)&m_pSphereCom, &ColliderDesc)))
		return E_FAIL;
	return S_OK;
}

HRESULT CLeona::SetUp_ShaderID()
{
	m_eShaderID = SHADER_ANIMDEFAULT;

	return S_OK;
}

CLeona* CLeona::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CLeona* pInstance = new CLeona(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CAlistar");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CLeona::Clone(void* pArg)
{
	CLeona* pInstance = new CLeona(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CAlistar");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CLeona::Free()
{
	__super::Free();

	Safe_Delete(m_pState);
}
