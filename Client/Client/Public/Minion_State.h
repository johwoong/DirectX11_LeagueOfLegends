#pragma once

#include "GameInstance.h"
#include "Client_Defines.h"
#include "Minion.h"
#include "MinionManager.h"


NS_BEGIN(Client)
class CMinionState
{
public:
	enum STATE_ID
	{
		STATE_IDLE,
		STATE_RUN,
		STATE_ATTACK,
		STATE_CHASE,
		STATE_SKILL,
		STATE_DEAD,
		STATE_END
	};

public:
	virtual ~CMinionState() {};

	virtual CMinionState* AI_Behaviour(_float fTimeDelta) PURE;
	virtual CMinionState* Update(_float fTimeDelta) PURE;
	virtual CMinionState* Late_Update(_float fTimeDelta) PURE;

	virtual void Enter() PURE;
	virtual void Exit() PURE;

public:
	CMinionState* ChangeState(CMinionState* pCurrentState, CMinionState* pNewState)
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

	STATE_ID Get_StateID() const { return m_eState; }
protected:
	CMinion* m_pOwner = nullptr;
	STATE_ID m_eState = STATE_IDLE;
	_uint m_ePlayerID = 0;

protected: // For Animation
	_bool m_bIsAnimationFinished = false;

};
NS_END



