#include "PlayerDeadState.h"
#include "Player.h"
#include "Champion.h"
#include "PlayerIdleState.h"
#include "Collision_Manager.h"

// ★ 챔피언별 ANIM enum 쓰려면 포함
#include "Ezreal.h"
#include "LeeSin.h"
#include "Temoo.h"   // 너 프로젝트에서 CTemoo 정의된 헤더로 변경

using namespace Client;
using namespace Client::Player;

CPlayerDeadState::CPlayerDeadState(CPlayer* pPlayer)
{
    m_pOwner = pPlayer;
    m_ePlayerID = m_pOwner->Get_PlayerID();
}

CPlayerState* CPlayerDeadState::HandleInput()
{
    return nullptr;
}

void CPlayerDeadState::Enter()
{
    if (!m_pOwner) return;

    // 다운 상태
    m_pOwner->Set_Down(true);

    // 이동/타겟/스킬 정리
    m_pOwner->SetUsingSkill(false);
    m_pOwner->Set_AttackTarget(nullptr);

    // ★ 죽었을 때: AI가 절대 못잡게 COLLISION_PLAYER에서 빼버림
    CCollision_Manager::GetInstance()->Out_CollisionGroup(
        CCollision_Manager::COLLISION_PLAYER, m_pOwner);

    Start_DeathAnim();

    m_ePhase = PHASE_DEATH_ANIM;
    m_fWaitAcc = 0.f;
}

void CPlayerDeadState::Exit()
{
    // Respawn에서 복구
}

CPlayerState* CPlayerDeadState::Update(_float fTimeDelta)
{
    if (!m_pOwner) return nullptr;

    CModel* pModel = m_pOwner->Get_Model();
    if (!pModel) return nullptr;

    const _float kInterp = 0.1f;

    switch (m_ePhase)
    {
    case PHASE_DEATH_ANIM:
    {
        const _bool bEnd = pModel->Play_Animation(fTimeDelta, false, nullptr, kInterp);
        if (bEnd)
        {
            m_ePhase = PHASE_WAIT_RESPAWN;
            m_fWaitAcc = 0.f;
        }
        break;
    }

    case PHASE_WAIT_RESPAWN:
    {
        m_fWaitAcc += fTimeDelta;
        if (m_fWaitAcc >= 5.f)
        {
            Do_Respawn();
            Start_SpawnAnim();
            m_ePhase = PHASE_SPAWN_ANIM;
        }
        break;
    }

    case PHASE_SPAWN_ANIM:
    {
        const _bool bEnd = pModel->Play_Animation(fTimeDelta, false, nullptr, kInterp);
        if (bEnd)
        {
            m_pOwner->Set_Spawning(false);
            m_pOwner->Set_Down(false);

            CCollision_Manager::GetInstance()->Add_CollisionGroup(
                CCollision_Manager::COLLISION_PLAYER, m_pOwner);

            return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
        }
        break;
    }
    }

    return nullptr;
}

CPlayerState* CPlayerDeadState::Late_Update(_float fTimeDelta)
{
    UNREFERENCED_PARAMETER(fTimeDelta);
    return nullptr;
}

void CPlayerDeadState::Start_DeathAnim()
{
    if (!m_pOwner) return;
    CModel* pModel = m_pOwner->Get_Model();
    if (!pModel) return;

    // ★ EmotionState처럼 PlayerID로 정확히 박아준다 (여기가 핵심)
    switch (m_pOwner->Get_PlayerID())
    {
    case CPlayer::EZREAL:
        pModel->Set_CurrentAnimIndex(CEzreal::ANIM::TAUNT);   // 네 enum 이름이 DIE면 DIE로
        break;

    case CPlayer::LEESIN:
        pModel->Set_CurrentAnimIndex(CLeeSin::ANIM::DEATH);
        break;

    case CPlayer::TEEMO:
        pModel->Set_CurrentAnimIndex(CTemoo::ANIM::DEATH);
        break;

    default:
        // 혹시 다른 챔피언은 기존 함수 사용
        pModel->Set_CurrentAnimIndex(m_pOwner->Get_DeathAnim());
        break;
    }
}

void CPlayerDeadState::Do_Respawn()
{
    if (!m_pOwner) return;

    if (CChampion* pChamp = dynamic_cast<CChampion*>(m_pOwner))
        pChamp->Revive_Full();

    m_pOwner->Respawon();

    // ★ Spawn 애니 시작
    m_pOwner->Set_Spawning(true);
}

void CPlayerDeadState::Start_SpawnAnim()
{
    if (!m_pOwner) return;
    CModel* pModel = m_pOwner->Get_Model();
    if (!pModel) return;

    // 스폰 애니가 챔피언별로 다르면 여기서도 PlayerID로 박아도 됨
    // 일단 너가 이미 쓰는 Get_SpawnAnim() 유지
    pModel->Set_CurrentAnimIndex(m_pOwner->Get_SpawnAnim());
}