#include "PlayerRecallState.h"
#include "PlayerIdleState.h"
#include "PlayerRunState.h"
#include "PlayerEmotionState.h"

using namespace Player;

CPlayerRecallState::CPlayerRecallState(CPlayer* pPlayer)
{
    m_pOwner = pPlayer;
    m_ePlayerID = m_pOwner->Get_PlayerID();
    m_bIsRecall = false;
    m_bIsAnimationFinished = false;
}

CPlayerState* CPlayerRecallState::HandleInput(void)
{
    CGameInstance* pGameInstance = CGameInstance::GetInstance();

    if (pGameInstance->Key_Pressing(DIK_LCONTROL))
    {
        if (pGameInstance->Key_Down(DIK_1))
        {
            return new CPlayerEmotionState(m_pOwner, CPlayerEmotionState::EMOTE_JOKE);
        }
        if (pGameInstance->Key_Down(DIK_2))
        {
            return new CPlayerEmotionState(m_pOwner, CPlayerEmotionState::EMOTE_LAUGH);
        }
        if (pGameInstance->Key_Down(DIK_3))
        {
            return new CPlayerEmotionState(m_pOwner, CPlayerEmotionState::EMOTE_TAUNT);
        }
    }

    // 우클릭 → 바로 Run 상태로 전환
    if (pGameInstance->Mouse_Down(DIMK_RBUTTON))
    {
        _float3 vClickPos = {};

        vClickPos = CPickingManager::GetInstance()->Get_PickingPos();
        m_pOwner->Set_TargetPosition(vClickPos);

        return new CRunState(m_pOwner);

    }

    return nullptr;
}

CPlayerState* CPlayerRecallState::Update(_float fTimeDelta)
{
    m_bIsAnimationFinished =
        m_pOwner->Get_Model()->Play_Animation(
            fTimeDelta,
            false,   
            nullptr,
            0.1f
        );

    if (m_bIsAnimationFinished)
    {
        if (m_bIsRecall)
        {
            m_pOwner->Respawon();
            return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
        }
        m_bIsRecall = true;
        m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::RECALL_LEADOUT);
        
    }

    return nullptr;
}

CPlayerState* CPlayerRecallState::Late_Update(_float fTimeDelta)
{
    return nullptr;
}

void CPlayerRecallState::Enter(void)
{
    __super::Enter();
    m_eState = STATE_RECALL;

    // 들어갈 때도 깔끔하게 리셋
    m_bIsRecall = false;
    m_bIsAnimationFinished = false;

    m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::RECALL);
}

void CPlayerRecallState::Exit(void)
{
    __super::Exit();
}
