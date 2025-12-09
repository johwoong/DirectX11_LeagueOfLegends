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
	// ★ 추가
	_int        Get_LaneNodeIndex() const { return m_iLaneNodeIndex; }
	void        Set_LaneNodeIndex(_int i) { m_iLaneNodeIndex = i; }
	LINE_TYPE   Get_LineType()     const { return m_eLineType; }
	CNavigation* Get_Navigation() { return m_pNavigationCom; }
	// ★ LanePath 얻기용 정적 함수 (MoveState에서 쓰게)
	static const vector<_float3>& Get_EnemyLanePath(LINE_TYPE eLine);
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
	void Follow_Lane(_float fTimeDelta);

private:
	virtual HRESULT Ready_Components(void* pArg) override;

private:
	class CVayneState* m_pState = nullptr;
	// 🔹 이 베인이 향할 최종 목적지 (적 타워 근처)
	_float3 m_vLaneTarget = { 0.f, 0.f, 0.f };
	// ★ 현재 경로 인덱스
	_int    m_iLaneNodeIndex = 0;
public:
	static CVayne* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free() override;

};
NS_END

