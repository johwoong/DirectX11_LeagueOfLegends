
#include "EzrealProjectile.h"
#include "Enemy.h"
#include "ParticleSystem.h"

CEzrealProjectile::CEzrealProjectile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CProjectile(pDevice, pContext)
{
}

CEzrealProjectile::CEzrealProjectile(const CEzrealProjectile& rhs)
	: CProjectile(rhs)
{
}

HRESULT CEzrealProjectile::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CEzrealProjectile::Initialize(void* pArg)
{
	__super::Initialize(pArg);

	_matrix mWorldMatrix = XMMatrixIdentity();
	_vector vOffset = { 0.f, 0.f, 0.f, 0.f };
	_vector vLocation = { 0.f, 0.f, 0.f, 0.f };
	_vector vDir = { 0.f, 0.f, 0.f, 0.f };



	switch (m_ProjectileDesc.eProjType)
	{
	case BASE:
	{
		mWorldMatrix = m_ProjectileDesc.pOwner->Get_Transform()->Get_WorldMatrix();
		CEffect::PlayEffectAtLocation(TEXT("Ezreal_Base.dat"), mWorldMatrix);
		for (auto& pEffect : m_pEffects)
		{
			if (!pEffect)
				continue;

				_float4 vPosition;
				XMStoreFloat4(&vPosition, pEffect->Get_TransformState(CTransform::STATE::STATE_TRANSLATION));

				mWorldMatrix.r[3] = XMLoadFloat4(&vPosition);
				break;
			
		}

		break;
	}
	}

	return S_OK;
}

int CEzrealProjectile::Update(_float fTimeDelta)
{
	_int iSuperTick = __super::Update(fTimeDelta);

	if (iSuperTick == OBJ_DEAD)
	{
		Dead_Effect();
		return OBJ_DEAD;
	}

	if (iSuperTick == OBJ_NOSHOW)
		return OBJ_NOSHOW;

	switch (m_ProjectileDesc.eProjType)
	{
	case BASE:
	{
		Update_Projectile(fTimeDelta);
		break;
	}
	}

	if (m_pAABBCom != nullptr)
		m_pAABBCom->Update(m_pTransformCom->Get_WorldMatrix());
	if (m_pOBBCom != nullptr)
		m_pOBBCom->Update(m_pTransformCom->Get_WorldMatrix());
	if (m_pSphereCom != nullptr)
		m_pSphereCom->Update(m_pTransformCom->Get_WorldMatrix());

	return OBJ_NOEVENT;
}

void CEzrealProjectile::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);

	for (auto& iter : m_pEffects)
	{
		if (iter != nullptr && iter->Get_PreDead())
			iter = nullptr;
	}

	switch (m_ProjectileDesc.eProjType)
	{
	case BASE:
	{
		if (m_fTime >= m_ProjectileDesc.fDeadTime)
			m_bDead = true;
		break;
	}
	}
}

void CEzrealProjectile::Collision_Check()
{
	CActor* pCollisionTarget = nullptr;
	switch (m_ProjectileDesc.eProjType)
	{
	case BASE:
		if (CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_ENEMY, m_pOBBCom, &pCollisionTarget))
			dynamic_cast<CEnemy*>(pCollisionTarget)->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);
		else if (m_ProjectileDesc.eCollisionGroup == ENEMY)
			CCollision_Manager::GetInstance()->Add_CollisionGroup(CCollision_Manager::COLLISION_ENEMY, this);
		else if (m_ProjectileDesc.eCollisionGroup == MINION)
			CCollision_Manager::GetInstance()->Add_CollisionGroup(CCollision_Manager::COLLISION_MINION, this);
		else if (m_ProjectileDesc.eCollisionGroup == BUILD)
			CCollision_Manager::GetInstance()->Add_CollisionGroup(CCollision_Manager::COLLISION_BUILDING, this);
		break;
	}
}

void CEzrealProjectile::Dead_Effect()
{
	for (auto& iter : m_pEffects)
	{
		if (iter != nullptr && iter->Get_PreDead())
			iter = nullptr;
	}

	switch (m_ProjectileDesc.eProjType)
	{
	case Q:
	{
		if (!m_pEffects.empty())
		{
			for (auto& iter : m_pEffects)
			{
				if (iter != nullptr)
				{
					CParticleSystem* pParticleSystem = dynamic_cast<CParticleSystem*>(iter);
					if (pParticleSystem != nullptr)
						pParticleSystem->Set_Stop(true);
				}
			}
		}
		m_pEffects.clear();
		break;
	}
	}
}

HRESULT CEzrealProjectile::Ready_Components(void* pArg)
{
	if (FAILED(__super::Ready_Components(pArg)))
		return E_FAIL;

	/* For.Com_SPHERE */
	CColider::COLLIDERDESC ColliderDesc;
	ZeroMemory(&ColliderDesc, sizeof(CColider::COLLIDERDESC));

	switch (m_ProjectileDesc.eProjType)
	{
	case BASE:
	{
		ColliderDesc.vScale = _float3(1.f, 1.f, 1.f);
		ColliderDesc.vRotation = _float3(0.f, 0.f, 0.f);
		ColliderDesc.vPosition = _float3(0.f, 0.f, 0.f);

		if (FAILED(__super::Add_Components(TEXT("Com_OBB"), LEVEL_STATIC, TEXT("Prototype_Component_Colider_OBB"), (CComponent**)&m_pOBBCom, &ColliderDesc)))
			return E_FAIL;

		break;
	}
	}

	return S_OK;
}
void CEzrealProjectile::Update_Projectile(_float fTimeDelta)
{
	m_fTime += fTimeDelta;

	for (auto& iter : m_pEffects)
	{
		if (iter != nullptr && iter->Get_PreDead())
			iter = nullptr;
	}
}

CEzrealProjectile* CEzrealProjectile::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CEzrealProjectile* pInstance = new CEzrealProjectile(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Create: CEzrealProjectile");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CEzrealProjectile::Clone(void* pArg)
{
	CEzrealProjectile* pInstance = new CEzrealProjectile(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone: CEzrealProjectile");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CEzrealProjectile::Free()
{
	__super::Free();
}