#pragma once
#include "RenektonState.h"

NS_BEGIN(Client)
NS_BEGIN(Renekton)
class CIdleState : public CRenektonState
{
public:
	CIdleState(class CRenekton* pRenekton);

	virtual CRenektonState* AI_Behaviour(_float fTimeDelta) override;
	virtual CRenektonState* Update(_float fTimeDelta) override;
	virtual CRenektonState* LateUpdate(_float fTimeDelta) override;

	virtual void Enter() override;
	virtual void Exit() override;

private:
	_float			m_fIdleTime;

};
NS_END
NS_END

