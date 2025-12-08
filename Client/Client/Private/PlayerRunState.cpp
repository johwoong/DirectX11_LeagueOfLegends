#include "PlayerRunState.h"
#include "PlayerIdleState.h"
#include "Terrian.h"
#include "EzrealAttackState.h"
#include "EzrealSkillStateQ.h"
#include "EzrealSkillStateW.h"
#include "EzrealSkillStateE.h"
#include "EzrealSkillStateR.h"
#include "PlayerRecallState.h"

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
    }

    if (GI->Key_Down(DIK_W))
    {
        if (m_pOwner->Get_PlayerID() == CPlayer::EZREAL)
        {
            // 지금 달려가고 있는 목적지 (Set_TargetPosition 으로 저장해 둔 값)
            // 인자는 생략해도 내부에서 Get_TargetPosition()을 기본으로 사용하니까 아래 한 줄이면 됨
            return new CEzrealSkillStateW(m_pOwner, CEzrealSkillStateW::CAST_RUN);
        }
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
                // 이미 사거리 안 → 바로 공격
                return new CEzrealAttackState(m_pOwner, STATE_ATTACK1);
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
    // 1) 먼저 타겟이 있고, 살아있으면
    CActor* pTarget = m_pOwner->Get_AttackTarget();
    if (pTarget && !pTarget->Get_Dead())
    {
        _vector myPos = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
        _vector tarPos = pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

        float dist = GetDistanceXZ(myPos, tarPos);
        float range = dynamic_cast<CChampion*>(m_pOwner)->Get_WorldAttackRange(); // ★ 아래 2번에서 설명할 통일된 사거리

        // 사거리 안으로 들어오면 → 바로 AttackState 진입
        if (dist <= range)
            return new CEzrealAttackState(m_pOwner, STATE_ATTACK1);
    }

    // 2) 그 다음에 이동
    Move(fTimeDelta);
    m_pOwner->Check_Navigation();

    if (m_pOwner->IsUsingSkill())
        return nullptr;

    if (STATETYPE_END == m_eStateType)
        return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);

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
    // ★ 반드시 여기에서 최신 타겟 위치로 경로 생성
    _float3 target = m_pOwner->Get_TargetPosition();

    CTransform* tr = m_pOwner->Get_Transform();
    CNavigation* nav = m_pOwner->Get_Navigation();

    tr->Set_Destination(XMLoadFloat3(&target), nav);

    // 애니메이션 설정
    switch (m_ePlayerID)
    {
    case CPlayer::EZREAL:
        if (m_pOwner->IsUsingSkill())
            return;

        // 평소엔 Run 애니 재생
        m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::RUN);
        break;
    }

}

void CRunState::Exit()
{
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


