#include "PlayerRunState.h"
#include "PlayerIdleState.h"
#include "Terrian.h"
#include "EzrealAttackState.h"
#include "EzrealSkillStateQ.h"

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
        {
            // 달리면서 Q 시전
            return new CEzrealSkillStateQ(m_pOwner); // ← runningQ 모드
        }
    }


    if (GI->Mouse_Down(DIMK_RBUTTON))
    {

        CGameObject* pickedObj = PM->Get_PickedObj();
        _float3 clickPos = PM->Get_PickingPos();

        // ------------------------------
        // 1) Enemy 클릭
        // ------------------------------
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

        m_pOwner->Get_Transform()->Set_Destination(XMLoadFloat3(&clickPos),
            m_pOwner->Get_Navigation());
    }

    return nullptr;
}

CPlayerState* CRunState::Update(_float fTimeDelta)
{
    // 입력 처리 후 상태 변경 허용
    if (CPlayerState* next = HandleInput())
        return next;   // ★ 여기서 즉시 반환! 아래 코드 절대 실행하면 안 됨

    Move(fTimeDelta);
    m_pOwner->Check_Navigation();

    // (나머지 로직 동일)

    if (m_pOwner->IsUsingSkill())
        return nullptr;

    if (STATETYPE_END == m_eStateType)
        return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);

    // ★ SkillState로 넘어가지 않을 때만 애니메이션 재생
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


