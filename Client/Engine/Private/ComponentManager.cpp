#include "ComponentManager.h"

IMPLEMENT_SINGLETON(CComponentManager)

CComponentManager::CComponentManager()
{

}

HRESULT CComponentManager::Reserve_Container(_uint iNumLevels)
{
	if (nullptr != m_pPrototypes)
		return E_FAIL;

	m_pPrototypes = new PROTOTYPES[iNumLevels];
	m_iNumLevels = iNumLevels;

	return S_OK;
}

HRESULT CComponentManager::Add_Prototype(_uint iLevelIdx, const _tchar* pPrototypeTag, CComponent* pPrototype)
{
	if (nullptr == m_pPrototypes || iLevelIdx >= m_iNumLevels)
		return E_FAIL;

	if (nullptr != Find_Component(iLevelIdx, pPrototypeTag))
		return E_FAIL;

	m_pPrototypes[iLevelIdx].emplace(pPrototypeTag, pPrototype);

	return S_OK;
}

CComponent* CComponentManager::Clone_Component(_uint iLevelIdx, const _tchar* pPrototypeTag, void* pArg)
{
	if (nullptr == m_pPrototypes || iLevelIdx >= m_iNumLevels)
		return nullptr;

	CComponent* pPrototype = Find_Component(iLevelIdx, pPrototypeTag);
	if (nullptr == pPrototype)
		return nullptr;
	
 	CComponent* pComponent = pPrototype->Clone(pArg);
	if (nullptr == pComponent)
		return nullptr;
	return pComponent;
}

HRESULT CComponentManager::Find_Prototype(_uint iLevelIdx, const _tchar* pPrototypeTag)
{
	CComponent* pPrototype = Find_Component(iLevelIdx, pPrototypeTag);
	if (nullptr == pPrototype)
		return S_OK;

	return E_FAIL;
}

void CComponentManager::Clear(_uint iLevelIdx)
{
	if (iLevelIdx >= m_iNumLevels)
		return;

	for (auto& Pair : m_pPrototypes[iLevelIdx])
		Safe_Release(Pair.second);

	m_pPrototypes[iLevelIdx].clear();
}

CComponent* CComponentManager::Find_Component(_uint iLevelIdx, const _tchar* pPrototypeTag)
{
	auto iter = find_if(m_pPrototypes[iLevelIdx].begin(), m_pPrototypes[iLevelIdx].end(), CTag_Finder(pPrototypeTag));

	if (iter == m_pPrototypes[iLevelIdx].end())
		return nullptr;

	return iter->second;
}

void CComponentManager::Free()
{
	for (_uint i = 0; i < m_iNumLevels; ++i)
	{
		for (auto& Pair : m_pPrototypes[i])
		{
			Safe_Release(Pair.second);
		}
		m_pPrototypes[i].clear();
	}
	Safe_Delete_Array(m_pPrototypes);
}
