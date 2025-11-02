#include "PlayerIdleState.h"
#include "GameInstance.h"
#include "PickingManager.h"

// State
#include "PlayerRunState.h"

using namespace Player;

CIdleState::CIdleState(CPlayer* pPlayer, IDLETYPE eIdleType)
{
	m_pOwner = pPlayer;
	m_eIdleType = eIdleType;
	m_ePlayerID = m_pOwner->Get_PlayerID();
}

CPlayerState* CIdleState::HandleInput()
{
	return nullptr;
}

CPlayerState* CIdleState::Update(_float fTimeDelta)
{
	CGameInstance* pGameInstance = CGameInstance::GetInstance();
	pGameInstance->AddRef();
	if (CPickingManager::GetInstance()->Picking())
	{
		_float3 vClickPos = CPickingManager::GetInstance()->Get_PickingPos();
		m_pOwner->Set_TargetPosition(vClickPos);
	}

	pGameInstance->Release();

	return nullptr;
}

CPlayerState* CIdleState::Late_Update(_float fTimeDelta)
{
	return nullptr;
}


void CIdleState::Enter()
{
	__super::Enter();
	m_eState = STATE::STATE_IDLE;
	switch (m_pOwner->Get_PlayerID())
	{
	case CPlayer::EZREAL:
		break;
	}
}

void CIdleState::Exit()
{
	__super::Exit();
}
