#include "PlayerRunState.h"
#include "PlayerIdleState.h"
#include "EzrealAttackState.h"
#include "Terrian.h"

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

    if (GI->Mouse_Down(DIMK_RBUTTON))
    {
        // Picking 실패 → 무시
        if (!PM->Picking())
            return nullptr;

        CGameObject* pickedObj = PM->Get_PickedObj();
        _float3 clickPos = PM->Get_PickingPos();

        // ------------------------------
        // 1) Enemy 클릭
        // ------------------------------
        if (CEnemy* enemy = dynamic_cast<CEnemy*>(pickedObj))
        {
            m_pOwner->Set_AttackTarget(enemy);

            _float3 enemyPos;
            XMStoreFloat3(&enemyPos,
                enemy->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION));

            float dist = GetDistanceXZ(
                m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION),
                enemy->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION));

            float range = dynamic_cast<CChampion*>(m_pOwner)->Get_WorldAttackRange();

            // 사거리 안 → 공격 시작
            if (dist <= range)
                return new CEzrealAttackState(m_pOwner, STATE_ATTACK1);

            // 사거리 밖 → 적 위치로 다시 경로 설정
            m_pOwner->Set_TargetPosition(enemyPos);
            m_pOwner->Get_Transform()->Set_Destination(XMLoadFloat3(&enemyPos),
                m_pOwner->Get_Navigation());

            return nullptr; // RunState 유지
        }

        // ------------------------------
        // 2) 지형 클릭
        // ------------------------------
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
        return next;

    Move(fTimeDelta);
    m_pOwner->Check_Navigation();

    CActor* target = m_pOwner->Get_AttackTarget();
    if (target && !target->Get_Dead())
    {
        float dist = GetDistanceXZ(
            m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION),
            target->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION));

        float range = dynamic_cast<CChampion*>(m_pOwner)->Get_WorldAttackRange();

        if (dist <= range)
            return new CEzrealAttackState(m_pOwner, STATE_ATTACK1);
    }

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

    // Transform이 스스로 경로 따라 감
    if (!pTr->Move_ByPath(fTimeDelta, pNavi))
    {
        m_eStateType = STATETYPE_END;
        return;
    }
}


