#include "Renekton.h"
#include "RenektonState.h"
#include "RenektonIdleState.h"

using namespace Renekton;

_bool CRenekton::Is_AnimationLoop(_uint eAnimId)
{
	switch ((ANIM)eAnimId)
	{
	case IDLE:
	case RUN:
	case IDLE2:
	case IDLE3:
		return true;
	default:
		return false;
	}
}

void CRenekton::Ready_Animation_Event()
{
}



CRenekton::CRenekton(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CEnemy(pDevice, pContext)
{
}

CRenekton::CRenekton(const CRenekton& rhs)
	: CEnemy(rhs)
{
}

HRESULT CRenekton::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CRenekton::Initialize(void* pArg)
{

	// 2) Enemy 쪽 Initialize (여기서 HPBar 생성 + Set_Owner + MaxHp 스냅샷)
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	CRenektonState* pState = new CIdleState(this);
	m_pState = m_pState->ChangeState(m_pState, pState);
	m_eEnemyType = RENEKTON;

	Set_Scale(_float3(0.012f, 0.012f, 0.012f));
	m_tStats.MaxHp = 500.f;
	m_tStats.Hp = m_tStats.MaxHp;
	return S_OK;
}

int CRenekton::Update(_float fTimeDelta)
{
	// 1) 이미 죽었으면 바로 리턴
	if (m_bDead)
		return OBJ_DEAD;

	// 2) 부모 Update 호출하고, 여기 리턴값을 그대로 반영
	int iEvent = __super::Update(fTimeDelta);
	if (iEvent < 0)       // OBJ_DEAD(-1) 이나 OBJ_POOL(-2) 같은 값이면
		return iEvent;

	// 3) 살아 있는 경우에만 AI / State 진행
	AI_Behavior(fTimeDelta);
	Update_State(fTimeDelta);

	m_fTimeDeltaAcc += fTimeDelta;

	return OBJ_NOEVENT;
}

void CRenekton::Late_Update(_float fTimeDelta)
{
	if (m_bDead)
		return;

	__super::Late_Update(fTimeDelta);
	LateUpdate_State(fTimeDelta);
}

HRESULT CRenekton::Render()
{
	__super::Render();
	return S_OK;
}

HRESULT CRenekton::Render_Glow()
{
	return S_OK;
}

void CRenekton::AI_Behavior(_float fTimeDelta)
{
	CRenektonState* pNewState = m_pState->AI_Behaviour(fTimeDelta);
	if (pNewState)
		m_pState = m_pState->ChangeState(m_pState, pNewState);
}

void CRenekton::Update_State(_float fTimeDelta)
{
	CRenektonState* pNewState = m_pState->Update(fTimeDelta);
	if (pNewState)
		m_pState = m_pState->ChangeState(m_pState, pNewState);
}

void CRenekton::LateUpdate_State(_float fTimeDelta)
{
	CRenektonState* pNewState = m_pState->LateUpdate(fTimeDelta);
	if (pNewState)
		m_pState = m_pState->ChangeState(m_pState, pNewState);
}

void CRenekton::Check_Navigation()
{
	_vector vPosition = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
	_float m_fWalkingHeight = m_pNavigationCom->Compute_Height(vPosition, 0.f);

	vPosition = XMVectorSetY(vPosition, m_fWalkingHeight);
	m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vPosition);
}

HRESULT CRenekton::Ready_Components(void* pArg)
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
	if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Renekton"), (CComponent**)&m_pModelCom)))
		return E_FAIL;

	CNavigation::NAVDESC NaviDesc;
	ZeroMemory(&NaviDesc, sizeof NaviDesc);
	if (FAILED(__super::Add_Components(TEXT("Com_Navigation"), LEVEL_STATIC, TEXT("Prototype_Component_ChampionNavigation"), (CComponent**)&m_pNavigationCom, &NaviDesc)))
		return E_FAIL;


	CColider::COLLIDERDESC ColliderDesc;
	ZeroMemory(&ColliderDesc, sizeof(CColider::COLLIDERDESC));
	ColliderDesc.vScale = _float3(1., 1.f, 1.f);
	ColliderDesc.vRotation = _float3(0.f, 0.f, 0.f);
	ColliderDesc.vPosition = _float3(0.f, 80.f, 0.f);
	if (FAILED(__super::Add_Components(TEXT("Com_SPHERE"), LEVEL_STATIC, TEXT("Prototype_Component_Colider_Sphere"), (CComponent**)&m_pSphereCom, &ColliderDesc)))
		return E_FAIL;
	return S_OK;
}

HRESULT CRenekton::SetUp_ShaderID()
{
	m_eShaderID = SHADER_ANIMDEFAULT;

	return S_OK;
}

CRenekton* CRenekton::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CRenekton* pInstance = new CRenekton(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CVayne");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CRenekton::Clone(void* pArg)
{
	CRenekton* pInstance = new CRenekton(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CVayne");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CRenekton::Free()
{
	__super::Free();
	Safe_Delete(m_pState);
}
