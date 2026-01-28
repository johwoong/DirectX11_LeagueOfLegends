#include "PlayerEmotionState.h"
#include "PlayerIdleState.h"
#include "PlayerRunState.h"
#include "GameInstance.h"
#include "PickingManager.h"

using namespace Player;

CPlayerEmotionState::CPlayerEmotionState(CPlayer* pPlayer, EMOTION_TYPE eType)
{
    m_pOwner = pPlayer;
    m_eType = eType;
    m_ePlayerID = m_pOwner->Get_PlayerID();
}

const TCHAR* CPlayerEmotionState::GetEmotionSoundFile() const
{
    // ? 챔피언마다 파일명 다르면 여기만 관리하면 끝
    switch (m_ePlayerID)
    {
    case CPlayer::EZREAL:
        switch (m_eType)
        {
        case EMOTE_JOKE:  return TEXT("EzrealEmotion1.wav");
        case EMOTE_LAUGH: return TEXT("EzrealEmotion2.wav");
        case EMOTE_TAUNT: return TEXT("EzrealEmotion3.wav");
        }
        break;

    case CPlayer::LEESIN:
        switch (m_eType)
        {
        case EMOTE_JOKE:  return TEXT("397940538.wav");
        case EMOTE_LAUGH: return TEXT("LeeSinEmotionSound1.wav");
        case EMOTE_TAUNT: return TEXT("333082896.wav");
        }
        break;

    case CPlayer::TEEMO:
        switch (m_eType)
        {
        case EMOTE_JOKE:  return TEXT("TemooEmotion1.wav");
        case EMOTE_LAUGH: return TEXT("441309525.wav");
        case EMOTE_TAUNT: return TEXT("TemooEmotion3.wav");
        }
        break;
    }

    return nullptr;
}

void CPlayerEmotionState::PlayEmotionSound()
{
    const TCHAR* pFile = GetEmotionSoundFile();
    if (!pFile)
        return;

    // ? 기존 이모션 사운드가 남아있을 수 있으니 채널 먼저 끊고 재생
    StopEmotionSound();

    CGameInstance::GetInstance()->PlaySounds((TCHAR*)pFile, SOUND_EMOTION, 0.9f);
}

void CPlayerEmotionState::StopEmotionSound()
{
    // ? 플래그 필요 없음: 채널 끊으면 끝
    CGameInstance::GetInstance()->StopSound(SOUND_EMOTION);
}

CPlayerState* CPlayerEmotionState::HandleInput(void)
{
    CGameInstance* GI = CGameInstance::GetInstance();

    // Ctrl + 1/2/3 재입력 → 새 이모션 상태로 (StateMachine이 delete 해줘야 함)
    if (GI->Key_Pressing(DIK_LCONTROL))
    {
        if (GI->Key_Down(DIK_1)) return new CPlayerEmotionState(m_pOwner, EMOTE_JOKE);
        if (GI->Key_Down(DIK_2)) return new CPlayerEmotionState(m_pOwner, EMOTE_LAUGH);
        if (GI->Key_Down(DIK_3)) return new CPlayerEmotionState(m_pOwner, EMOTE_TAUNT);
    }

    // 우클릭 → Run 전환
    if (GI->Mouse_Down(DIMK_RBUTTON))
    {
        _float3 clickPos = CPickingManager::GetInstance()->Get_PickingPos();
        m_pOwner->Set_TargetPosition(clickPos);
        return new CRunState(m_pOwner);
    }

    return nullptr;
}

CPlayerState* CPlayerEmotionState::Update(_float fTimeDelta)
{
    const float kInterp = 0.1f;

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
            m_pOwner->Get_Model()->Play_Animation(fTimeDelta, true, nullptr, kInterp);

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
            const bool bEnd = m_pOwner->Get_Model()->Play_Animation(fTimeDelta, false, nullptr, kInterp);
            if (bEnd)
                return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
            break;
        }
        default:
            m_pOwner->Get_Model()->Play_Animation(fTimeDelta, true, nullptr, kInterp);
            break;
        }

        return nullptr;
    }

    // Teemo 아닌 애들 루프
    m_pOwner->Get_Model()->Play_Animation(fTimeDelta, true, nullptr, kInterp);
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

    // ? Enter에서 사운드 재생
    PlayEmotionSound();

    switch (m_ePlayerID)
    {
    case CPlayer::EZREAL:
        switch (m_eType)
        {
        case EMOTE_JOKE:  m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::JOKE);       break;
        case EMOTE_LAUGH: m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::DANCE);      break;
        case EMOTE_TAUNT: m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::DANCE_WIND); break;
        }
        break;

    case CPlayer::LEESIN:
        switch (m_eType)
        {
        case EMOTE_JOKE:  m_pOwner->Get_Model()->Set_CurrentAnimIndex(CLeeSin::ANIM::JOKE);  break;
        case EMOTE_LAUGH: m_pOwner->Get_Model()->Set_CurrentAnimIndex(CLeeSin::ANIM::LAUGH); break;
        case EMOTE_TAUNT: m_pOwner->Get_Model()->Set_CurrentAnimIndex(CLeeSin::ANIM::TAUNT); break;
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
    // ? Exit에서 무조건 끊기
    StopEmotionSound();

    __super::Exit();
}

