#pragma once

#include "GameInstance.h"
#include "Client_Defines.h"
#include "PickingManager.h"
#include "Player.h"
#include "Ezreal.h"
#include "Renekton.h"
#include "Vi.h"
#include "Enemy.h"
#include "Projectile.h"

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
		STATE_ATTACK1,
		STATE_ATTACK2,
		STATE_ATTACK3,
		STATE_ATTACK4,
		STATE_SKILL_ATTACK_Q,
		STATE_SKILL_ATTACK_W,
		STATE_SKILL_ATTACK_E,
		STATE_SKILL_ATTACK_R,
		STATE_EMOTION,
		STATE_DEAD,
		STATE_CHASE,
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

	float GetDistanceXZ(_vector a, _vector b)
	{
		float ax = XMVectorGetX(a);
		float az = XMVectorGetZ(a);

		float bx = XMVectorGetX(b);
		float bz = XMVectorGetZ(b);

		float dx = ax - bx;
		float dz = az - bz;

		return sqrtf(dx * dx + dz * dz);
	}

	_float3 GetMouseDirection()
	{
		CGameInstance* GI = CGameInstance::GetInstance();

		// Picking에서 제공하는 월드 레이 방향
		_vector vDir = GI->Get_RayDir();

		// Y 평면 고정 (리그오브레전드 스타일)
		vDir = XMVectorSetY(vDir, 0);

		// 정규화
		vDir = XMVector3Normalize(vDir);

		_float3 out;
		XMStoreFloat3(&out, vDir);
		return out;
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




