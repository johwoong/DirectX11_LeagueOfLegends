#pragma once
#include "Actor.h"
#include "GameInstance.h"

NS_BEGIN(Engine)
class CModel;
class CTexture;
NS_END

NS_BEGIN(Client)
class CMinion abstract: public CActor 
{
	typedef struct tagMininonStats
	{
		_float m_fMaxHp = 100.f;
		_float m_fCurrentHp = 10.f;
		_float m_fAttackPower = 10.f;
		_float m_fWalkSpeed = 3.f;
	};
};
NS_END

