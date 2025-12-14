#pragma once
#include "Actor.h"
#include "VIBuffer_Terrian.h"   // ? 이 줄 추가

NS_BEGIN(Engine)
class CTexture;
NS_END

NS_BEGIN(Client)
class CTerrian final : public CActor
{
public:
	enum SHADERID { DEFAULLT, WIRE, PICKED };
	enum TERRIAN_DEBUG_TYPE { DEBUG_SOLID, DEBUG_WIRE, DEBUG_NONE };
	enum TEXTURE { TYPE_DIFFUSE, TYPE_END };

private:
	CTerrian(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CTerrian(const CTerrian& rhs);
	virtual ~CTerrian() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg)override;
	HRESULT Initialize_Load(const _tchar* VIBufferTag, void* pArg);
	virtual int Update(_float fTimeDelta)override;
	virtual void Late_Update(_float fTimeDelta)override;
	virtual HRESULT Render() override;

public:
	const CVIBuffer_Terrian::TERRIANDESC& Get_TerrianDesc() const { return m_pVIBufferCom->Get_TerrianDesc(); }
public:
	virtual _bool Picking(_float3* PickingPoint);
	virtual void PickingTrue();

public:
	void Set_Picked();

private:
	virtual HRESULT Ready_Components(void* pArg)override;
	virtual HRESULT SetUp_ShaderID()override;
	virtual HRESULT SetUp_ShaderResources()override;

public:
	void Save_Terrain(HANDLE hFile, _ulong* dwByte);

private:
	CTexture* m_pTextureCom[TYPE_END] = { nullptr };
	CVIBuffer_Terrian* m_pVIBufferCom = { nullptr };
	_uint* pPixel = { nullptr };

	_float3* pPixelColor = { nullptr };

	_bool m_bFirst = false;
	_bool	m_bWireFrame = false;
	_bool	m_bDebugShow = true;
	TERRIAN_DEBUG_TYPE m_eDebugtype = DEBUG_NONE;

public:
	static CTerrian* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr) override;
	virtual CGameObject* Clone_Load(const _tchar* VIBufferTag, void* pArg = nullptr) override;
	virtual void Free() override;
};
NS_END
