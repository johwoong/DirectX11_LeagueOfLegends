
#pragma once

#include "Client_Defines.h"
#include "Actor.h"

NS_BEGIN(Engine)
class CModel;
NS_END

NS_BEGIN(Client)
class CPlayer abstract :public CActor
{ 
public:
	enum PLAYERID{EZREAL, RENEKTON, };
protected:
	CPlayer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CPlayer(const CPlayer& rhs);
	virtual ~CPlayer() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual int Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);
	virtual HRESULT Render();

public:
	PLAYERID Get_PlayerID() const { return m_ePlayerID; }
	void Set_PlayerID(PLAYERID eID) { m_ePlayerID = eID; }



protected:
	// Component
	CModel*		m_pModelCom =		 { nullptr };
	PLAYERID	m_ePlayerID = EZREAL;
	


protected:
	virtual HRESULT Ready_Components(void* pArg) = 0;
	virtual HRESULT SetUp_ShaderResources();

public:
	virtual void Free() override;
};
NS_END

