#pragma once
#include "Champion.h"

NS_BEGIN(Client)
class CEzreal final : public CChampion
{
public:
	enum ANIM {
		IDLE, ATTACK1, ATTACK2, ATTACK3, ATTACK4, CHANNEL, CHANNEL_WIND,
		CRIT, DANCE, DANCE_WIND, E_MINUS_180, E_MINUS_90, E180, E90, E_EXIT,
		E_EXITNOTARGET, E_EXIT_NOTARGET_IDLE, E_EXIT_NOTARGET_RUN, E_EXITRUN,
		E_GENERIC, IDLE01, IDLE2, IDLEQ, IDLEW, JOKE, LAUGH, Q, R, R_IDLE, R_RUN, RECALL,
		RECALL_LEADOUT, RECALL_WIND_DOWN, RUN, RUN_HOME_GUARD, RUN_TOIDLE, RUNQ,
		RUNW, TAUNT, W, ANIM_END
	};
private:
	CEzreal(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CEzreal(const CEzreal& rhs);
	virtual ~CEzreal() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);

public:
	virtual _bool Is_AnimationLoop(_uint eAnimId) override;

private:
	virtual void SetSkillInfo() override;

private:
	virtual HRESULT Ready_Components(void* pArg) override;

public:
	static CEzreal* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free() override;
};
NS_END

