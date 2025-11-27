#include "PoolManager.h"
#include "GameInstance.h"
#include "Model.h"
#include <fstream>
#include <string.h>
#include "GameObject.h"

IMPLEMENT_SINGLETON(CPoolManager)



CPoolManager::CPoolManager()
{
	m_pObjects = new POOLINGOJECTS[LEVEL_END];
}

void CPoolManager::Add_Pooling_Object(LEVEL iLevelIdx, const _tchar* pLayerTag, CGameObject* pGameObject)
{
	CGameInstance* pGameInstance = CGameInstance::GetInstance();
	pGameInstance->AddRef();

	// 1) 먼저 GameInstance의 레이어에서 이 객체를 빼준다.
	//    Out_GameObject는 단순히 레이어 리스트에서만 제거하고, Safe_Release는 안 하므로
	//    풀에서 재사용하기에 딱 좋음.
	pGameInstance->Out_GameObject(iLevelIdx, pLayerTag, pGameObject);

	// 2) 그 다음 풀 리스트에 넣는다.
	list<class CGameObject*>* pLayer = Find_PoolingObjects(iLevelIdx, pLayerTag);
	if (pLayer == nullptr)
	{
		pLayer = new LAYERS;
		pLayer->push_back(pGameObject);
		m_pObjects[iLevelIdx].emplace(pLayerTag, pLayer);
	}
	else
	{
		pLayer->push_back(pGameObject);
	}

	Safe_Release(pGameInstance);
}

void CPoolManager::Add_Pooling_Layer(LEVEL iLevelIdx, const _tchar* pLayerTag)
{
	CGameInstance* pGameInstance = CGameInstance::GetInstance();

	auto pLayerList = pGameInstance->Get_ObjectList(iLevelIdx, pLayerTag);
	if (pLayerList == nullptr)
		return;

	auto pLayer = Find_PoolingObjects(iLevelIdx, pLayerTag);
	if (pLayer == nullptr)
		pLayer = new LAYERS;

	for (auto& obj : *pLayerList)
		pLayer->push_back(obj);

	m_pObjects[iLevelIdx].emplace(pLayerTag, pLayer);
	pGameInstance->Out_GameObjectList(iLevelIdx, pLayerTag);
}

_bool CPoolManager::Reuse_Pooling_Layer(LEVEL iLevelIdx, const _tchar* pLayerTag, void* pArg)
{
	CGameInstance* pGameInstance = CGameInstance::GetInstance();

	auto pLayer = Find_PoolingObjects(iLevelIdx, pLayerTag);
	if (pLayer == nullptr)
		return false;

	for (auto iter = pLayer->begin(); iter != pLayer->end(); )
	{
		if (pArg)
			(*iter)->ReUse_Setting(pArg);

		pGameInstance->ReAdd_GameObject(iLevelIdx, pLayerTag, *iter);
		iter = pLayer->erase(iter);
	}
	return true;
}

_bool CPoolManager::Reuse_Pooling_Object(LEVEL iLevelIdx, const _tchar* pLayerTag, void* pArg)
{
	CGameInstance* pGameInstance = CGameInstance::GetInstance();

	auto pLayer = Find_PoolingObjects(iLevelIdx, pLayerTag);
	if (pLayer == nullptr || pLayer->empty())
		return false;

	auto iter = pLayer->begin();

	if (pArg)
		(*iter)->ReUse_Setting(pArg);

	pGameInstance->ReAdd_GameObject(iLevelIdx, pLayerTag, *iter);
	pLayer->erase(iter);

	return true;
}

_bool CPoolManager::Clear_AllPooling_Layer()
{
	for (int i = 0; i < LEVEL_END; ++i)
	{
		for (auto& iter : m_pObjects[i])
		{
			for (auto it = iter.second->begin(); it != iter.second->end(); )
			{
				Safe_Release(*it);
				it = iter.second->erase(it);
			}
			iter.second->clear();
			Safe_Delete(iter.second);
		}
		m_pObjects[i].clear();
	}
	Safe_Delete_Array(m_pObjects);
	return true;
}

list<class CGameObject*>* CPoolManager::Find_PoolingObjects(_uint iLevelIdx, const _tchar* pLayerTag)
{
	if (iLevelIdx >= LEVEL_END)
		return nullptr;

	auto	iter = find_if(m_pObjects[iLevelIdx].begin(), m_pObjects[iLevelIdx].end(), CTag_Finder(pLayerTag));
	if (iter == m_pObjects[iLevelIdx].end())
		return nullptr;

	return iter->second;
}

_bool CPoolManager::Find_LayerTag(_uint iLevelIdx, const _tchar* pLayerTag)
{
	if (iLevelIdx >= LEVEL_END)
		return false;

	auto	iter = find_if(m_pObjects[iLevelIdx].begin(), m_pObjects[iLevelIdx].end(), CTag_Finder(pLayerTag));
	if (iter == m_pObjects[iLevelIdx].end())
		return false;

	return true;
}

void CPoolManager::Free()
{
}
