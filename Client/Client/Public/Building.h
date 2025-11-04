#pragma once
#include "Actor.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CModel;
NS_END

NS_BEGIN(Client)
class CBuilding abstract : public CActor
{
public:
	enum TEAM {RED, BLUE};
public:
	typedef struct BuildgTag
	{
		NONANIMDESC ModelDesc;
		TEAM		eTeamType;
	}BUILDINGDESC;

protected:
	CBuilding(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CBuilding(const CBuilding& rhs);
	virtual ~CBuilding() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual int Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);
	virtual HRESULT Render();

protected:
	CModel* m_pModelCom = nullptr;
	BUILDINGDESC	m_BuildDesc;

protected:
	virtual HRESULT Ready_Components(void* pArg) = 0;
	virtual HRESULT SetUp_ShaderResources();
public:
	virtual void  Free() override;
};
NS_END

