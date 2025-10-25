#pragma once
#include "VIBuffer.h"


NS_BEGIN(Engine)
class CTexture;

class ENGINE_DLL CVIBuffer_Terrian final : public CVIBuffer
{
public:
	typedef struct TerrianTagDesc
	{
		_int  m_iPositionX = 0;
		_int  m_iPositionZ = 0;
		_int  m_iVerticeNumX = 10;
		_int  m_iVerticeNumZ = 10;
		_float m_fHeight = 0;
		_bool m_isHeightMap = false;
	}TERRIANDESC;

protected:
	CVIBuffer_Terrian(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CVIBuffer_Terrian(const CVIBuffer_Terrian& rhs);
	virtual ~CVIBuffer_Terrian() = default;

public:
	virtual HRESULT Initialize_Prototype(_uint iNumVerticeX, _uint iNumVerticeZ);
	virtual HRESULT Initialize(void* pArg);
	virtual _bool Picking(class CTransform* pTransform, _float3* pOut) override;
	void Set_Terrain_UpDown(_float fVec, _float fRad, _float fSharp, _float3 vPoint, _float fTimeDelta);
	void Set_Terrain_Buffer(TERRIANDESC TerrainDesc);
	HRESULT Save_VertexPosition(HANDLE hFile, _ulong* dwByte);
	HRESULT Load_Prototype(HANDLE hFile, _ulong& dwByte, _bool bCreateQuadTree = false);

public:
	TERRIANDESC Get_TerrianDesc() { return m_pTerrianDesc; }

private:
	_uint m_iNumVerticesX = 0, m_iNumVerticesZ = 0;
	TERRIANDESC m_pTerrianDesc;
	vector<VTXNORTEX> vecVertex;
	_float3				m_bPickingLocalPos = _float3(0.0f, 0.f, 0.f);

public:
	static CVIBuffer_Terrian* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _uint iNumVerticeX, _uint iNumVerticeZ, _float fHeight);
	static CVIBuffer_Terrian* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, HANDLE hFile, _ulong& dwByte);

	virtual CComponent* Clone(void* pArg = nullptr);
	virtual void Free() override;
};
NS_END

