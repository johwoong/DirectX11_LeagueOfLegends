#include "Minion_MoveState.h"
#include "Minion.h"
#include "MinionManager.h" 
#include "Transform.h"
#include "Navigation.h"
#include "Minion_ChaseState.h"

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

    // 1) 애니 업데이트
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

        if (nextIdx < 0 || nextIdx >= (_int)path.size())
        {
            return nullptr;
        }

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

    // ★ 현재 위치 기준으로 "앞으로 가야 할" 노드 다시 선택
    _int nodeIdx = m_pOwner->Find_ForwardLaneNodeIndex();
    if (nodeIdx < 0 || nodeIdx >= (_int)path.size())
        return;

    m_pOwner->Set_LaneNodeIndex(nodeIdx);

    _float3 vTarget = path[nodeIdx];
    _vector vTargetV = XMLoadFloat3(&vTarget);

    // ★ 이제는 "현재 위치 → 앞으로 가야 할 노드"로 바로 이동
    pTr->Set_Destination(vTargetV, pNavi);
}

void CMoveState::Exit()
{
}
