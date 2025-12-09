#include "Minion_MoveState.h"
#include "Minion.h"
#include "MinionManager.h" 
#include "Transform.h"
#include "Navigation.h"
#include "Minion_ChaseState.h"
#include "Minion_IdleState.h"

using namespace Minion;

CMoveState::CMoveState(CMinion* pMinion)
{
    m_pOwner = pMinion;
}

CMinionState* CMoveState::AI_Behaviour(_float fTimeDelta)
{
    CActor* pTarget = m_pOwner->Check_FiledTarget();

    if (!pTarget)
    {
        m_pOwner->Find_Target(fTimeDelta);
        pTarget = m_pOwner->Get_Target();
    }

    if (pTarget)
    {
        return new CChaseState(m_pOwner);
    }



    return nullptr;
}

CMinionState* CMoveState::Update(_float fTimeDelta)
{
    if (!m_pOwner)
        return nullptr;

    // 1) 애니메이션 업데이트
    m_bIsAnimationFinished =
        m_pOwner->Get_Model()->Play_Animation(
            fTimeDelta,
            m_pOwner->Is_AnimationLoop(m_pOwner->Get_Model()->Get_CurrentAnimIndex()),
            nullptr,
            0.1f);

    CTransform* pTr = m_pOwner->Get_Transform();
    CNavigation* pNavi = m_pOwner->Get_Navigation();
    if (!pTr || !pNavi)
        return nullptr;

    // 2) 네비 경로 따라 이동
    if (!pTr->Move_ByPath(fTimeDelta, pNavi))
    {
        CMinionManager* pMgr = CMinionManager::GetInstance();
        if (!pMgr)
            return nullptr;

        const auto& path = pMgr->GetLanePath(m_pOwner->Get_Lane());
        if (path.empty())
            return nullptr;

        _int idx = m_pOwner->Get_LaneNodeIndex();
        _int step = pMgr->GetLaneStep(m_pOwner->Get_Team());
        _int nextIdx = idx + step;

        // ★ 여기: 더 이상 앞으로 갈 노드가 없을 때
        if (nextIdx < 0 || nextIdx >= (_int)path.size())
        {
            // 먼저 타겟이 있는지 확인
            CActor* pTarget = m_pOwner->Get_Target();
            if (!pTarget)
            {
                m_pOwner->Find_Target(fTimeDelta);
                pTarget = m_pOwner->Get_Target();
            }

            // 타겟이 있으면 추적 상태로 전환 (타워/억제기 쫓아가게)
            if (pTarget)
                return new CChaseState(m_pOwner);

            // 타겟도 없으면 그냥 그 자리에서 멈추도록 Idle
            return new CIdleState(m_pOwner);
        }

        // 아직 경로가 남았으면 다음 노드로 진행
        m_pOwner->Set_LaneNodeIndex(nextIdx);

        _float3 vTarget = path[nextIdx];
        _vector vTargetV = XMLoadFloat3(&vTarget);

        pTr->Set_Destination(vTargetV, pNavi);
    }

    m_pOwner->Check_Navigation();

    return nullptr;
}

CMinionState* CMoveState::Late_Update(_float fTimeDelta)
{
    return nullptr;
}

void CMoveState::Enter()
{
    m_eState = STATE_RUN;

    if (!m_pOwner)
        return;

    if (m_pOwner->Get_Model())
        m_pOwner->Get_Model()->Set_CurrentAnimIndex(m_pOwner->GetRunAnimIndex());

    CTransform* pTr = m_pOwner->Get_Transform();
    CNavigation* pNavi = m_pOwner->Get_Navigation();
    if (!pTr || !pNavi)
        return;

    CMinionManager* pMgr = CMinionManager::GetInstance();
    if (!pMgr)
        return;

    const auto& path = pMgr->GetLanePath(m_pOwner->Get_Lane());
    if (path.empty())
        return;

    // 우선 기존 LaneNodeIndex 사용
    _int nodeIdx = m_pOwner->Get_LaneNodeIndex();

    // 만약 초기값이 잘못 들어와 있으면 그때만 보정
    if (nodeIdx < 0 || nodeIdx >= (_int)path.size())
    {
        nodeIdx = m_pOwner->Find_ForwardLaneNodeIndex();
        if (nodeIdx < 0 || nodeIdx >= (_int)path.size())
            return;
        m_pOwner->Set_LaneNodeIndex(nodeIdx);
    }

    _float3 vTarget = path[nodeIdx];
    _vector vTargetV = XMLoadFloat3(&vTarget);

    pTr->Set_Destination(vTargetV, pNavi);
}

void CMoveState::Exit()
{
}
