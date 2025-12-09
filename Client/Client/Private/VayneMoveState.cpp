
#include "VayneMoveState.h"
#include "VayneIdleState.h"
#include "VayneAttackState.h"
#include "Vayne.h"
#include "Transform.h"
#include "Navigation.h"

using namespace Vayne;

CMoveState::CMoveState(CVayne* pVayne)
{
    m_pOwner = pVayne;
}

CVayneState* CMoveState::AI_Behaviour(_float fTimeDelta)
{
    if (!m_pOwner)
        return nullptr;

    // 1) 타겟 유효성 검사
    CActor* pTarget = m_pOwner->Check_FiledTarget();

    // 2) 타겟 없으면 새로 찾기
    if (!pTarget)
    {
        m_pOwner->Find_Target();
        pTarget = m_pOwner->Get_Target();
    }

    if (!pTarget)
        return nullptr; // 계속 Move

    // 3) 공격 사거리 안인지 체크
    float dist = m_pOwner->Target_Distance(pTarget);
    float range = m_pOwner->Get_AttackRangeWorld();

    if (dist <= range)
    {
        // ?? 사거리 안 → 공격 상태로 전환
        return new CAttackState(m_pOwner);
    }

    return nullptr; // 사거리 밖이면 계속 Move
}

CVayneState* CMoveState::Update(_float fTimeDelta)
{
    if (!m_pOwner)
        return nullptr;

    // ?? 1) 라인 따라 이동
    m_pOwner->Follow_Lane(fTimeDelta);

    // ?? 2) RUN 애니메이션 진행
    CModel* pModel = m_pOwner->Get_Model();
    if (pModel)
    {
        m_bIsAnimationFinished =
            pModel->Play_Animation(
                fTimeDelta,
                m_pOwner->Is_AnimationLoop(
                    pModel->Get_CurrentAnimIndex())
            );
    }

    return nullptr;  // 상태 유지
}

CVayneState* CMoveState::LateUpdate(_float fTimeDelta)
{
    return nullptr;
}

void CMoveState::Enter()
{
    m_eStateId = STATE_MOVE;
    m_fMoveTimeAcc = 0.f;

    if (!m_pOwner)
        return;

    // RUN 애니메이션
    if (m_pOwner->Get_Model())
        m_pOwner->Get_Model()->Set_CurrentAnimIndex(CVayne::ANIM::RUN);

    // ★ 첫 목적지 세팅
    CTransform* pTr = m_pOwner->Get_Transform();
    CNavigation* pNavi = m_pOwner->Get_Navigation();
    if (!pTr || !pNavi)
        return;

    const auto& path = CVayne::Get_EnemyLanePath(m_pOwner->Get_LineType());
    if (path.empty())
        return;

    _int nodeIdx = m_pOwner->Get_LaneNodeIndex();
    if (nodeIdx < 0 || nodeIdx >= (_int)path.size())
        nodeIdx = 0;

    m_pOwner->Set_LaneNodeIndex(nodeIdx);

    _float3 vTarget = path[nodeIdx];
    _vector vTargetV = XMLoadFloat3(&vTarget);

    pTr->Set_Destination(vTargetV, pNavi);
}

void CMoveState::Exit()
{
}
