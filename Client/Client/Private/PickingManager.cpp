#include "PickingManager.h"
#include "GameInstance.h"
#include "Transform.h"
#include "GameObject.h"
#include "Actor.h"

IMPLEMENT_SINGLETON(CPickingManager)


CPickingManager::CPickingManager()
{
}


void CPickingManager::Clear_PickingMgr()
{
	m_GameObjects.clear();
}


void CPickingManager::Add_PickingGroup(CGameObject* pGameObject)
{
	if (nullptr == pGameObject)
		return;
	m_GameObjects.push_back(pGameObject);
}

void CPickingManager::Out_PickingGroup(CGameObject* pGameObject)
{
	auto iter = m_GameObjects.begin();
	while (iter != m_GameObjects.end())
	{
		if (*iter == pGameObject)
			iter = m_GameObjects.erase(iter);
		else
			++iter;
	}
}

void CPickingManager::Set_PickedObj(CGameObject* pGameObject)
{
	if (pGameObject == nullptr)
	{
		if (m_pPickedObj != nullptr)
			dynamic_cast<CActor*>(m_pPickedObj)->Set_Picked(false);
		m_pPickedObj = nullptr;
		return;
	}

	m_pPickedObj = pGameObject;

	for (auto& pGameObject : m_GameObjects)
		dynamic_cast<CActor*>(pGameObject)->Set_Picked(false);

	dynamic_cast<CActor*>(m_pPickedObj)->Set_Picked(true);
}

_bool CPickingManager::Picking()
{
	if (m_bMouseInUI)
		return false;

	list<CGameObject*> vecPicked;
	vector<_float3>		vecPos;
	_float3 vPosition;

	for (auto& pGameObject : m_GameObjects)
	{
		if (pGameObject->Picking(&vPosition) == true)
		{
			vecPicked.push_back(pGameObject);
			vecPos.push_back(vPosition);
			m_vPickingPos = vPosition;
		}
	}

	if (!vecPicked.empty())
	{
		vecPicked.sort([](CGameObject* pSour, CGameObject* pDest)
			{
				return pSour->Get_CamDistance() < pDest->Get_CamDistance();
			});

		vecPicked.front()->PickingTrue();
		return true;
	}
	return false;
}



void CPickingManager::Free()
{
	m_GameObjects.clear();
}