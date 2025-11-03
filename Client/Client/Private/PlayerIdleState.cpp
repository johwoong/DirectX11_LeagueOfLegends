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
    CGameInstance* pGameInstance = CGameInstance::GetInstance();

    if (pGameInstance->Mouse_Down(DIMK_RBUTTON))
    {
        _float3 vClickPos = {};

        if (CPickingManager::GetInstance()->Picking())
        {
            vClickPos = CPickingManager::GetInstance()->Get_PickingPos();
            m_pOwner->Set_TargetPosition(vClickPos);

            return new CRunState(m_pOwner);
        }
    }

    return nullptr; // 입력 없음
}

CPlayerState* CIdleState::Update(_float fTimeDelta)
{
	return nullptr;
}

CPlayerState* CIdleState::Late_Update(_float fTimeDelta)
{
	return nullptr;
}


void CIdleState::Enter()
{
	__super::Enter();
	m_eState = STATE_ID::STATE_IDLE;
}


void CIdleState::Exit()
{
	__super::Exit();
}
