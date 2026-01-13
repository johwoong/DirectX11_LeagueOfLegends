#pragma once
#include "Champion.h"
class CTemoo final : public CChampion
{
public:
	enum ANIM {
		ATTACK1, ATTACK_TOIDLE, CHANNEL, CRIT, CRIT_TOIDLE, DANCE_1, DANCE_1_B, DANCE_2, DANCE_2_B, DEATH,
		HOMEGUARD, IDLE, IDLE_IN, IDLE2_IN, IDLE2, IDLE3, JOKE_IN, JOKE_OUT, JOKE_SPAM, KONCKUP, LAUGH, LAUGH_TOIDLE, 
		RECALL, RUN, RUN_TO_ATTACK, RUN_TORIGHT, RUN_TURNLEFT, SPAWN, SPAWN_TOIDLE, Q, Q_TOIDLE, Q_TORUN, W, W_ATTACK,
		R_MINUS180, R_MINUS45, R_MINUS90, R_0, R_180, R_45, R_90, R_INATTACK, R_TOIDLE, R_TORUN, STUN, TAUNT, ANIM_END
	};
private:
	CTemoo(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CTemoo(const CTemoo& rhs);
	virtual ~CTemoo() = default;

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
	static CTemoo* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free() override;
};

