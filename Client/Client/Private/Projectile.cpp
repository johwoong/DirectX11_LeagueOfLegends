#include "Projectile.h"
#include "PoolManager.h"
CProjectile::CProjectile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CActor(pDevice, pContext)
{
}

CProjectile::CProjectile(const CProjectile& rhs)
	: CActor(rhs)
{
}

HRESULT CProjectile::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CProjectile::Initialize(void* pArg)
{
	if (!pArg) return E_FAIL;

	memcpy(&m_ProjectileDesc, pArg, sizeof(PROJECTILEDESC));
	m_ePoolLevel = m_ProjectileDesc.ePoolLevel;
	m_pPoolLayerTag = m_ProjectileDesc.pPoolLayerTag;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	if (FAILED(Ready_Components(pArg)))
		return E_FAIL;

	// ? 프리워밍은 "비활성 상태"로만 만들어서 풀에 넣는 용도
	if (m_ProjectileDesc.bPrewarm)
	{
		m_bDead = false;
		m_bDeadEffect = false;
		m_bInPool = true;              // 중요: Late_Update에서 렌더/충돌/타이머 금지
		m_fTime = 0.f;
		m_bCollisionRegistered = false; // 충돌그룹 등록 안 했으니 false

		m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, m_ProjectileDesc.vInitPositon);
		return S_OK;
	}

	// ? 실사용(발사된 탄)만 충돌그룹 등록
	if (m_ProjectileDesc.eCollisionGroup == PLAYER)
		CCollision_Manager::GetInstance()->Add_CollisionGroup(CCollision_Manager::COLLISION_PLAYER, this);
	else if (m_ProjectileDesc.eCollisionGroup == ENEMY)
		CCollision_Manager::GetInstance()->Add_CollisionGroup(CCollision_Manager::COLLISION_ENEMY, this);
	else if (m_ProjectileDesc.eCollisionGroup == MINION)
		CCollision_Manager::GetInstance()->Add_CollisionGroup(CCollision_Manager::COLLISION_MINION, this);
	else if (m_ProjectileDesc.eCollisionGroup == BUILD)
		CCollision_Manager::GetInstance()->Add_CollisionGroup(CCollision_Manager::COLLISION_BUILDING, this);

	m_bCollisionRegistered = true;

	m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, m_ProjectileDesc.vInitPositon);
	return S_OK;
}

int CProjectile::Update(_float fTimeDelta)
{
	// ? 풀 상태면 씬에서 제거 대상(레이어에서 erase만!)
	if (m_bInPool)
		return OBJ_POOL;

	if (m_bDead || m_bDeadEffect)
	{
		if (!m_bInPool)
		{
			m_bInPool = true;
			On_ReturnToPool();
		}
		return OBJ_POOL;
	}
	return OBJ_NOEVENT;
}

void CProjectile::Late_Update(_float fTimeDelta)
{
	if (m_bInPool)
		return; // ★ 풀에 들어간 놈은 렌더/충돌/타이머 다 금지

	if (m_pRendererCom)
	{
		m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_NOALPHABLEND, this);
		__super::Late_Update(fTimeDelta);
	}

	m_fTime += CGameInstance::GetInstance()->Get_TimeDelta(TEXT("Timer_60"));
	if (m_fTime >= m_ProjectileDesc.fDeadTime)
		m_bDeadEffect = true;

	if (!m_bDeadEffect && !m_bDead)
		Collision_Check();
}

void CProjectile::ReUse_Setting(void* pArg)
{
	if (!pArg) return;

	memcpy(&m_ProjectileDesc, pArg, sizeof(PROJECTILEDESC));
	m_ProjectileDesc.bPrewarm = false; // ? 재사용 들어오면 프리워밍 해제

	m_ePoolLevel = m_ProjectileDesc.ePoolLevel;
	m_pPoolLayerTag = m_ProjectileDesc.pPoolLayerTag;

	m_bDead = false;
	m_bDeadEffect = false;
	m_bInPool = false;
	m_fTime = 0.f;

	m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, m_ProjectileDesc.vInitPositon);

	// ? 충돌그룹 재등록 (중복 방지)
	if (!m_bCollisionRegistered)
	{
		if (m_ProjectileDesc.eCollisionGroup == PLAYER)
			CCollision_Manager::GetInstance()->Add_CollisionGroup(CCollision_Manager::COLLISION_PLAYER, this);
		else if (m_ProjectileDesc.eCollisionGroup == ENEMY)
			CCollision_Manager::GetInstance()->Add_CollisionGroup(CCollision_Manager::COLLISION_ENEMY, this);
		else if (m_ProjectileDesc.eCollisionGroup == MINION)
			CCollision_Manager::GetInstance()->Add_CollisionGroup(CCollision_Manager::COLLISION_MINION, this);
		else if (m_ProjectileDesc.eCollisionGroup == BUILD)
			CCollision_Manager::GetInstance()->Add_CollisionGroup(CCollision_Manager::COLLISION_BUILDING, this);

		m_bCollisionRegistered = true;
	}

	if (m_pOBBCom) m_pOBBCom->Update(m_pTransformCom->Get_WorldMatrix());
}

HRESULT CProjectile::Render()
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


