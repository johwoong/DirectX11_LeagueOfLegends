#include "Building.h"
#include "GameInstance.h"
#include "PickingManager.h"
#include "CursorManager.h"
#include "PlayerManager.h"
#include "Champion.h"
#include "UI_Building_HpBar.h"
#include "Collision_Manager.h"
CBuilding::CBuilding(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CActor(pDevice, pContext)
{
}

CBuilding::CBuilding(const CBuilding& rhs)
	: CActor(rhs)
{
}

HRESULT CBuilding::Initialize_Prototype()
{
	if(FAILED(__super::Initialize_Prototype()))
		return E_FAIL;

	return S_OK;
}

HRESULT CBuilding::Initialize(void* pArg)
{
	if (pArg != nullptr)
		memcpy(&m_BuildDesc, pArg, sizeof(BUILDINGDESC));

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	if (FAILED(Ready_Components(pArg)))
		return E_FAIL;

	if (pArg != nullptr)
	{
		m_pTransformCom->Set_WorldMatrix(XMLoadFloat4x4(&m_BuildDesc.ModelDesc.WorldMatrix));
		_vector vPosition = XMLoadFloat3(&m_BuildDesc.ModelDesc.vPosition);
		vPosition = XMVectorSetW(vPosition, 1.f);
		m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vPosition);
		m_pTransformCom->Set_Rotation(m_BuildDesc.ModelDesc.vRotation);
		Set_Scale(m_BuildDesc.ModelDesc.vScale);
	}
	CCollision_Manager::GetInstance()->Add_CollisionGroup(CCollision_Manager::COLLISION_BUILDING, this);
	// HpBar 생성
	CGameObject* pObj = nullptr;
	if (FAILED(m_pGameInstance->Add_GameObject_Out(
		TEXT("Prototype_GameObject_BuildingHpBar"),
		LEVEL_GAMEPLAY,
		TEXT("Layer_UI_HpBar"),
		pObj,
		this)))
	{
		return E_FAIL;
	}

	m_pHpBar = dynamic_cast<CBuilding_HpBar*>(pObj);
	if (m_pHpBar)
		m_pHpBar->Set_Owner(this); // ★ 여기서 Owner 설정 → 그 순간 타입별 크기 적용

	return S_OK;
}

_int CBuilding::Update(_float fTimeDelta)
{
	if (m_bDead)
		return OBJ_DEAD;

	__super::Update(fTimeDelta);
	Update_Collider();

	CPickingManager::GetInstance()->Out_PickingGroup(this);
	return OBJ_NOEVENT;
}

void CBuilding::Late_Update(_float fTimeDelta)
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

HRESULT CBuilding::Render()
{
	if (nullptr == m_pShaderCom || m_pModelCom == nullptr)
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

HRESULT CBuilding::Render_ShadowDepth()
{
	return S_OK;
}

HRESULT CBuilding::Render_EdgeDetection()
{
	return S_OK;
}

_bool CBuilding::Picking(_float3* PickingPoint)
{
	CColider* pCollider = Get_Collider();
	if (pCollider == nullptr)
		return false;

	float   fDist = 0.f;
	_float3 vHitPos = {};

	// Collider Ray Picking
	if (pCollider->RayPick(m_pTransformCom, &vHitPos, &fDist))
	{
		CChampion* pPlayer = CPlayerManager::GetInstance()->Get_ActivePlayer();
		if (!pPlayer)
			return false;

		// ?? 공격 커서 + 타겟 설정
		CCursorManager::GetInstance()->SetState(CURSOR_ATTACK);
		pPlayer->Set_AttackTarget(this);

		if (PickingPoint)
			*PickingPoint = vHitPos;

		return true;
	}

	ZeroMemory(&m_vMousePickedPos, sizeof(_float3));
	return false;
}

void CBuilding::PickingTrue()
{
}

void CBuilding::Set_Picked()
{
}

_int CBuilding::Take_Damage(int iDamage, CActor* DamageCauser)
{
	// 1) HP 감소
	m_Stat.m_fCurrentHp -= (_float)iDamage;
	if (m_Stat.m_fCurrentHp < 0.f)
		m_Stat.m_fCurrentHp = 0.f;

	// 2) HP 0 이하 → 죽음 처리
	if (m_Stat.m_fCurrentHp <= 0.f)
	{
		Mark_Dead();
	}

	return (_int)m_Stat.m_fCurrentHp;
}

void CBuilding::Mark_Dead()
{

	if (m_bDead)
		return;

	m_bDead = true;

	CPickingManager::GetInstance()->Out_PickingGroup(this);

	CCollision_Manager::GetInstance()->Out_CollisionGroup(CCollision_Manager::COLLISION_BUILDING, this);

}

HRESULT CBuilding::SetUp_ShaderResources()
{
	if (!m_pShaderCom || !m_pTransformCom)
		return E_FAIL;

	_float4x4 WorldMatrix = m_pTransformCom->Get_World_TP();
	_float4x4 ViewMatrix = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_VIEW);
	_float4x4 ProjMatrix = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_PROJ);


	if (FAILED(m_pShaderCom->Set_RawValue("g_WorldMatrix", &WorldMatrix, sizeof(_float4x4))))
		return E_FAIL;

	if (FAILED(m_pShaderCom->Set_RawValue("g_ViewMatrix", &ViewMatrix, sizeof(_float4x4))))
		return E_FAIL;

	if (FAILED(m_pShaderCom->Set_RawValue("g_ProjMatrix", &ProjMatrix, sizeof(_float4x4))))
		return E_FAIL;

	return S_OK;
}

void CBuilding::Free()
{
	__super::Free();
	CCollision_Manager::GetInstance()->Out_CollisionGroup(CCollision_Manager::COLLISION_BUILDING, this);
}

