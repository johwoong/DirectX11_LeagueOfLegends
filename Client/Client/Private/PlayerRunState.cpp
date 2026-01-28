#include "PlayerRunState.h"
#include "PlayerIdleState.h"
#include "Terrian.h"
#include "EzrealAttackState.h"
#include "EzrealSkillStateQ.h"
#include "EzrealSkillStateW.h"
#include "EzrealSkillStateE.h"
#include "EzrealSkillStateR.h"
#include "PlayerRecallState.h"
#include "LeeSinAttackState.h"
#include "LeeSinSkillStateQ.h"
#include "TemooSkillStateW.h"
#include "Temoo.h"
#include "Champion.h"
using namespace Player;

CRunState::CRunState(CPlayer* pPlayer)
{
    m_pOwner = pPlayer;
    m_ePlayerID = m_pOwner->Get_PlayerID();
    m_vTargetPos = m_pOwner->Get_TargetPosition();
}

CPlayerState* CRunState::HandleInput()
{
    CGameInstance* GI = CGameInstance::GetInstance();
    CPickingManager* PM = CPickingManager::GetInstance();


    if (GI->Key_Down(DIK_Q))
    {
        if (m_pOwner->Get_PlayerID() == CPlayer::EZREAL)
            return new CEzrealSkillStateQ(m_pOwner);

        if (m_pOwner->Get_PlayerID() == CPlayer::LEESIN)
            return new CLeeSinSkillStateQ(m_pOwner);
    }

    if (GI->Key_Down(DIK_W))
    {
        if (m_pOwner->Get_PlayerID() == CPlayer::EZREAL)
        {
            return new CEzrealSkillStateW(m_pOwner, CEzrealSkillStateW::CAST_RUN);
        }
        if (m_pOwner->Get_PlayerID() == CPlayer::TEEMO)
            return new CTemooSkillStateW(m_pOwner);
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

    if (GI->Mouse_Down(DIMK_RBUTTON))
    {
        CGameObject* pickedObj = PM->Get_PickedObj();
        _float3 clickPos = PM->Get_PickingPos();

        // 1) Enemy 찍었을 때
        if (CEnemy* pEnemy = dynamic_cast<CEnemy*>(pickedObj))
        {
            m_pOwner->Set_AttackTarget(pEnemy);

            _vector myPos = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
            _vector tarPos = pEnemy->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

            float dist = GetDistanceXZ(myPos, tarPos);
            float range = dynamic_cast<CChampion*>(m_pOwner)->Get_WorldAttackRange();

            if (dist <= range)
            {
                switch (m_pOwner->Get_PlayerID())
                {
                case CPlayer::EZREAL:
                    return new CEzrealAttackState(m_pOwner, STATE_ATTACK1);

                case CPlayer::LEESIN:
                    return new CLeeSinAttackState(m_pOwner);
                }
            }
            else
            {
                // 사거리 밖 → 공격 포지션으로 경로 갱신만 하고 Run 유지
                Set_AttackMoveToTarget(m_pOwner, pEnemy);
                return nullptr;
            }
        }
        else
        {
            // 2) Enemy가 아니면 → 순수 이동 명령
            m_pOwner->Set_AttackTarget(nullptr);
            m_pOwner->Set_TargetPosition(clickPos);
            m_pOwner->Get_Transform()->Set_Destination(
                XMLoadFloat3(&clickPos),
                m_pOwner->Get_Navigation());
            return nullptr;
        }
    }

    if (GI->Key_Pressing(DIK_B))
    {
        return new CPlayerRecallState(m_pOwner);
    }

    return nullptr;
}

CPlayerState* CRunState::Update(_float fTimeDelta)
{
    // 기존 타겟 추적/공격 로직 유지
    CActor* pTarget = m_pOwner->Get_AttackTarget();

    if (pTarget && !pTarget->Get_Dead())
    {
        _vector myPos = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
        _vector tarPos = pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

        float dist = GetDistanceXZ(myPos, tarPos);
        float range = static_cast<CChampion*>(m_pOwner)->Get_WorldAttackRange();

        if (dist <= range)
        {
            switch (m_pOwner->Get_PlayerID())
            {
            case CPlayer::EZREAL: return new CEzrealAttackState(m_pOwner, STATE_ATTACK1);
            case CPlayer::LEESIN: return new CLeeSinAttackState(m_pOwner);
            }
        }
    }

    Move(fTimeDelta);
    m_pOwner->Check_Navigation();

    if (m_pOwner->IsUsingSkill())
        return nullptr;

    if (STATETYPE_END == m_eStateType)
        return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);

    // =========================
    // ? Run 대사 (Ezreal)
    // - Run 상태 유지 중에만
    // - 진입 후 5~8초에 1번은 무조건
    // - 이후 12~22초 랜덤 반복
    // =========================
    if (m_pOwner && m_pOwner->Get_PlayerID() == CPlayer::EZREAL)
    {
        m_fRunNoTalkAcc += fTimeDelta;
        m_fRunTalkTimer -= fTimeDelta;

        if (m_fRunTalkTimer <= 0.f && m_fRunNoTalkAcc >= 5.f)
        {
            static const TCHAR* talks[] =
            {
                TEXT("EzrealTalk.wav"),
                TEXT("EzrealTalk1.wav"),
                TEXT("EzrealTalk2.wav"),
                TEXT("EzrealTalk3.wav"),
            };

            int tidx = rand() % (int)(sizeof(talks) / sizeof(talks[0]));
            CGameInstance::GetInstance()->PlaySounds((TCHAR*)talks[tidx], SOUND_PLAYER, 0.85f);

            float t = (rand() % 1000) / 1000.f;
            m_fRunTalkTimer = 12.f + 10.f * t; // 12~22초
        }
    }
    else if (m_pOwner && m_pOwner->Get_PlayerID() == CPlayer::LEESIN)
    {
        m_fRunNoTalkAcc += fTimeDelta;
        m_fRunTalkTimer -= fTimeDelta;

        if (m_fRunTalkTimer <= 0.f && m_fRunNoTalkAcc >= 5.f)
        {
            static const TCHAR* talks[] =
            {
                    TEXT("LeeSinTalk1.wav"),
                    TEXT("LeeSinTalk3.wav"),
                    TEXT("LeeSinTalk5.wav"),
                    TEXT("LeeSinTalk4.wav"),
            };

            int tidx = rand() % (int)(sizeof(talks) / sizeof(talks[0]));
            CGameInstance::GetInstance()->PlaySounds((TCHAR*)talks[tidx], SOUND_PLAYER, 0.85f);

            float t = (rand() % 1000) / 1000.f;
            m_fRunTalkTimer = 12.f + 10.f * t; // 12~22초
        }
    }
    else if (m_pOwner && m_pOwner->Get_PlayerID() == CPlayer::TEEMO)
    {
        m_fRunNoTalkAcc += fTimeDelta;
        m_fRunTalkTimer -= fTimeDelta;

        if (m_fRunTalkTimer <= 0.f && m_fRunNoTalkAcc >= 5.f)
        {
            static const TCHAR* talks[] =
            {
                    TEXT("TemooTalkSound1.wav"),
                    TEXT("TemooTalkSound2.wav"),
                    TEXT("TemooTalkSound3.wav"),
            };

            int tidx = rand() % (int)(sizeof(talks) / sizeof(talks[0]));
            CGameInstance::GetInstance()->PlaySounds((TCHAR*)talks[tidx], SOUND_PLAYER, 0.85f);

            float t = (rand() % 1000) / 1000.f;
            m_fRunTalkTimer = 12.f + 10.f * t; // 12~22초
        }
    }
    // 기존 Run 애니 재생
    m_pOwner->Get_Model()->Play_Animation(
        fTimeDelta,
        m_pOwner->Is_AnimationLoop(m_pOwner->Get_Model()->Get_CurrentAnimIndex())
    );

    return nullptr;
}

