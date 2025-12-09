#pragma once
#include "LeonaState.h"

NS_BEGIN(Client)
NS_BEGIN(Leona)
class CIdleState : public CLeonaState
{
public:
	CIdleState(class CLeona* pVayne);

	virtual CLeonaState* AI_Behaviour(_float fTimeDelta) override;
	virtual CLeonaState* Update(_float fTimeDelta) override;
	virtual CLeonaState* LateUpdate(_float fTimeDelta) override;

	virtual void Exit() override;
	virtual void Enter() override;

private:
	_float			m_fIdleTime;

};
NS_END
NS_END

