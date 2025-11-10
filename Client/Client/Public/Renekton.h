#pragma once
#include "Champion.h"
class CRenekton final : public CChampion
{
public:
	enum ANIM {
		ATTACK1, ATTACK2, CHANNEL, CHANNEL_WIND, CRIT,
		DANCE, DEATH, IDLE1, IDLE2, IDLE3, JOKE, LAUGH, RUN1, Q,
		W, W_RAGED, E, R, TAUNT, ANIM_END
	};
private:
	CRenekton(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CRenekton(const CRenekton& rhs);
	virtual ~CRenekton() = default;

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
	static CRenekton* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free() override;
};

