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
    m_ePhase = PHASE_RECALL;
}

CPlayerState* CPlayerRecallState::HandleInput()
{
    CGameInstance* GI = CGameInstance::GetInstance();

    // Recall 도중 우클릭 → 이동 캔슬
    if (GI->Mouse_Down(DIMK_RBUTTON))
    {
        _float3 pos = CPickingManager::GetInstance()->Get_PickingPos();
        m_pOwner->Set_TargetPosition(pos);
        return new CRunState(m_pOwner);
    }

    return nullptr;
}

CPlayerState* CPlayerRecallState::Update(_float fTimeDelta)
{
    // ===============================
    // 애니메이션 재생 (프레임당 1회)
    // ===============================
    m_bIsAnimationFinished =
        m_pOwner->Get_Model()->Play_Animation(
            fTimeDelta,
            false,
            nullptr,
            0.1f
        );

    if (!m_bIsAnimationFinished)
        return nullptr;

    // ===============================
    // 애니 끝났을 때 처리
    // ===============================
    switch (m_ePhase)
    {
    case PHASE_RECALL:
    {
        // Ezreal: 기존 LeadOut
        if (m_ePlayerID == CPlayer::EZREAL)
        {
            m_ePhase = PHASE_WINDDOWN;
            m_bIsAnimationFinished = false;
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::RECALL_LEADOUT);
        }
        // LeeSin: WINDDOWN으로 자연스럽게
        else if (m_ePlayerID == CPlayer::LEESIN)
        {
            m_ePhase = PHASE_WINDDOWN;
            m_bIsAnimationFinished = false;
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CLeeSin::ANIM::WINDDOWN);
        }
        break;
    }

    case PHASE_WINDDOWN:
    {
        // 최종 종료
        m_pOwner->Respawon();
        return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
    }
    }

    return nullptr;
}

CPlayerState* CPlayerRecallState::Late_Update(_float)
{
    return nullptr;
}

void CPlayerRecallState::Enter()
{
    __super::Enter();
    m_eState = STATE_RECALL;

    m_bIsRecall = false;
    m_bIsAnimationFinished = false;
    m_ePhase = PHASE_RECALL;

    switch (m_ePlayerID)
    {
    case CPlayer::EZREAL:
        m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::RECALL);
        break;

    case CPlayer::LEESIN:
        m_pOwner->Get_Model()->Set_CurrentAnimIndex(CLeeSin::ANIM::RECALL);
        break;
    }
}

void CPlayerRecallState::Exit()
{
    __super::Exit();
}
