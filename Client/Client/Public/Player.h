
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
	enum PLAYERID{EZREAL, LEESIN, TEEMO, };
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

public:
	bool Is_Down() const { return m_bDown; }
	void Set_Down(bool b) { m_bDown = b; }

	void Set_DeathAnim(_uint i) { m_iDeathAnim = i; }
	void Set_SpawnAnim(_uint i) { m_iSpawnAnim = i; }
	_uint Get_DeathAnim() const { return m_iDeathAnim; }
	_uint Get_SpawnAnim() const { return m_iSpawnAnim; }

public:
	virtual bool IsDeadForTargeting() const override { return m_bDown; }
public:
	bool Is_Spawning() const { return m_bSpawning; }
	void Set_Spawning(bool b) { m_bSpawning = b; }

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
public:
	bool Is_LeeSinLeftAttack() const { return m_bLeeSinLeftAttack; }
	void Toggle_LeeSinAttackSide() { m_bLeeSinLeftAttack = !m_bLeeSinLeftAttack; }
	void Set_PlayTeemoIdleIn(bool b) { m_bPlayTeemoIdleIn = b; }
	bool Get_PlayTeemoIdleIn() const { return m_bPlayTeemoIdleIn; }
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

protected:
	bool  m_bDown = false;          
	_uint m_iDeathAnim = 0;    
	_uint m_iSpawnAnim = 0;  
protected:
	bool m_bSpawning = false;

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
private:
	bool m_bLeeSinLeftAttack = true;
	bool m_bPlayTeemoIdleIn = false;
protected:
	virtual HRESULT Ready_Components(void* pArg) = 0;
	virtual HRESULT SetUp_ShaderResources() = 0;

public:
	virtual void  Free() override;
};
NS_END

