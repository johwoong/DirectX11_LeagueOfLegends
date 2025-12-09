#pragma once
#include "Leona.h"
#include "Client_Defines.h"

NS_BEGIN(Client)
class CLeonaState
{
public:
	enum STATE_ID
	{
		STATE_IDLE,
		STATE_MOVE,
		STATE_END
	};

	virtual ~CLeonaState() {};
	virtual CLeonaState* AI_Behaviour(_float fTimeDelta) { return nullptr; };
	virtual CLeonaState* Update(_float fTimeDelta) PURE;
	virtual CLeonaState* LateUpdate(_float fTimeDelta) PURE;


	virtual void Enter() PURE;
	virtual void Exit() PURE;

	CLeonaState* ChangeState(CLeonaState* pCurrentState, CLeonaState* pNewState)
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
	CLeona* m_pOwner = nullptr;

	CActor* m_pOriginTarget = nullptr;
	CActor* m_pCurTarget = nullptr;
};
NS_END
