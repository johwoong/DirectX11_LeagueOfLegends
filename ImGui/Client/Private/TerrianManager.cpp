#include "TerrianManager.h"
#include "GameInstance.h"
#include "PickingMgr.h"

IMPLEMENT_SINGLETON(CTerrianManager)

CTerrianManager::CTerrianManager()
{
}

CVIBuffer_Terrian* CTerrianManager::Create_Terrain(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, HANDLE hFile, _ulong& dwByte)
{
    return CVIBuffer_Terrian::Create(pDevice, pContext, hFile, dwByte);
}


HRESULT CTerrianManager::Create_Terrain(LEVEL eLevel, const _tchar* pLayerTag, const _tchar* pTerrainTag)
{
	CGameInstance* pGameInstance = CGameInstance::GetInstance();
	pGameInstance->AddRef();

	TERRIANDESC TerrianDesc = m_pTerrianDesc;
	TerrianDesc.m_eDebugTerrian = DEBUG_NONE;

	if (pTerrainTag == nullptr)
	{
		if (FAILED(pGameInstance->Add_GameObject(TEXT("Prototype_GameObject_Terrain"), eLevel, pLayerTag, &TerrianDesc)))
			return E_FAIL;
	}
	else
	{
		if (FAILED(pGameInstance->Add_GameObjectLoad(TEXT("Prototype_GameObject_Terrain"), eLevel, pLayerTag, pTerrainTag, nullptr)))
			return E_FAIL;
	}

	pGameInstance->Release();

    return S_OK;
}


void CTerrianManager::Add_PrototypeTag(_tchar* TempTag)
{
	_tchar* pModeltag = new _tchar[MAX_PATH];
	wsprintf(pModeltag, TempTag);
	m_pPrototypeTags.push_back(pModeltag);
}

void CTerrianManager::Free()
{
	for (auto& iter : m_pPrototypeTags)
		Safe_Delete(iter);
	m_pPrototypeTags.clear();

	m_pCloneTerrianTags.clear();
}
