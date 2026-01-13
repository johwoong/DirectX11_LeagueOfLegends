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
    const float kInterp = 0.1f;

    // ===== Teemo만 시퀀스 처리 =====
    if (m_pOwner->Get_PlayerID() == CPlayer::TEEMO)
    {
        switch (m_eTeemoPhase)
        {
        case TEEMO_PHASE::JOKE_IN:
        {
            const bool bEnd = m_pOwner->Get_Model()->Play_Animation(fTimeDelta, false, nullptr, kInterp);
            if (bEnd)
            {
                m_eTeemoPhase = TEEMO_PHASE::JOKE_SPAM;
                m_fPhaseAcc = 0.f;
                m_pOwner->Get_Model()->Set_CurrentAnimIndex(CTemoo::ANIM::JOKE_SPAM);
            }
            break;
        }
        case TEEMO_PHASE::JOKE_SPAM:
        {
            // 루프 재생
            m_pOwner->Get_Model()->Play_Animation(fTimeDelta, true, nullptr, kInterp);

            // 일정 시간만 스팸 재생 후 OUT
            m_fPhaseAcc += fTimeDelta;
            if (m_fPhaseAcc >= m_fJokeSpamDuration)
            {
                m_eTeemoPhase = TEEMO_PHASE::JOKE_OUT;
                m_pOwner->Get_Model()->Set_CurrentAnimIndex(CTemoo::ANIM::JOKE_OUT);
            }
            break;
        }
        case TEEMO_PHASE::JOKE_OUT:
        {
            const bool bEnd = m_pOwner->Get_Model()->Play_Animation(fTimeDelta, false, nullptr, kInterp);
            if (bEnd)
            {
                // Idle로 돌아갈 때 Idle_In 먼저 재생
                m_pOwner->Set_PlayTeemoIdleIn(true);
                return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
            }
            break;
        }

        case TEEMO_PHASE::LAUGH:
        {
            const bool bEnd = m_pOwner->Get_Model()->Play_Animation(fTimeDelta, false, nullptr, kInterp);
            if (bEnd)
            {
                m_eTeemoPhase = TEEMO_PHASE::LAUGH_TOIDLE;
                m_pOwner->Get_Model()->Set_CurrentAnimIndex(CTemoo::ANIM::LAUGH_TOIDLE);
            }
            break;
        }
        case TEEMO_PHASE::LAUGH_TOIDLE:
        {
            const bool bEnd = m_pOwner->Get_Model()->Play_Animation(fTimeDelta, false, nullptr, kInterp);
            if (bEnd)
            {
                m_pOwner->Set_PlayTeemoIdleIn(true);
                return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
            }
            break;
        }

        case TEEMO_PHASE::TAUNT:
        {
            // 너 말대로 TAUNT는 크게 상관 없다고 했으니
            // 그냥 한 번 재생하고 Idle 복귀로 해둠(원하면 true 루프로 바꿔도 됨)
            const bool bEnd = m_pOwner->Get_Model()->Play_Animation(fTimeDelta, false, nullptr, kInterp);
            if (bEnd)
                return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
            break;
        }

        default:
            // 안전: 그냥 루프
            m_pOwner->Get_Model()->Play_Animation(fTimeDelta, true, nullptr, kInterp);
            break;
        }

        return nullptr;
    }

    // ===== Teemo 아닌 애들은 기존처럼 루프 =====
    m_bIsAnimationFinished =
        m_pOwner->Get_Model()->Play_Animation(
            fTimeDelta,
            true,
            nullptr,
            kInterp
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

    case CPlayer::LEESIN:
        switch (m_eType)
        {
        case EMOTE_JOKE:
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CLeeSin::ANIM::JOKE);
            break;
        case EMOTE_LAUGH:
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CLeeSin::ANIM::LAUGH);
            break;
        case EMOTE_TAUNT:
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CLeeSin::ANIM::TAUNT);
            break;
        }
        break;
    case CPlayer::TEEMO:
        m_fPhaseAcc = 0.f;
        m_eTeemoPhase = TEEMO_PHASE::NONE;

        switch (m_eType)
        {
        case EMOTE_JOKE:
            m_eTeemoPhase = TEEMO_PHASE::JOKE_IN;
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CTemoo::ANIM::JOKE_IN);
            break;

        case EMOTE_LAUGH:
            m_eTeemoPhase = TEEMO_PHASE::LAUGH;
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CTemoo::ANIM::LAUGH);
            break;

        case EMOTE_TAUNT:
            m_eTeemoPhase = TEEMO_PHASE::TAUNT;
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CTemoo::ANIM::TAUNT);
            break;
        }
        break;
    }
}

void CPlayerEmotionState::Exit(void)
{
    __super::Exit();
}
