#pragma once
#include "Client_Defines.h"
#include "GameObject.h"
#include "Transform.h"

NS_BEGIN(Engine)
class CShader;
class CRenderer;
class CTransform;
NS_END

NS_BEGIN(Client)

class CActor abstract : public CGameObject
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

public:
	OBJID Get_ObjectID() { return m_eObjectID; }
	_float3 Get_Scale() { return m_vScale; }
	_vector Get_Position();
	void Set_State(CTransform::STATE eState, _fvector vState);
	void Set_Scale(_float3 vScale);
	virtual void Set_Picked(_bool Type) { m_bPicked = Type; }
	CTransform* Get_Transform() { return m_pTransformCom; }
	_vector Get_TransformState(CTransform::STATE eState);

protected:
	CShader* m_pShaderCom = { nullptr };
	CRenderer* m_pRenderCom = { nullptr };
	CTransform* m_pTransformCom = { nullptr };

protected:
	virtual HRESULT Ready_Components(void* pArg) = 0;
	virtual HRESULT SetUp_ShaderID() { return S_OK; }
	virtual HRESULT SetUp_ShaderResources() = 0;
	 
protected:
	_uint m_eShaderID = SHADER_DEFAULT;
	OBJID m_eObjectID = OBJ_END;
	_float3 m_vScale = _float3(1.f, 1.f, 1.f);
	_float3 m_vMousePickPos;
	_bool m_bPicked = false;

public:
	virtual void Free() override;

};

NS_END

