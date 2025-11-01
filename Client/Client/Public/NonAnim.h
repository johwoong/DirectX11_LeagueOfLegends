
#pragma once
#include "Actor.h"

NS_BEGIN(Engine)
class CModel;
NS_END
class CNonAnim :
    public CActor
{
protected:
	CNonAnim(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CNonAnim(const CNonAnim& rhs);
	virtual ~CNonAnim() = default;
public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual int Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);
	virtual HRESULT Render();

public:
	const char* Get_ModelTag() { return m_ModelDesc.pModeltag; }
	const NONANIMDESC Get_ModelDesc() { return m_ModelDesc; }
	_float Check_CullingRadius();

public:
	virtual _bool Picking(_float3* PickingPoint) override { return true; };
	virtual void PickingTrue() override {};

protected:
	CModel* m_pModelCom = { nullptr };
	NONANIMDESC m_ModelDesc;
	_float	m_fAlpha = 1.f;

protected:
	virtual HRESULT Ready_Components(void* pArg) override;
	virtual HRESULT SetUp_ShaderResources() override;
	virtual HRESULT SetUp_ShaderID() override;

public:
	static CNonAnim* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free() override;
};

 