#pragma once
#include "Actor.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CModel;
NS_END

NS_BEGIN(Client)
class CNpc abstract : public CActor
{
public:
	typedef struct BuildgTag
	{
		NONANIMDESC ModelDesc;
	}BUILDINGDESC;

protected:
	CNpc(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CNpc(const CNpc& rhs);
	virtual ~CNpc() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual int Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);
	virtual HRESULT Render();
	virtual HRESULT Render_ShadowDepth() override;
	virtual HRESULT Render_EdgeDetection() override;


public:
	virtual _bool Picking(_float3* PickingPoint);
	virtual void PickingTrue();
	virtual void Set_Picked();


protected:
	CModel* m_pModelCom = nullptr;
	BUILDINGDESC	m_BuildDesc;
protected:
	virtual HRESULT Ready_Components(void* pArg) PURE;
	virtual HRESULT SetUp_ShaderResources();
public:
	virtual void  Free() override;
};
NS_END
