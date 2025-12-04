#include "PlayerEmotionState.h"
#include "PlayerIdleState.h"
#include "PlayerRunState.h"

using namespace Player;

CPlayerEmotionState::CPlayerEmotionState(CPlayer* pPlayer, EMOTION_TYPE eType)
{
    m_pOwner = pPlayer;
    m_eType = eType;
    m_ePlayerID = m_pOwner->Get_PlayerID();
}

CPlayerState* CPlayerEmotionState::HandleInput(void)
{
    CGameInstance* pGameInstance = CGameInstance::GetInstance();

    // Ctrl + 1/2/3 이모션 재입력 → 같은 이모션도 처음부터 다시 재생
    if (pGameInstance->Key_Pressing(DIK_LCONTROL))
    {
        if (pGameInstance->Key_Down(DIK_1))
        {
            return new CPlayerEmotionState(m_pOwner, EMOTE_JOKE);
        }
        if (pGameInstance->Key_Down(DIK_2))
        {
            return new CPlayerEmotionState(m_pOwner, EMOTE_LAUGH);
        }
        if (pGameInstance->Key_Down(DIK_3))
        {
            return new CPlayerEmotionState(m_pOwner, EMOTE_TAUNT);
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

CPlayerState* CPlayerEmotionState::Update(_float fTimeDelta)
{
    // 이모션은 계속 반복 재생
    m_bIsAnimationFinished =
        m_pOwner->Get_Model()->Play_Animation(
            fTimeDelta,
            true,      // ← 루프 재생
            nullptr,
            0.1f
        );

    return nullptr;
}

CPlayerState* CPlayerEmotionState::Late_Update(_float fTimeDelta)
{
    return nullptr;
}

void CPlayerEmotionState::Enter(void)
{
    __super::Enter();
    m_eState = STATE_EMOTION;

    switch (m_ePlayerID)
    {
    case CPlayer::EZREAL:
        switch (m_eType)
        {
        case EMOTE_JOKE:
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::JOKE);
            break;
        case EMOTE_LAUGH:
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::DANCE);
            break;
        case EMOTE_TAUNT:
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::DANCE_WIND);
            break;
        }
        break;

    case CPlayer::VI:
        switch (m_eType)
        {
        case EMOTE_JOKE:
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CVi::ANIM::DANCE);
            break;
        case EMOTE_LAUGH:
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CVi::ANIM::LAUGH);
            break;
        case EMOTE_TAUNT:
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CVi::ANIM::TAUNT);
            break;
        }
        break;
    }
}

void CPlayerEmotionState::Exit(void)
{
    __super::Exit();
}
