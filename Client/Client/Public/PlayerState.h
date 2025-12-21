#pragma once

#include "GameInstance.h"
#include "Client_Defines.h"
#include "PickingManager.h"
#include "Player.h"
#include "Ezreal.h"
#include "Renekton.h"
#include "Vi.h"
#include "Enemy.h"
#include "EzrealProjectile.h"

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
		STATE_RECALL,
		STATE_DEAD,
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



	static _float3 Compute_AttackMoveDest(CPlayer* pOwner, CActor* pTarget)
	{
		_float3 result{};

		CChampion* pChamp = dynamic_cast<CChampion*>(pOwner);
		if (!pChamp || !pTarget)
			return result;

		CTransform* pTrOwner = pOwner->Get_Transform();
		CTransform* pTrTar = pTarget->Get_Transform();
		if (!pTrOwner || !pTrTar)
			return result;

		_vector myPos = pTrOwner->Get_State(CTransform::STATE_TRANSLATION);
		_vector tarPos = pTrTar->Get_State(CTransform::STATE_TRANSLATION);

		CNavigation* pNavi = pOwner->Get_Navigation();

		// XZ 방향만
		_vector dir = tarPos - myPos;
		dir = XMVectorSetY(dir, 0.f);

		if (XMVector3Equal(dir, XMVectorZero()))
		{
			if (pNavi)
			{
				float h = pNavi->Compute_Height(myPos, 0.f);
				myPos = XMVectorSetY(myPos, h);
			}
			XMStoreFloat3(&result, myPos);
			return result;
		}

		dir = XMVector3Normalize(dir);

		// 실제 타겟까지 거리
		float distToTarget = XMVectorGetX(
			XMVector3Length(XMVectorSetY(tarPos - myPos, 0.f)));

		float range = pChamp->Get_WorldAttackRange();

		_vector bestDest = myPos;
		bool bFound = false;

		for (float ratio = 0.9f; ratio >= 0.3f; ratio -= 0.15f)
		{
			float approachDist = max(range * ratio, 0.1f);

			// 혹시라도 사거리*ratio가 실제 거리보다 커지지 않게
			approachDist = min(approachDist, distToTarget - 0.1f); // 살짝 여유

			if (approachDist <= 0.f)
				continue;

			_vector dest = tarPos - dir * approachDist;

			if (pNavi)
			{
				float h = pNavi->Compute_Height(dest, 0.f);
				dest = XMVectorSetY(dest, h);

				if (pNavi->isMove(dest))
				{
					bestDest = dest;
					bFound = true;
					break;
				}
			}
			else
			{
				bestDest = dest;
				bFound = true;
				break;
			}
		}

		if (!bFound)
		{
			_vector cur = myPos;
			if (pNavi)
			{
				float h = pNavi->Compute_Height(cur, 0.f);
				cur = XMVectorSetY(cur, h);
			}
			bestDest = cur;
		}

		XMStoreFloat3(&result, bestDest);
		return result;
	}

	static void Set_AttackMoveToTarget(CPlayer* pOwner, CActor* pTarget)
	{
		if (!pOwner || !pTarget) return;

		_float3 dest = Compute_AttackMoveDest(pOwner, pTarget);

		CNavigation* pNavi = pOwner->Get_Navigation();
		CTransform* pTr = pOwner->Get_Transform();
		if (!pNavi || !pTr) return;

		pOwner->Set_TargetPosition(dest);
		pTr->Set_Destination(XMLoadFloat3(&dest), pNavi);
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




