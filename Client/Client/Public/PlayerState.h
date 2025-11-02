#pragma once

#include "GameInstance.h"
#include "Ezreal.h"
#include "Renekton.h"

NS_BEGIN(Engine)
class CModel;
NS_END

NS_BEGIN(Client)
class CPlayerState
{
public:
	enum STATE{ STATE_IDLE, STATE_WALK,STATE_RUN,STATE_END};

	STATE Get_State() { return m_eState; }
	_float Get_Time(void) { return m_fTime; }
	CPlayer* Get_Owner(void) { return m_pOwner; }

	virtual CPlayerState* HandleInput() { return nullptr; };
	virtual CPlayerState* Update(_float fTimeDelta) = 0;
	virtual CPlayerState* Late_Update(_float fTimeDelta) = 0;
	virtual ~CPlayerState() {};

	virtual void Enter() // 상태 진입
	{
		
	}

	virtual void Exit() // 상태 나가기
	{

	}

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

protected:
	STATE m_eState = STATE_END;
	CPlayer* m_pOwner = { nullptr };
	
	_bool m_bIsFly = false;
	_uint m_ePlayerID = 0;
	_float m_fTime = 0.f;
};
NS_END




