#include "Collision_Manager.h"
#include "Actor.h"
#include "GameInstance.h"

IMPLEMENT_SINGLETON(CCollision_Manager)


CCollision_Manager::CCollision_Manager()
{

}

void CCollision_Manager::Add_CollisionGroup(COLLISION_GROUP CollisionGroup, CActor* pGameObject)
{
	if (nullptr == pGameObject)
		return;

	auto iter = m_GameObjects[CollisionGroup].begin();
	while (iter != m_GameObjects[CollisionGroup].end())
	{
		if (*iter == pGameObject)
			return;
		else
			++iter;
	}

	m_GameObjects[CollisionGroup].push_back(pGameObject);

}

void CCollision_Manager::Add_CollisionGroupCollider(COLLISION_GROUP CollisionGroup, CColider* pCollider, CActor* pGameObject)
{
	if (nullptr == pCollider || nullptr == pGameObject)
		return;

	m_Colliders[CollisionGroup].push_back(pCollider);
	m_ColliderGameObjects[CollisionGroup].push_back(pGameObject);
}

void CCollision_Manager::Out_CollisionGroup(COLLISION_GROUP CollisionGroup, CActor* pGameObject)
{
	auto iter = m_GameObjects[CollisionGroup].begin();
	while (iter != m_GameObjects[CollisionGroup].end())
	{
		if (*iter == pGameObject)
			iter = m_GameObjects[CollisionGroup].erase(iter);
		else
			++iter;
	}
}

void CCollision_Manager::Out_CollisionGroupCollider(COLLISION_GROUP CollisionGroup, CColider* pCollider, CActor* pGameObject)
{
	auto iterObj = m_ColliderGameObjects[CollisionGroup].begin();
	auto iterCol = m_Colliders[CollisionGroup].begin();

	while (iterObj != m_ColliderGameObjects[CollisionGroup].end())
	{
		if (*iterObj == pGameObject)
			iterObj = m_ColliderGameObjects[CollisionGroup].erase(iterObj);
		else
			++iterObj;
	}
	while (iterCol != m_Colliders[CollisionGroup].end())
	{
		if (*iterCol == pCollider)
			iterCol = m_Colliders[CollisionGroup].erase(iterCol);
		else
			++iterCol;
	}
}

void CCollision_Manager::Clear_CollisionGroup(COLLISION_GROUP CollisionGroup)
{
	m_GameObjects[CollisionGroup].clear();
}

void CCollision_Manager::Clear_CollisionGroupExpect(COLLISION_GROUP CollisionGroup)
{
	for (_uint i = 0; i < COLLISION_END; ++i)
	{
		if (i == CollisionGroup)
			continue;
		m_GameObjects[i].clear();
	}
}

void CCollision_Manager::Clear_AllCollisionGroup()
{
	for (_uint i = 0; i < COLLISION_END; ++i)
	{
		if (i == COLLISION_PLAYER)
			continue;

		m_GameObjects[i].clear();
		m_ColliderGameObjects[i].clear();
		m_Colliders[i].clear();
	}
}

_bool CCollision_Manager::CollisionwithGroup(COLLISION_GROUP CollisionGroup, CColider* pCollider, CActor** pOut, COLLIDER_TYPE eType)
{
	for (auto& iter : m_GameObjects[CollisionGroup])
	{
		if (iter == nullptr || iter->Get_Collider() == pCollider)
			continue;

		CColider* pTargetCollider = nullptr;
		
		switch (eType)
		{
		case Client::CCollision_Manager::COLLIDER_AABB:
			pTargetCollider = iter->Get_AABBCollider();
			break;
		case Client::CCollision_Manager::COLLIDER_OBB:
			pTargetCollider = iter->Get_OBBCollider();
			break;
		case Client::CCollision_Manager::COLLIDER_SPHERE:
			pTargetCollider = iter->Get_SphereCollider();
			break;
		case Client::CCollision_Manager::COLLIDER_END:
			pTargetCollider = iter->Get_Collider();
			break;
		}

		if (pCollider->Collision(pTargetCollider))
		{
			if (pOut != nullptr)
				*pOut = iter;
			return true;
		}
	}
	return false;
}