HRESULT CProjectile::Ready_Components(void* pArg)
{
	/* For.Com_Renderer */
	if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRendererCom)))
		return E_FAIL;

	/* For.Com_Transform */
	CTransform::TRANSFORMDESC TransformDesc;
	ZeroMemory(&TransformDesc, sizeof(CTransform::TRANSFORMDESC));

	TransformDesc.fSpeedPerSec = 5.f * m_ProjectileDesc.fVelocity;
	TransformDesc.fRotationPerSec = XMConvertToRadians(90.0f);

	if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC, TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom, &TransformDesc)))
		return E_FAIL;

	/* For.Com_Shader */
	if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxModel"), (CComponent**)&m_pShaderCom)))
		return E_FAIL;

	return S_OK;
}

HRESULT CProjectile::SetUp_ShaderResources()
{
	if (!m_pShaderCom)
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

HRESULT CProjectile::SetUp_ShaderID()
{
	return S_OK;
}

void CProjectile::Collision_Check()
{
	CActor* pCollisionTarget = nullptr;

	if (m_ProjectileDesc.eCollisionGroup == PLAYER)
	{
		if (CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_ENEMY, m_pOBBCom, &pCollisionTarget))
		{
			dynamic_cast<CEnemy*>(pCollisionTarget)->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);
			m_bDead = true;
		}
		else if (CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_MINION, m_pOBBCom, &pCollisionTarget))
		{
			dynamic_cast<CMinion*>(pCollisionTarget)->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);
			m_bDead = true;
		}
		else if (CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_BUILDING, m_pOBBCom, &pCollisionTarget))
		{
			dynamic_cast<CBuilding*>(pCollisionTarget)->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);
			m_bDead = true;
		}
	}
	else if(m_ProjectileDesc.eCollisionGroup == ENEMY)
	{
		if (CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_PLAYER, m_pOBBCom, &pCollisionTarget))
		{
			dynamic_cast<CPlayer*>(pCollisionTarget)->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);
			m_bDead = true;
		}
		else if (CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_MINION, m_pOBBCom, &pCollisionTarget))
		{
			dynamic_cast<CMinion*>(pCollisionTarget)->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);
			m_bDead = true;
		}
		else if (CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_BUILDING, m_pOBBCom, &pCollisionTarget))
		{
			dynamic_cast<CBuilding*>(pCollisionTarget)->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);
			m_bDead = true;
		}
	}
}


void CProjectile::On_ReturnToPool()
{
	// 1) 충돌그룹 Out (등록된 경우만)
	if (m_bCollisionRegistered)
	{
		if (m_ProjectileDesc.eCollisionGroup == PLAYER)
			CCollision_Manager::GetInstance()->Out_CollisionGroup(CCollision_Manager::COLLISION_PLAYER, this);
		else if (m_ProjectileDesc.eCollisionGroup == ENEMY)
			CCollision_Manager::GetInstance()->Out_CollisionGroup(CCollision_Manager::COLLISION_ENEMY, this);
		else if (m_ProjectileDesc.eCollisionGroup == MINION)
			CCollision_Manager::GetInstance()->Out_CollisionGroup(CCollision_Manager::COLLISION_MINION, this);
		else if (m_ProjectileDesc.eCollisionGroup == BUILD)
			CCollision_Manager::GetInstance()->Out_CollisionGroup(CCollision_Manager::COLLISION_BUILDING, this);

		m_bCollisionRegistered = false;
	}

	// 2) FX 정리
	for (auto& fx : m_pEffects)
	{
		if (!fx) continue;
		fx->Set_AttachObject(nullptr);
		if (auto ps = dynamic_cast<CParticleSystem*>(fx)) ps->Set_Stop(true);
		fx->Set_Dead(true);
	}
	m_pEffects.clear();

	for (auto& fx : m_pDeadEffects)
	{
		if (!fx) continue;
		fx->Set_AttachObject(nullptr);
		if (auto ps = dynamic_cast<CParticleSystem*>(fx)) ps->Set_Stop(true);
		fx->Set_Dead(true);
	}
	m_pDeadEffects.clear();

	m_ProjectileDesc.pTarget = nullptr;
	m_ProjectileDesc.pOwner = nullptr;

	if (m_pPoolLayerTag)
		CPoolManager::GetInstance()->Add_Pooling_Object(m_ePoolLevel, m_pPoolLayerTag, this);
}

void CProjectile::Free()
{
	__super::Free();

	// ? 여기서도 Out은 등록된 경우만
	if (m_bCollisionRegistered)
	{
		if (m_ProjectileDesc.eCollisionGroup == PLAYER)
			CCollision_Manager::GetInstance()->Out_CollisionGroup(CCollision_Manager::COLLISION_PLAYER, this);
		else if (m_ProjectileDesc.eCollisionGroup == ENEMY)
			CCollision_Manager::GetInstance()->Out_CollisionGroup(CCollision_Manager::COLLISION_ENEMY, this);
		else if (m_ProjectileDesc.eCollisionGroup == MINION)
			CCollision_Manager::GetInstance()->Out_CollisionGroup(CCollision_Manager::COLLISION_MINION, this);
		else if (m_ProjectileDesc.eCollisionGroup == BUILD)
			CCollision_Manager::GetInstance()->Out_CollisionGroup(CCollision_Manager::COLLISION_BUILDING, this);

		m_bCollisionRegistered = false;
	}
}