CPlayerState* CRunState::Late_Update(_float fTimeDelta)
{
    if (STATETYPE_END == m_eStateType)
        return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
	return nullptr;
}

void CRunState::Enter()
{
    __super::Enter();

    m_eState = STATE_RUN;
    m_eStateType = STATETYPE_START;

    _float3 target = m_pOwner->Get_TargetPosition();

    CTransform* tr = m_pOwner->Get_Transform();
    CNavigation* nav = m_pOwner->Get_Navigation();

    tr->Set_Destination(XMLoadFloat3(&target), nav);

    // 애니메이션 설정
    switch (m_ePlayerID)
    {
    case CPlayer::EZREAL:
        if (!m_pOwner->IsUsingSkill())
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::RUN);
        break;

    case CPlayer::LEESIN:
        if (!m_pOwner->IsUsingSkill())
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CLeeSin::ANIM::RUN1);
        break;

    case CPlayer::TEEMO:
    {
        if (m_pOwner->IsUsingSkill())
            break;

        CChampion* pChamp = dynamic_cast<CChampion*>(m_pOwner);
        const bool bWOn = (pChamp && pChamp->Get_MoveSpeedMul() > 1.001f);

        const _uint animNormal = CTemoo::ANIM::RUN;
        const _uint animWRun = CTemoo::ANIM::W;

        m_pOwner->Get_Model()->Set_CurrentAnimIndex(bWOn ? animWRun : animNormal);
        break;
    }
    }

    // =========================
    // ? Run 진입 후 5~8초 뒤 대사 1번 "무조건"
    // =========================
    m_fRunNoTalkAcc = 0.f;
    {
        float t = (rand() % 1000) / 1000.f;
        m_fRunTalkTimer = 5.f + 3.f * t; // 5~8초
    }

}

void CRunState::Exit()
{
    m_fRunNoTalkAcc = 0.f;
    m_fRunTalkTimer = 0.f;
}

void CRunState::Move(_float fTimeDelta)
{
       CTransform* pTr = m_pOwner->Get_Transform();
    CNavigation* pNavi = m_pOwner->Get_Navigation();

    if (!pTr->Move_ByPath(fTimeDelta, pNavi))
    {
        // 스킬 시전 중엔 절대 Run 종료시키지 말 것
        if (!m_pOwner->IsUsingSkill())
            m_eStateType = STATETYPE_END;
        return;
    }
}


