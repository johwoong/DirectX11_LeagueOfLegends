#pragma once

#include "GameInstance.h"
#include "Client_Defines.h"
#include "Ezreal.h"
#include "Renekton.h"

NS_BEGIN(Engine)
class CModel;
NS_END

NS_BEGIN(Client)
class CPlayer;
class CPlayerState
{
public:
	enum STATE_ID
	{
		STATE_IDLE,
		STATE_RUN,
		STATE_ATTACK,
		STATE_HIT,
		STATE_SKILL,
		STATE_END
	};

public:
	virtual ~CPlayerState() = default;

	virtual CPlayerState* HandleInput() { return nullptr; }
	virtual CPlayerState* Update(_float fTimeDelta) { return nullptr; }
	virtual CPlayerState* Late_Update(_float fTimeDelta) { return nullptr; }

	virtual void Enter() {}
	virtual void Exit() {}
	
public:
	CPlayerState* ChangeState(CPlayerState* pCurrentState, CPlayerState* pNewState)
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
	CPlayer* m_pOwner = nullptr;
	STATE_ID m_eState = STATE_IDLE;
	_uint m_ePlayerID = 0;

protected: // For Animation
	_bool m_bIsAnimationFinished = false;

};
NS_END




