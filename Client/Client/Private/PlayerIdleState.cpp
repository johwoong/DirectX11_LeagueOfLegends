#include "PlayerIdleState.h"

// State
#include "PlayerRunState.h"
#include "PlayerEmotionState.h"
#include "EzrealAttackState.h"
#include "EzrealSkillStateQ.h"
#include "EzrealSkillStateW.h"
#include "EzrealSkillStateE.h"
#include "EzrealSkillStateR.h"
#include "PlayerRecallState.h"

using namespace Player;

CIdleState::CIdleState(CPlayer* pPlayer, IDLETYPE eIdleType)
{
	m_pOwner = pPlayer;
	m_eIdleType = eIdleType;
	m_ePlayerID = m_pOwner->Get_PlayerID();
}

CPlayerState* CIdleState::HandleInput()
{
    CGameInstance* GI = CGameInstance::GetInstance();
    CPickingManager* PM = CPickingManager::GetInstance();

    if (GI->Key_Down(DIK_Q))
    {
        if (m_pOwner->Get_PlayerID() == CPlayer::EZREAL)
            return new CEzrealSkillStateQ(m_pOwner);
    }
    if (GI->Key_Down(DIK_W))
    {
        if (m_pOwner->Get_PlayerID() == CPlayer::EZREAL)
            return new CEzrealSkillStateW(m_pOwner, CEzrealSkillStateW::CAST_IDLE);
    }
    if (GI->Key_Down(DIK_E))
    {
        if (m_pOwner->Get_PlayerID() == CPlayer::EZREAL)
            return new CEzrealSkillStateE(m_pOwner);
    }
    if (GI->Key_Down(DIK_R))
    {
        if (m_pOwner->Get_PlayerID() == CPlayer::EZREAL)
            return new CEzrealSkillStateR(m_pOwner);
    }


    if (GI->Key_Pressing(DIK_LCONTROL))
    {
        if (GI->Key_Down(DIK_1))
            return new CPlayerEmotionState(m_pOwner, CPlayerEmotionState::EMOTE_JOKE);

        if (GI->Key_Down(DIK_2))
            return new CPlayerEmotionState(m_pOwner, CPlayerEmotionState::EMOTE_LAUGH);

        if (GI->Key_Down(DIK_3))
            return new CPlayerEmotionState(m_pOwner, CPlayerEmotionState::EMOTE_TAUNT);
    }

    if (GI->Key_Pressing(DIK_B))
    {
        return new CPlayerRecallState(m_pOwner);
    }


    if (GI->Mouse_Down(DIMK_RBUTTON))
    {
        CGameObject* pickedObj = PM->Get_PickedObj();
        _float3 clickPos = PM->Get_PickingPos();

        
        if (m_pOwner->Get_AttackTarget() != nullptr)
        {
            CActor* pEnemy = m_pOwner->Get_AttackTarget();
            // 적 실제 좌표만 사용
            _float3 enemyPos;
            XMStoreFloat3(&enemyPos,
               pEnemy->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION));

            float dist = GetDistanceXZ(
                m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION),
                pEnemy->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION));

            float range = dynamic_cast<CChampion*>(m_pOwner)->Get_WorldAttackRange();

            // 사거리 안 바로 공격
            if (dist <= range)
                return new CEzrealAttackState(m_pOwner, STATE_ATTACK1);

            // 사거리  추격
            m_pOwner->Set_TargetPosition(enemyPos);

            return new CRunState(m_pOwner);
        }


        m_pOwner->Set_AttackTarget(nullptr);
        m_pOwner->Set_TargetPosition(clickPos);

        return new CRunState(m_pOwner);
    }

    return nullptr;
}

CPlayerState* CIdleState::Update(_float fTimeDelta)
{
    _uint curAnim = m_pOwner->Get_Model()->Get_CurrentAnimIndex();

    // ? Ezreal IdleQ일 때만 애니메이션 속도 빠르게
    if (m_pOwner->Get_PlayerID() == CPlayer::EZREAL &&
        curAnim == CEzreal::ANIM::IDLEQ)
    {
        fTimeDelta *= 2.f; 
    }

    m_bIsAnimationFinished =
        m_pOwner->Get_Model()->Play_Animation(
            fTimeDelta,
            m_pOwner->Is_AnimationLoop(curAnim),
            nullptr,
            0.1f
        );

    return nullptr;
}

CPlayerState* CIdleState::Late_Update(_float fTimeDelta)
{
    if (m_bIsAnimationFinished)
    {
        // Ezreal IdleQ → Idle1/Idle2 전환
        if (m_pOwner->Get_Model()->Get_CurrentAnimIndex() == CEzreal::ANIM::IDLEQ)
        {
            int idx = rand() % m_vecIdleAnim.size();
            _uint eAnim = m_vecIdleAnim[idx];
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(eAnim);
            return nullptr;
        }

        // 다른 Idle도 기존처럼 랜덤 변경
        int idx = rand() % m_vecIdleAnim.size();
        _uint eAnim = m_vecIdleAnim[idx];
        m_pOwner->Get_Model()->Set_CurrentAnimIndex(eAnim);
    }

    return nullptr;
}


void CIdleState::Enter()
{
    __super::Enter();
    m_eState = STATE_ID::STATE_IDLE;

    m_vecIdleAnim.clear();

    // Ezreal 후딜 IdleQ
    if (m_pOwner->Get_PlayerID() == CPlayer::EZREAL &&
        m_pOwner->Get_PlayIdleQ())
    {
        m_pOwner->Set_PlayIdleQ(false);

        // Idle 이후 랜덤 돌아갈 Idle 목록 미리 넣기
        m_vecIdleAnim = {
            CEzreal::ANIM::IDLE01,
            CEzreal::ANIM::IDLE2
        };

        m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::IDLEQ);
        return;
    }

    // ===== 기존 Idle 랜덤 로직 =====
    switch (m_pOwner->Get_PlayerID())
    {
    case CPlayer::EZREAL:
        m_vecIdleAnim = {
            CEzreal::ANIM::IDLE01,
            CEzreal::ANIM::IDLE2
        };
        break;

    case CPlayer::VI:
        m_vecIdleAnim = {
            CVi::ANIM::IDLE1,
            CVi::ANIM::IDLE2
        };
        break;
    }

    int idx = rand() % m_vecIdleAnim.size();
    m_pOwner->Get_Model()->Set_CurrentAnimIndex(m_vecIdleAnim[idx]);
    
}


void CIdleState::Exit()
{
	__super::Exit();
}
