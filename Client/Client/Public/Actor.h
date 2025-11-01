#pragma once
#include "GameObject.h"
#include "Client_Defines.h"
#include "Transform.h"

NS_BEGIN(Engine)
class CRenderer;
class CTransform;
class CShader;
NS_END

NS_BEGIN(Client)

class CActor abstract :public CGameObject
{
protected:
	CActor(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CActor(const CActor& rhs);
	virtual ~CActor() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual int Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);
	virtual HRESULT Render();


public:
	virtual _bool Picking(_float3* PickingPoint) { return false; }
	virtual void PickingTrue() {};
	_bool Is_Frustum(_float fOffset = 0);
	void SetUp_BillBoard();
public:
	_float3 Get_Scale() { return m_vScale; }
	CTransform* Get_Transform() { return m_pTransformCom; }
	_vector Get_TransformState(CTransform::STATE eState);
	void Set_State(CTransform::STATE eState, _fvector vState);
	void Set_Scale(_float3 vScale);
	void Set_Picked(_bool type) { m_bPicked = type; }

protected:
	virtual HRESULT Ready_Components(void* pArg) = 0;
	virtual HRESULT SetUp_ShaderID() { return S_OK; }
	virtual HRESULT SetUp_ShaderResources() = 0;

protected:
	// Component
	CShader* m_pShaderCom = { nullptr };
	CRenderer* m_pRendererCom = { nullptr };
	CTransform* m_pTransformCom = { nullptr };

protected:
	_uint	m_eShaderID = SHADER_NOANIM_DEFAULT;
	_float3 m_vScale = _float3(1.f, 1.f, 1.f);
	_float3 m_vMousePickedPos;
	_bool m_bPicked = false;
	_float m_fAlpha = 1.f;

public:
	virtual void Free() override;

};

NS_END

