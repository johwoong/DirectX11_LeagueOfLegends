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
    // 1) 현재 타겟이 여전히 유효한지 확인
    CActor* pTarget = m_pOwner->Check_FiledTarget();

    // 2) 없으면 새로 탐색 (미니언만)
    if (!pTarget)
    {
        m_pOwner->Find_Target();
        pTarget = m_pOwner->Get_Target();
    }

    // 3) 타겟 있으면 Chase 상태로 전환
    if (pTarget)
    {
        return new CChaseState(m_pOwner);
    }

    // 타겟 없으면 그냥 라인 따라 이동만
    // (네가 이미 만든 라인 이동 로직 호출)
    // m_pOwner->Follow_Lane(fTimeDelta);

    return nullptr; // 상태 유지
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

    // 2) 네비 경로 따라 이동 (공용)
    //CTransform* pTr = m_pOwner->Get_Transform();
    //CNavigation* pNavi = m_pOwner->Get_Navigation();
    //if (!pTr || !pNavi)
    //    return nullptr;

    //// Move_ByPath : true = 아직 경로 남음, false = 목적 웨이포인트 도착
    //if (!pTr->Move_ByPath(fTimeDelta, pNavi))
    //{
    //    // ===== 웨이포인트 도착했을 때 처리 =====
    //    CMinionManager* pMgr = CMinionManager::GetInstance();
    //    if (!pMgr)
    //        return nullptr;

    //    const auto& path = pMgr->GetLanePath(m_pOwner->Get_Lane());
    //    if (path.empty())
    //        return nullptr;

    //    _int idx = m_pOwner->Get_LaneNodeIndex();
    //    _int step = pMgr->GetLaneStep(m_pOwner->Get_Team()); // BLUE:+1, RED:-1
    //    _int nextIdx = idx + step;

    //    // 라인 끝 도착 → 나중에 타워/넥서스 공격 상태로 전환 자리
    //    if (nextIdx < 0 || nextIdx >= (_int)path.size())
    //    {
    //        // TODO: 타워/넥서스 타겟팅 상태로 변환
    //        return nullptr;
    //    }

    //    // 다음 웨이포인트 인덱스 갱신
    //    m_pOwner->Set_LaneNodeIndex(nextIdx);

    //    _float3 vTarget = path[nextIdx];
    //    _vector vTargetV = XMLoadFloat3(&vTarget);

    //    // ★ 다음 웨이포인트로 A* 목적지 재설정
    //    pTr->Set_Destination(vTargetV, pNavi);
    //}

    // 네비 Y 높이 보정
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

    // ★ 타입 상관 없이 Run 애니 재생
    if (m_pOwner->Get_Model())
        m_pOwner->Get_Model()->Set_CurrentAnimIndex(m_pOwner->GetRunAnimIndex());

    CTransform* pTr = m_pOwner->Get_Transform();
    CNavigation* pNavi = m_pOwner->Get_Navigation();
    if (!pTr || !pNavi)
        return;

    // ★ 라인 웨이포인트
    CMinionManager* pMgr = CMinionManager::GetInstance();
    if (!pMgr)
        return;

    const auto& path = pMgr->GetLanePath(m_pOwner->Get_Lane());
    if (path.empty())
        return;

    _int nodeIdx = m_pOwner->Get_LaneNodeIndex();
    if (nodeIdx < 0 || nodeIdx >= (_int)path.size())
        return;

    _float3 vTarget = path[nodeIdx];
    _vector vTargetV = XMLoadFloat3(&vTarget);

    // ★ 여기서 A* 셋업
    pTr->Set_Destination(vTargetV, pNavi);
}

void CMoveState::Exit()
{
}
