#include "Enemy.h"
#include "Player.h"
#include "PickingManager.h"
#include "PlayerManager.h"
#include "Champion.h"
#include "CursorManager.h"
#include "Collision_Manager.h"
#include "UI_Enemy_HpBar.h"


CEnemy::CEnemy(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CActor(pDevice, pContext)
{
}

CEnemy::CEnemy(const CEnemy& rhs)
	: CActor(rhs)
{
}

HRESULT CEnemy::Initialize_Prototype()
{
	if (FAILED(__super::Initialize_Prototype()))
		return E_FAIL;

	return S_OK;
}

HRESULT CEnemy::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	if (FAILED(Ready_Components(pArg)))
		return E_FAIL;

	NONANIMDESC ModelDesc;
	if (pArg)
		memcpy(&ModelDesc, pArg, sizeof(NONANIMDESC));

	if (pArg)
	{
		_vector vPosition = XMLoadFloat3(&ModelDesc.vPosition);
		vPosition = XMVectorSetW(vPosition, 1.f);
		m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vPosition);
		Set_Scale(ModelDesc.vScale);

		if (ModelDesc.m_fAngle != 0)
			m_pTransformCom->Rotation(XMVectorSet(0.f, 1.f, 0.f, 0.f), XMConvertToRadians(ModelDesc.m_fAngle));
	}
	_vector pos = Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
	float h = m_pNavigationCom->Compute_Height(pos, 0);
	pos = XMVectorSetY(pos, h);
	Get_Transform()->Set_State(CTransform::STATE_TRANSLATION, pos);
	CCollision_Manager::GetInstance()->Add_CollisionGroup(CCollision_Manager::COLLISION_ENEMY, this);
	m_pNavigationCom->Compute_CurrentIndex_byXZ(Get_TransformState(CTransform::STATE_TRANSLATION));
	Check_Navigation();

	CGameObject* pObj = nullptr;

	if (FAILED(m_pGameInstance->Add_GameObject_Out(
		TEXT("Prototype_GameObject_EnemyHpBar"),
		LEVEL_GAMEPLAY,
		TEXT("Layer_UI_HpBar"),
		pObj,
		this)))
	{
		return E_FAIL;
	}

	m_pHpBar = dynamic_cast<CEnemy_HpBar*>(pObj);

	if (m_pHpBar)
		m_pHpBar->Set_Owner(this);

	return S_OK;
}

int CEnemy::Update(_float fTimeDelta)
{
	if (m_bDead)
		return OBJ_DEAD;

	__super::Update(fTimeDelta);
	Update_Collider();
	
	CPickingManager::GetInstance()->Out_PickingGroup(this);
	return OBJ_NOEVENT;
}

void CEnemy::Late_Update(_float fTimeDelta)
{
	if (m_bDead)
		return;

	__super::Late_Update(fTimeDelta);

	if (m_pRendererCom)
	{
		m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_NOALPHABLEND, this);
	}

	CPickingManager::GetInstance()->Add_PickingGroup(this);
}

HRESULT CEnemy::Render()
{
	if (!m_pShaderCom || !m_pModelCom)
		return E_FAIL;

	if (FAILED(SetUp_ShaderResources()))
		return E_FAIL;
	if (FAILED(SetUp_ShaderID()))
		return E_FAIL;

	_uint iNumMeshes = m_pModelCom->Get_NumMeshs();
	for (_uint i = 0; i < iNumMeshes; ++i)
	{
		if (FAILED(m_pModelCom->SetUp_Material(m_pShaderCom, "g_DiffuseTexture", i, aiTextureType_DIFFUSE)))
			return E_FAIL;
		if (FAILED(m_pModelCom->SetUp_Material(m_pShaderCom, "g_NormalTexture", i, aiTextureType_NORMALS)))
			return E_FAIL;

		if (FAILED(m_pModelCom->Render(m_pShaderCom, i, m_eShaderID)))
			return E_FAIL;
	}

	return S_OK;
}

HRESULT CEnemy::Render_ShadowDepth()
{
	return S_OK;
}

HRESULT CEnemy::Render_EdgeDetection()
{
	return S_OK;
}

_bool CEnemy::Picking(_float3* PickingPoint)
{
	if (m_pModelCom->Picking(m_pTransformCom, PickingPoint) == true)
	{
		CCursorManager::GetInstance()->SetState(CURSOR_ATTACK);
		CChampion* pPlayer = CPlayerManager::GetInstance()->Get_ActivePlayer();
		pPlayer->Set_AttackTarget(this);
		return true;
	}
	else
	{
		ZeroMemory(&m_vMousePickedPos, sizeof(_float3));
	}
	return false;
}

void CEnemy::PickingTrue()
{
}

void CEnemy::Set_Picked()
{

}

void CEnemy::Find_Target()
{

}

CActor* CEnemy::Find_MinDistance_Target()
{
	return nullptr;
}

CActor* CEnemy::Check_FiledTarget()
{
	return nullptr;
}

void CEnemy::Compute_CurrentIndex()
{
	m_pNavigationCom->Compute_CurrentIndex_byXZ(m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION));
}

HRESULT CEnemy::SetUp_ShaderResources()
{
	if (!m_pShaderCom || !m_pTransformCom)
		return E_FAIL;

	_float4x4 w = m_pTransformCom->Get_World_TP();
	_float4x4 v = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_VIEW);
	_float4x4 p = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_PROJ);

	if (FAILED(m_pShaderCom->Set_RawValue("g_WorldMatrix", &w, sizeof(_float4x4)))) return E_FAIL;
	if (FAILED(m_pShaderCom->Set_RawValue("g_ViewMatrix", &v, sizeof(_float4x4)))) return E_FAIL;
	if (FAILED(m_pShaderCom->Set_RawValue("g_ProjMatrix", &p, sizeof(_float4x4)))) return E_FAIL;

	return S_OK;
}

HRESULT CEnemy::SetUp_ShaderID()
{
	return S_OK;
}

_int CEnemy::Take_Damage(int iDamage, CActor* DamageCauser)
{
	m_tStats.Hp -= iDamage;
	if (m_tStats.Hp < 0.f)
		m_tStats.Hp = 0.f;

	if (m_tStats.Hp <= 0.f)
	{
		Mark_Dead();
	}

	return (_int)m_tStats.Hp;
}

void CEnemy::Mark_Dead()
{
	if (m_bDead)
		return;

	m_bDead = true;

	// 0) 이 적을 타겟팅하고 있는 플레이어 타겟 해제
	CPlayerManager* pPM = CPlayerManager::GetInstance();
	if (pPM)
	{
		// 현재는 1챔프만 있다고 치고
		CChampion* pChampion = pPM->Get_ActivePlayer();
		if (pChampion && pChampion->Get_AttackTarget() == this)
		{
			pChampion->Set_AttackTarget(nullptr);
		}
	}

	// 1) 충돌 그룹에서 적 제거
	CCollision_Manager::GetInstance()->Out_CollisionGroup(CCollision_Manager::COLLISION_ENEMY, this);
	CPickingManager::GetInstance()->Out_PickingGroup(this);
}


void CEnemy::Free()
{
	__super::Free();
	CCollision_Manager::GetInstance()->Out_CollisionGroup(CCollision_Manager::COLLISION_ENEMY, this);
}
