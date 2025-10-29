#pragma once

#include "Client_Defines.h"
#include "Actor.h"

NS_BEGIN(Engine)
class CTexture;
class CVIBuffer_Terrian;
NS_END

NS_BEGIN(Client)
class CTerrian final :
    public CActor
{
public:
	enum TERRAIN_DEBUG_TYPE { DEBUG_SOLID, DEBUG_WIRE, DEBUG_NONE };

private:
	CTerrian(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CTerrian(const CTerrian& rhs);
	virtual ~CTerrian() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;
	HRESULT Initialize_Load(const _tchar* VIBufferTag, void* pArg);
	virtual int Update(_float fTimeDelta) override;
	virtual void Late_Update(_float fTimeDelta) override;
	virtual HRESULT Render() override;

private:
	HRESULT Ready_Components(void* pArg);
	HRESULT SetUp_ShaderResources();

private:
	CVIBuffer_Terrian* m_pVIBufferCom = nullptr;

	_bool m_bWireFrame = false;
	TERRAIN_DEBUG_TYPE m_eDebugtype = DEBUG_SOLID;

public:
	static CTerrian* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr) override;
	virtual CGameObject* Clone_Load(const _tchar* VIBufferTag, void* pArg = nullptr) override;
	virtual void Free() override;
};
NS_END

