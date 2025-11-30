#pragma once
#include "Champion.h"
class CVi final : public CChampion
{
public:
	enum ANIM {
		ATTACK1, ATTACK2, ATTACK3, ATTACK_AFTERULT, CHANNEL, CHANNEL_WINDUP, CRIT, DANCE,
		DANCE_IN, DEATH, IDLE1, IDLE1_INTOB, IDLE1_INTOC, IDLE2, IDLE2_INTO1, JOKE2, JOKE1, 
		LAUGH, RECALL, RUN1, RUN1_IN, RUN2, RUN2_IN, Q, Q_IDLE_1, Q_IDLEINTRO_1, Q_RUN, W,
		W_TWO, E, R, R_HIT1, R_RUN1, STUN, TAUNT, TAUNT_1, ANIM_END
	};
private:
	CVi(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CVi(const CVi& rhs);
	virtual ~CVi() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);

public:
	virtual _bool Is_AnimationLoop(_uint eAnimId) override;
	virtual void Ready_Animation_Event() override;
private:
	virtual void SetSkillInfo() override;
private:
	virtual HRESULT Ready_Components(void* pArg) override;
public:
	static CVi* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free() override;
};

