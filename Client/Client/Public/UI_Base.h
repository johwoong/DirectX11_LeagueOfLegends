#pragma once
#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CShader;
class CTexture;
class CRenderer;
class CTransform;
class CVIBuffer_Rect;
NS_END

NS_BEGIN(Client)

class CUI_Base abstract :public CGameObject
{
public:
	enum SHADERUI_ID {UI_DEFAULT, UI_ALPHABLEND, UI_END};

protected:
	CUI_Base(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CUI_Base(const CUI_Base& rhs);
	virtual ~CUI_Base() = default;

public:
	virtual HRESULT Initialize_Prototype() = 0;
	virtual HRESULT Initialize(void* pArg);
	virtual int Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);
	virtual HRESULT Render();

public:
	_float2 Get_Position() { return m_fPosition; }
	_float2 Get_Scale() { return m_fSize; }
	_float3	Get_Scale_float3() { return m_vScale; };
	void Set_Position(_float2 fPostion) { m_fPosition = fPostion; };
	void Set_Scale(_float2 fScale) { m_fSize = fScale; };
	void Set_Scale(_float3 vScale);
	_bool Get_Dead() { return m_bDead; }
	void SetUp_BillBoard();


protected:
	CShader* m_pShaderCom = nullptr;
	CTexture* m_pTextureCom = nullptr;
	CRenderer* m_pRendererCom = nullptr;
	CTransform* m_pTransformCom = nullptr;
	CVIBuffer_Rect* m_pVIBufferCom = nullptr;

protected:
	_float2					m_fPosition, m_fSize;
	_float4x4				m_ViewMatrix, m_ProjMatrix;
	_uint					m_eShaderID = UI_DEFAULT;
	_float					m_fAlpha = 1.f;
	_float					m_fGlowPower = 1.f;

	_float3			m_vScale = _float3(1.f, 1.f, 1.f);

public:
	virtual HRESULT Ready_Components(void* pArg = nullptr) = 0;
	virtual HRESULT SetUp_ShaderResources() = 0;
	virtual HRESULT SetUp_ShaderID() { return S_OK; };

public:
	virtual void Free() override;
};

NS_END

