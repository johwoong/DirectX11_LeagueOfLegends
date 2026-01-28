// ===============================
// PlayerRecallState.cpp (전체 수정본)
// ===============================
#include "PlayerRecallState.h"
#include "PlayerIdleState.h"
#include "PlayerRunState.h"
#include "PlayerEmotionState.h"

#include "GameInstance.h"
#include "PickingManager.h"
#include "ParticleSystem.h"   // StopRecallFX에서 dynamic_cast 사용

#include "Ezreal.h"
#include "LeeSin.h"
#include "Temoo.h"

using namespace Player;

CPlayerRecallState::CPlayerRecallState(CPlayer* pPlayer)
{
    m_pOwner = pPlayer;
    m_ePlayerID = m_pOwner->Get_PlayerID();

    m_bIsRecall = false;
    m_bIsAnimationFinished = false;
    m_ePhase = PHASE_RECALL;

    m_bRecallStartFx = false;
    m_bRecallFinishSfx = false;
    m_bFinishing = false;
}

CPlayerState* CPlayerRecallState::HandleInput()
{
    CGameInstance* GI = CGameInstance::GetInstance();

    // Recall 도중 우클릭 → 이동 캔슬
    if (GI->Mouse_Down(DIMK_RBUTTON))
    {
        CancelRecall();

        _float3 pos = CPickingManager::GetInstance()->Get_PickingPos();
        m_pOwner->Set_TargetPosition(pos);
        return new CRunState(m_pOwner);
    }

    return nullptr;
}

CPlayerState* CPlayerRecallState::Update(_float fTimeDelta)
{
    // 애니메이션 재생(프레임당 1회)
    m_bIsAnimationFinished =
        m_pOwner->Get_Model()->Play_Animation(
            fTimeDelta,
            false,
            nullptr,
            0.1f
        );

    if (!m_bIsAnimationFinished)
        return nullptr;

    // 애니 끝났을 때 처리
    switch (m_ePhase)
    {
    case PHASE_RECALL:
    {
        // 다음 마무리 단계로 넘어감
        m_ePhase = PHASE_WINDDOWN;
        m_bIsAnimationFinished = false;

        if (m_ePlayerID == CPlayer::EZREAL)
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::RECALL_LEADOUT);
        else if (m_ePlayerID == CPlayer::LEESIN)
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CLeeSin::ANIM::WINDDOWN);
        else if (m_ePlayerID == CPlayer::TEEMO)
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CTemoo::ANIM::SPAWN);

        break;
    }

    case PHASE_WINDDOWN:
    {
        // ★ 여기서 완료 처리(Recall2 + FX 종료)
        FinishRecall();

        // 리스폰
        m_pOwner->Respawon();

        // 상태 변경(이때 Exit 호출될 수 있음)
        return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
    }
    }

    return nullptr;
}

CPlayerState* CPlayerRecallState::Late_Update(_float)
{
    return nullptr;
}

void CPlayerRecallState::CancelRecall()
{
    CGameInstance* GI = CGameInstance::GetInstance();

    // 완료 플래그 끄기(캔슬은 Exit에서 진행음 끊어도 됨)
    m_bFinishing = false;

    // 진행 사운드/완료 사운드 전부 끊기
    GI->StopSound(SOUND_RECALL);
    GI->StopSound(SOUND_RECALL_FINISH);

    StopRecallFX();

    m_bRecallStartFx = false;
    m_bRecallFinishSfx = false;
}

void CPlayerRecallState::FinishRecall()
{
    CGameInstance* GI = CGameInstance::GetInstance();

    // ★ 완료로 빠져나가는 중(Exit에서 SOUND_RECALL_FINISH는 끊지 않게)
    m_bFinishing = true;

    // 진행음 끊기
    GI->StopSound(SOUND_RECALL);

    // FX 종료
    StopRecallFX();

    // 완료음 1회 (★ 반드시 SOUND_RECALL_FINISH 채널로)
    if (!m_bRecallFinishSfx)
    {
        m_bRecallFinishSfx = true;
        GI->PlaySounds(const_cast<TCHAR*>(L"Recall2.wav"), SOUND_RECALL_FINISH, 0.6f);
    }
}

void CPlayerRecallState::Enter()
{
    __super::Enter();
    m_eState = STATE_RECALL;

    m_bIsRecall = false;
    m_bIsAnimationFinished = false;
    m_ePhase = PHASE_RECALL;

    m_bRecallStartFx = false;
    m_bRecallFinishSfx = false;
    m_bFinishing = false;
    m_RecallFX.clear();

    // 애니 세팅
    switch (m_ePlayerID)
    {
    case CPlayer::EZREAL:  m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::RECALL); break;
    case CPlayer::LEESIN:  m_pOwner->Get_Model()->Set_CurrentAnimIndex(CLeeSin::ANIM::RECALL); break;
    case CPlayer::TEEMO:   m_pOwner->Get_Model()->Set_CurrentAnimIndex(CTemoo::ANIM::RECALL); break;
    }

    CGameInstance* GI = CGameInstance::GetInstance();

    // 혹시 이전에 남아있을 수 있으니 정리
    GI->StopSound(SOUND_RECALL);
    GI->StopSound(SOUND_RECALL_FINISH);

    // 1) Recall1 사운드(리콜 중) - 진행 채널
    GI->PlaySounds(const_cast<TCHAR*>(L"Recall1.wav"), SOUND_RECALL, 0.6f);

    // 2) Recall FX 스폰
    _matrix mWorld = m_pOwner->Get_Transform()->Get_WorldMatrix();

    _vector pos = mWorld.r[3];

    // 위로
    pos += XMVectorSet(0.f, 1.0f, 0.f, 0.f);

    // ★ 오른쪽으로 (플레이어 기준)
    _vector vRight = mWorld.r[0];               // STATE_RIGHT
    vRight = XMVector3Normalize(vRight);
    pos += vRight * 0.1f;                        // ← 0.2 ~ 0.4 사이에서 조절

    mWorld.r[3] = XMVectorSetW(pos, 1.f);

    m_RecallFX = CEffect::PlayEffectAtLocation(TEXT("Recall.dat"), mWorld);


    // 필요하면 플레이어에 붙이기
    for (auto& fx : m_RecallFX)
        if (fx) fx->Set_AttachObject(m_pOwner);

    m_bRecallStartFx = true;
}

void CPlayerRecallState::StopRecallFX()
{
    if (m_RecallFX.empty())
        return;

    for (auto& fx : m_RecallFX)
    {
        if (!fx) continue;

        if (auto ps = dynamic_cast<CParticleSystem*>(fx))
            ps->Set_Stop(true);

        fx->Set_Dead(true);
    }

    m_RecallFX.clear();
}

void CPlayerRecallState::Exit()
{
    CGameInstance* GI = CGameInstance::GetInstance();

    // 진행음은 항상 끊기
    GI->StopSound(SOUND_RECALL);

    // ★ 완료로 나가는 Exit이면 Recall2는 끊지 않는다
    if (!m_bFinishing)
        GI->StopSound(SOUND_RECALL_FINISH);

    StopRecallFX();

    __super::Exit();
}

