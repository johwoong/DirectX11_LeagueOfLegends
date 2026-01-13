#pragma once
#include "Enemy.h"
#include "Client_Defines.h"

NS_BEGIN(Client)
class CVayne final : public CEnemy
{
public:
    enum ANIM {
        ATTACK1, ATTACK2, CHANNEL, CHANNEL_WIND, CRIT,DANCE, DEATH, IDLE1, IDLE2, IDLE3, IDLE4,
        JOKE, LAUGH, RUN, Q,E, TAUNT, Q_ATTACK, Q_IDLE, Q_RUN, R_ATTACK, RQ_IDLE, R_RUN,
        R_THREE_ATTACK, R_IDLE, IDLE
    };
	


public:
	CModel* Get_Model() { return m_pModelCom; }
	CTransform* Get_Transform() { return m_pTransformCom; }
	CShader* Get_Shader() { return m_pShaderCom; }
	class CVayneState* Get_State() { return m_pState; }
	// 🔹 MoveState에서 사용할 목적지
	void Set_LaneTarget(const _float3& v) { m_vLaneTarget = v; }
	const _float3& Get_LaneTarget() const { return m_vLaneTarget; }
	_int        Get_LaneNodeIndex() const { return m_iLaneNodeIndex; }
	void        Set_LaneNodeIndex(_int i) { m_iLaneNodeIndex = i; }
	LINE_TYPE   Get_LineType()     const { return m_eLineType; }
	CNavigation* Get_Navigation() { return m_pNavigationCom; }
public:
	virtual _bool Is_AnimationLoop(_uint eAnimId) override;
	virtual void Ready_Animation_Event() override;
	virtual HRESULT SetUp_ShaderID() override;

private:
	CVayne(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CVayne(const CVayne& rhs);
	virtual ~CVayne() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual int Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);
	virtual HRESULT Render() override;
	virtual HRESULT Render_Glow() override;

public: /*For.State*/
	void AI_Behavior(_float fTimeDelta);
	void Update_State(_float fTimeDelta);
	void LateUpdate_State(_float fTimeDelta);
	virtual _int Take_Damage(int iDamage, CActor* DamageCauser) override; // ★ 추가
public:	/*For Navigation*/
	virtual void Check_Navigation() override;
	// 🔹 미니언처럼 경로 따라 이동
protected:
	virtual void OnRespawned() override;
private:
	bool  m_bJustAttacked = false;
public:
	float m_fPostAttackMoveTime = 0.f;
public:
	void Set_JustAttacked(bool b) { m_bJustAttacked = b; }
	bool Get_JustAttacked() const { return m_bJustAttacked; }
	// Vayne.h
	_vector m_vKitingDest = XMVectorZero();
	bool    m_bDoKiting = false;
	bool m_bQEmpowered = false;          // Q 강화 대기 상태
	bool m_bConsumeQThisAttack = false; // 이번 공격이 Q 강화인지
	float m_fQEmpoweredTime = 0.f;
	const float Q_EMPOWER_DURATION = 3.0f; // 베인 원본 느낌
	bool m_bSkillLock = false;
private:
	virtual HRESULT Ready_Components(void* pArg) override;

private:
	class CVayneState* m_pState = nullptr;
public:
	static CVayne* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free() override;

};
NS_END