_bool CCollision_Manager::CollisionwithGroupCollider(COLLISION_GROUP CollisionGroup, CColider* pCollider, CActor** pOut, COLLIDER_TYPE eType)
{
	if (0 >= m_ColliderGameObjects[CollisionGroup].size() || 0 >= m_Colliders[CollisionGroup].size())
		return false;

	auto iterObj = m_ColliderGameObjects[CollisionGroup].begin();

	for (auto& iterCol : m_Colliders[CollisionGroup])
	{
		if (nullptr == iterCol)
		{
			++iterObj;
			continue;
		}

		if (pCollider->Collision(iterCol))
		{
			if (nullptr != pOut)
				*pOut = *iterObj;

			return true;
		}

		if (iterObj != m_ColliderGameObjects[CollisionGroup].end())
			++iterObj;
	}

	return false;
}

_bool CCollision_Manager::CollisionwithGroup_All(COLLISION_GROUP CollisionGroup, CColider* pCollider, std::vector<CActor*>* pOut, COLLIDER_TYPE eType)
{
	if (!pCollider || !pOut)
		return false;

	pOut->clear();
	_bool bHit = false;

	for (auto& iter : m_GameObjects[CollisionGroup])
	{
		if (iter == nullptr || iter->Get_Collider() == pCollider)
			continue;

		CColider* pTargetCollider = nullptr;

		switch (eType)
		{
		case Client::CCollision_Manager::COLLIDER_AABB:
			pTargetCollider = iter->Get_AABBCollider();
			break;
		case Client::CCollision_Manager::COLLIDER_OBB:
			pTargetCollider = iter->Get_OBBCollider();
			break;
		case Client::CCollision_Manager::COLLIDER_SPHERE:
			pTargetCollider = iter->Get_SphereCollider();
			break;
		case Client::CCollision_Manager::COLLIDER_END:
			pTargetCollider = iter->Get_Collider();
			break;
		}

		if (nullptr == pTargetCollider)
			continue;

		if (pCollider->Collision(pTargetCollider))
		{
			pOut->push_back(iter);
			bHit = true;   // ?? 여기서는 return 안 하고 계속 탐색
		}
	}

	return bHit; // 하나라도 맞았는지만 리턴
}

_bool CCollision_Manager::CollisionwithGroup(COLLISION_GROUP SourGroup, COLLISION_GROUP DestGroup, CActor** outSour, CActor** outDest)
{
	for (auto& Sour : m_GameObjects[SourGroup])
	{
		CColider* pSourCollider = Sour->Get_Collider();
		if (pSourCollider == nullptr)
			continue;

		for (auto& Dest : m_GameObjects[DestGroup])
		{
			CColider* pDestCollider = Dest->Get_Collider();
			if (pDestCollider == nullptr || Dest == Sour)
				continue;



			if (pDestCollider->Collision(pSourCollider))
			{
				*outSour = Sour;
				*outDest = Dest;
				return true;
			}
		}
	}

	*outSour = nullptr;
	*outDest = nullptr;
	return false;
}

void CCollision_Manager::Update_Collider()
{
	for (_uint i = 0; i < COLLISION_END; ++i)
	{
		for (auto& iter : m_GameObjects[i])
		{
			iter->Update_Collider();
		}
	}
}


CColider* CCollision_Manager::Reuse_Collider(CColider::TYPE eType, _uint iLevelIdx, const _tchar* pPrototypeTag, void* pArg)
{
	CColider* pCollider = nullptr;
	
	if (m_ColliderPool[eType].empty())
	{
		CGameInstance* pGameInstance = CGameInstance::GetInstance();
		pGameInstance->AddRef();
		
		pCollider = (CColider*)pGameInstance->Clone_Component(iLevelIdx, pPrototypeTag, pArg);
		pGameInstance->Release();
	}
	else
	{
		pCollider = m_ColliderPool[eType].front();
		m_ColliderPool[eType].pop_front();
		pCollider->Set_Colider_Desc(pArg);
	}

	if (nullptr == pCollider)
		return nullptr;

	return pCollider;
}

void CCollision_Manager::Collect_Collider(CColider::TYPE eType, CColider* pCollider)
{
	if (nullptr == pCollider)
		return;

	m_ColliderPool[eType].push_back(pCollider);
}

void CCollision_Manager::Free()
{
	for (_uint i = 0; i < COLLISION_END; ++i)
	{
		m_GameObjects[i].clear();
		m_ColliderGameObjects[i].clear();
		m_Colliders[i].clear();
	}
	for (_uint i = 0; i < COLLIDER_END; ++i)
	{
		for (auto& iter : m_ColliderPool[i])
			Safe_Release(iter);

		m_ColliderPool[i].clear();
	}
}
