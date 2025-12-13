
#include "VayneMoveState.h"
#include "VayneIdleState.h"
#include "VayneAttackState.h"
#include "Vayne.h"
#include "Transform.h"
#include "Navigation.h"
#include "Minion.h"

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

    CTransform* pTr = m_pOwner->Get_Transform();
    CNavigation* pNavi = m_pOwner->Get_Navigation();
    if (!pTr || !pNavi)
        return nullptr;

    if (!m_pOwner->HasReachedFirstTower())
    {
        m_pOwner->Follow_Lane(fTimeDelta);

        _float3 vWaitPos = m_pOwner->Get_FirstTowerWaitPos();

        _vector vMy = pTr->Get_State(CTransform::STATE_TRANSLATION);
        _vector vDst = XMLoadFloat3(&vWaitPos);

        vMy = XMVectorSetY(vMy, 0.f);
        vDst = XMVectorSetY(vDst, 0.f);

        float fDist = XMVectorGetX(XMVector3Length(vDst - vMy));
        if (fDist <= 2.f)
        {
            m_pOwner->SetReachedFirstTower(true);
            return new CIdleState(m_pOwner);
        }
    }
    else
    {
        // ? 첫 포탑 이후 구간
        CMinion* pFront = m_pOwner->Get_FrontAllyMinion(100.f);

        if (pFront)
        {
            _vector vMinionPos = pFront->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
            _vector vMyPos = pTr->Get_State(CTransform::STATE_TRANSLATION);

            vMinionPos = XMVectorSetY(vMinionPos, XMVectorGetY(vMyPos));

            const auto& path = CVayne::Get_EnemyLanePath(m_pOwner->Get_LineType());
            _vector vFollowPos = vMinionPos;

            if (path.size() >= 2)
            {
                _vector vStart = XMLoadFloat3(&path.front());
                _vector vEnd = XMLoadFloat3(&path.back());
                _vector vDir = XMVector3Normalize(XMVectorSetY(vEnd - vStart, 0.f));

                vFollowPos = vMinionPos - vDir * 2.5f; // 미니언 뒤에 붙기
            }

            _float3 newFollow{};
            XMStoreFloat3(&newFollow, vFollowPos);

            if (!m_bHasFollowPos ||
                XMVectorGetX(
                    XMVector3Length(
                        XMLoadFloat3(&m_vLastFollowPos) - vFollowPos)) > 1.f)
            {
                m_vLastFollowPos = newFollow;
                m_bHasFollowPos = true;
                pTr->Set_Destination(vFollowPos, pNavi);
            }

            pTr->Move_ByPath(fTimeDelta, pNavi);
            m_pOwner->Check_Navigation();
        }
        else
        {
            // 앞에 우리 미니언이 아예 없으면 그냥 라인 따라 이동 (원래 로직)
            m_pOwner->Follow_Lane(fTimeDelta);
        }
    }

    // RUN 애니
    if (CModel* pModel = m_pOwner->Get_Model())
    {
        m_bIsAnimationFinished =
            pModel->Play_Animation(
                fTimeDelta,
                m_pOwner->Is_AnimationLoop(
                    pModel->Get_CurrentAnimIndex()));
    }

    return nullptr;
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
