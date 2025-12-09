#include "Npc.h"
#include "GameInstance.h"
#include "PickingManager.h"
#include "CursorManager.h"
#include "PlayerManager.h"
#include "Collision_Manager.h"
CNpc::CNpc(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CActor(pDevice, pContext)
{
}

CNpc::CNpc(const CNpc& rhs)
	: CActor(rhs)
{
}

HRESULT CNpc::Initialize_Prototype()
{
	if (FAILED(__super::Initialize_Prototype()))
		return E_FAIL;

	return S_OK;
}

HRESULT CNpc::Initialize(void* pArg)
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
	CCollision_Manager::GetInstance()->Add_CollisionGroup(CCollision_Manager::COLLISION_NPC, this);

	return S_OK;
}

_int CNpc::Update(_float fTimeDelta)
{
	if (m_bDead)
		return OBJ_DEAD;

	__super::Update(fTimeDelta);
	Update_Collider();

	CPickingManager::GetInstance()->Out_PickingGroup(this);
	return OBJ_NOEVENT;
}

void CNpc::Late_Update(_float fTimeDelta)
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

HRESULT CNpc::Render()
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

HRESULT CNpc::Render_ShadowDepth()
{
	return S_OK;
}

HRESULT CNpc::Render_EdgeDetection()
{
	return S_OK;
}

_bool CNpc::Picking(_float3* PickingPoint)
{
	CColider* pCollider = Get_Collider();
	if (pCollider == nullptr)
		return false;

	float   fDist = 0.f;
	_float3 vHitPos = {};

	// Collider Ray Picking
	if (pCollider->RayPick(m_pTransformCom, &vHitPos, &fDist))
	{
		if (PickingPoint)
			*PickingPoint = vHitPos;
		return true;
	}

	ZeroMemory(&m_vMousePickedPos, sizeof(_float3));
	return false;
}

void CNpc::PickingTrue()
{
}

void CNpc::Set_Picked()
{
}

HRESULT CNpc::SetUp_ShaderResources()
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

void CNpc::Free()
{
	__super::Free();
	CCollision_Manager::GetInstance()->Out_CollisionGroup(CCollision_Manager::COLLISION_NPC, this);
}

