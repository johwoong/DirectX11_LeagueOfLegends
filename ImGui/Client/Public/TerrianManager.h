#pragma once
#include "Base.h"
#include "Client_Defines.h"
#include "VIBuffer_Terrian.h"
#include "Terrian.h"

NS_BEGIN(Client)
class CTerrianManager final : public CBase
{
	DECLARE_SINGLETON(CTerrianManager)

public:
	enum TERRIAN_DEBUG_TYPE {DEBUG_SOLID, DEBUG_WIRE, DEBUG_NONE};

	typedef struct TerrianTagDesc
	{
		CVIBuffer_Terrian::TERRIANDESC TerrianDesc;

		_bool m_bShowWireFrame = true;
		TERRIAN_DEBUG_TYPE m_eDebugTerrian = DEBUG_NONE;
	}TERRIANDESC;

private:
	CTerrianManager();
	virtual ~CTerrianManager() = default;

public:
	void Add_DebugTerrain(class CTerrian* pTerrain) { m_pTerrian = pTerrain; }
	CVIBuffer_Terrian* Create_Terrain(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, HANDLE hFile, _ulong& dwByte);
	HRESULT Create_Terrain(LEVEL eLevel, const _tchar* pLayerTag, const _tchar* pTerrainTag = nullptr);
	void Out_DebugTerrain();

public:
	void Set_TerrainDesc(TERRIANDESC* eTerrainDesc) { memcpy(&m_pTerrianDesc, eTerrainDesc, sizeof(TERRIANDESC)); }
	void Set_bWireFrame(_bool type) { m_pTerrianDesc.m_bShowWireFrame = type; }
	void Set_TerrainShow(_bool type) { m_bTerrianShow = type; }
	void Set_PickingWorldPos(_float3 vPickingPos) { m_vMousePickPos = vPickingPos; }

public:
	TERRIANDESC Get_TerrianDesc() const { return m_pTerrianDesc; }
	_bool Get_TerrianShow() { return m_bTerrianShow; }
	_float3 Get_PickingPos() { return m_vMousePickPos; }
	vector<const _tchar*> Get_PrototypeList() { return m_pPrototypeTags; }

public:
	void Add_PrototypeTag(_tchar* TempTag);

private:
	class CTerrian* m_pTerrian = { nullptr };
	TERRIANDESC m_pTerrianDesc;
	_bool m_bTerrianShow = true;
	_float3 m_vMousePickPos = _float3(0, 0, 0);
private:
	vector<const _tchar*>	m_pPrototypeTags;
	vector<const _tchar*>	m_pCloneTerrianTags;

public:
	void Free() override;

};
NS_END

