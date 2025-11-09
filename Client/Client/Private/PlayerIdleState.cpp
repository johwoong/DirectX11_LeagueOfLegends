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
    m_bIsAnimationFinished = m_pOwner->Get_Model()->Play_Animation(fTimeDelta, m_pOwner->Is_AnimationLoop(m_pOwner->Get_Model()->Get_CurrentAnimIndex()));
	return nullptr;
}

CPlayerState* CIdleState::Late_Update(_float fTimeDelta)
{
    if (m_bIsAnimationFinished)
    {
        switch (m_eIdleType)
        {
        case Client::Player::CIdleState::IDLE_MAIN:
        {
            switch (m_ePlayerID)
            {
            case CPlayer::EZREAL:
                m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::IDLE);
                break;
            }
        }
            break;
        }
    }
	return nullptr;
}


void CIdleState::Enter()
{
	__super::Enter();
	m_eState = STATE_ID::STATE_IDLE;

    switch (m_pOwner->Get_PlayerID())
    {
    case CPlayer::EZREAL:
        switch (m_eIdleType)
        {
        case Client::Player::CIdleState::IDLE_MAIN:
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::IDLE);
            break;
        }
        break;
    }
    
}


void CIdleState::Exit()
{
	__super::Exit();
}
