#pragma once

#include "Vayne.h"
#include "Client_Defines.h"
#include "PoolManager.h"

NS_BEGIN(Client)
class CVayneState
{
public:
	enum STATE_ID
	{
		STATE_IDLE,
		STATE_MOVE,
		STATE_END
	};

	virtual ~CVayneState() {};
	virtual CVayneState* AI_Behaviour(_float fTimeDelta) { return nullptr; };
	virtual CVayneState* Update(_float fTimeDelta) PURE;
	virtual CVayneState* LateUpdate(_float fTimeDelta) PURE;


	virtual void Enter() PURE;
	virtual void Exit() PURE;

	CVayneState* ChangeState(CVayneState* pCurrentState, CVayneState* pNewState)
	{
		if (pCurrentState)
		{
			pCurrentState->Exit();
			Safe_Delete(pCurrentState);
		}

		pCurrentState = pNewState;
		pCurrentState->Enter();

		return pCurrentState;
	}

protected:
	STATETYPE m_eStateType = STATETYPE_DEFAULT;
	STATE_ID m_eStateId = STATE_END;

	_bool		m_bIsAnimationFinished = false;
	_bool		m_bAnimFinish = false;
	_float		m_fTimeDeltaAcc = 0.f;
	CVayne*		m_pOwner = nullptr;

	CActor* m_pOriginTarget = nullptr;
	CActor* m_pCurTarget = nullptr;
};
NS_END

