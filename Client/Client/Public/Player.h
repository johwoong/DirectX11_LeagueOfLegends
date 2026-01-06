
#pragma once

#include "Actor.h"
#include "Client_Defines.h"
#include "PlayerManager.h"

NS_BEGIN(Engine)
class CModel;
class CNavigation;
NS_END

NS_BEGIN(Client)
class CPlayerState;
class CPlayer abstract :public CActor
{ 
public:
	enum PLAYERID{EZREAL, VI, };
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
	virtual HRESULT Render_ShadowDepth() override;
	virtual HRESULT Render_EdgeDetection() override;

public: // GET, SET
	CModel* Get_Model() { return m_pModelCom; }
	CNavigation* Get_Navigation(void) { return m_pNavigationCom; }
	CPlayerState* Get_State() { return m_pPlayerState; }
	void Set_PlayerState(class CPlayerState* pPlayerState) { m_pPlayerState = pPlayerState; }
	void Set_TargetPosition(_float3 vPos) { m_vTargetPos = vPos; m_bMoving = true; }
	_float3 Get_TargetPosition() { return m_vTargetPos; }
	PLAYERID Get_PlayerID() const { return m_ePlayerID; }
	void Set_PlayerID(PLAYERID eID) { m_ePlayerID = eID; }
	void Reset_State();
	void Set_AttackTarget(CActor* pTarget) { m_pAttackTarget = pTarget; }
	CActor* Get_AttackTarget() const { return m_pAttackTarget; }
	bool Get_PlayIdleQ() const { return m_bPlayIdleQ; }
	void Set_PlayIdleQ(bool v) { m_bPlayIdleQ = v; }
	bool IsUsingSkill() const { return m_bUsingSkill; }
	void SetUsingSkill(bool b) { m_bUsingSkill = b; }
	bool IsMoving() const { return m_bMoving; }
public: // State
	virtual _bool Is_AnimationLoop(_uint eAnimId) = 0;
	void HandleInput();
	void Update_State(_float fTimeDelta);
	void Late_Update_State(_float fTimeDelta);

public: // Behavior
	void Respawon();
	 

public: // Navigation
	void Check_Navigation();
	void Compute_CurrentIndex();

public: // Animation Event
	virtual void Ready_Animation_Event() PURE;
public:
	/* LimGright */
	void            Set_Overlimit(_bool bOverlimit) { m_bOverLimit = bOverlimit; }
	_bool           Get_Overlimit() { return m_bOverLimit; }
	void			Get_AuraColor();
	
protected:
	// Component
	CModel*		m_pModelCom =		 { nullptr };
	CNavigation* m_pNavigationCom = { nullptr };
protected: // 챔피언 교체
	PLAYERID	m_ePlayerID = EZREAL;
	CPlayerState* m_pPlayerState = { nullptr };
	CPlayerManager* m_pPlayerManager = { nullptr };

	// Rim Light
	_bool			m_bOverLimit = false;
	_float			m_fOverLimitTimer = 0.f;
	_float			m_fFresnelTimer = 0.f;
	_bool			m_bIsOverlimiEffectSpawned = false;
	_float3			m_vAuraColor;
	_float			m_fAuraTimer = 0.f;

protected: // 플레이어 이동
	_bool m_bMoving = false;
	_float3 m_vTargetPos = { 0.f, 0.f, 0.f };
	_float3 m_vSpawnPos = { 0.f, 0.f, 0.f };

protected: // 애니메이션 교체 관련
 	bool m_bPlayIdleQ = false;
protected: // 스킬
	bool m_bUsingSkill = false;

protected:
	CActor* m_pAttackTarget = nullptr;     // 현재 평타 타겟 (추후 공격 상태에서 사용)

protected:
	virtual HRESULT Ready_Components(void* pArg) = 0;
	virtual HRESULT SetUp_ShaderResources() = 0;

public:
	virtual void  Free() override;
};
NS_END

