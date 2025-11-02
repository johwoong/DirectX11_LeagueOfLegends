
#pragma once

#include "Client_Defines.h"
#include "Actor.h"
#include "PlayerManager.h"

NS_BEGIN(Engine)
class CModel;
NS_END

NS_BEGIN(Client)
class CPlayerState;
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

public:
	void Set_TargetPosition(const _float3 vPos)
	{
		m_vTargetPos = vPos;
		m_bMoving = true;
	}

protected:
	// Component
	CModel*		m_pModelCom =		 { nullptr };

protected: // 챔피언 교체
	PLAYERID	m_ePlayerID = EZREAL;
	CPlayerState* m_ePlayerState = { nullptr };
	CPlayerManager* m_pPlayerManager = { nullptr };

protected: // 플레이어 이동
	_bool m_bMoving = false;
	_float3 m_vTargetPos = { 0.f, 0.f, 0.f };



protected:
	virtual HRESULT Ready_Components(void* pArg) = 0;
	virtual HRESULT SetUp_ShaderResources();

public:
	virtual void Free() override;
};
NS_END

