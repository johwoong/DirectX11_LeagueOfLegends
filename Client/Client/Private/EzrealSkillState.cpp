#include "EzrealSkillState.h"

using namespace Player;
CEzrealSkillState::CEzrealSkillState(CPlayer* pPlayer, STATE_ID eStateType)
{
	m_eState = eStateType;
	m_pOwner = pPlayer;
}

CPlayerState* CEzrealSkillState::HandleInput()
{
	return nullptr;
}

CPlayerState* CEzrealSkillState::Update(_float fTimeDelta)
{
	return nullptr;
}

CPlayerState* CEzrealSkillState::Late_Update(_float fTimeDelta)
{
	return nullptr;
}

void CEzrealSkillState::Enter()
{
}

void CEzrealSkillState::Exit()
{
}
