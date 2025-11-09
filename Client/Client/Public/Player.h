
#pragma once

#include "Actor.h"
#include "Client_Defines.h"
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
	virtual HRESULT Render() = 0;

public: // GET, SET
	CModel* Get_Model() { return m_pModelCom; }
	CPlayerState* Get_State() { return m_pPlayerState; }
	void Set_PlayerState(class CPlayerState* pPlayerState) { m_pPlayerState = pPlayerState; }
	void Set_TargetPosition(_float3 vPos) { m_vTargetPos = vPos; m_bMoving = true; }
	_float3 Get_TargetPosition() { return m_vTargetPos; }
	PLAYERID Get_PlayerID() const { return m_ePlayerID; }
	void Set_PlayerID(PLAYERID eID) { m_ePlayerID = eID; }

public: // State
	virtual _bool Is_AnimationLoop(_uint eAnimId) = 0;
	void HandleInput();
	void Update_State(_float fTimeDelta);
	void Late_Update_State(_float fTimeDelta);
	
protected:
	// Component
	CModel*		m_pModelCom =		 { nullptr };

protected: // 챔피언 교체
	PLAYERID	m_ePlayerID = EZREAL;
	CPlayerState* m_pPlayerState = { nullptr };
	CPlayerManager* m_pPlayerManager = { nullptr };

protected: // 플레이어 이동
	_bool m_bMoving = false;
	_float3 m_vTargetPos = { 0.f, 0.f, 0.f };

protected:
	virtual HRESULT Ready_Components(void* pArg) = 0;
	virtual HRESULT SetUp_ShaderResources() = 0;

public:
	virtual void  Free() override;
};
NS_END

