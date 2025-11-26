#pragma once
#include "VayneState.h"

NS_BEGIN(Client)
NS_BEGIN(Vayne)
class CIdleState : public CVayneState
{
public:
	CIdleState(class CVayne* pVayne);

	virtual CVayneState* AI_Behaviour(_float fTimeDelta) override;
	virtual CVayneState* Update(_float fTimeDelta) override;
	virtual CVayneState* LateUpdate(_float fTimeDelta) override;

	virtual void Enter() override;
	virtual void Exit() override;

private:
	_float			m_fIdleTime;

};
NS_END
NS_END

