#pragma once
#include "AlistarState.h"

NS_BEGIN(Client)
NS_BEGIN(Alistar)
class CIdleState : public CAlistarState
{
public:
	CIdleState(class CAlistar* pVayne);

	virtual CAlistarState* AI_Behaviour(_float fTimeDelta) override;
	virtual CAlistarState* Update(_float fTimeDelta) override;
	virtual CAlistarState* LateUpdate(_float fTimeDelta) override;

	virtual void Exit() override;
	virtual void Enter() override;

private:
	_float			m_fIdleTime;

};
NS_END
NS_END

