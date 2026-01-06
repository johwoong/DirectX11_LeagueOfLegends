
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

    // 공격 쿨타임 감소
    if (m_pOwner->m_fPostAttackMoveTime > 0.f)
    {
        m_pOwner->m_fPostAttackMoveTime -= fTimeDelta;
        return nullptr; // ★ 이 프레임은 절대 공격 안 함
    }

    if (dist <= range)
    {
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

    if (m_pOwner->m_fPostAttackMoveTime > 0.f)
    {
        // ?? 이동
        pTr->Move_ByPath(fTimeDelta, pNavi);
        m_pOwner->Check_Navigation();

        // ?? 이동 중에는 무조건 RUN 애니
        if (CModel* pModel = m_pOwner->Get_Model())
        {
            pModel->Play_Animation(fTimeDelta, true);
        }

        return nullptr;
    }

    CActor* pTarget = m_pOwner->Check_FiledTarget();
    if (pTarget)
    {
        float dist = m_pOwner->Target_Distance(pTarget);
        float range = m_pOwner->Get_AttackRangeWorld();

        if (dist > range)
        {
            _vector vTargetPos =
                pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

            // 네비 목적지 설정
            pTr->Set_Destination(vTargetPos, pNavi);
            pTr->Move_ByPath(fTimeDelta, pNavi);
            m_pOwner->Check_Navigation();

            // RUN 애니
            if (CModel* pModel = m_pOwner->Get_Model())
                pModel->Play_Animation(fTimeDelta, true);

            return nullptr;
        }
    }


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
        CMinion* pFront = m_pOwner->Get_FrontAllyMinion(100.f);

        bool bDidFollowMinion = false;

        if (pFront)
        {
            _vector vMyPos =
                pTr->Get_State(CTransform::STATE_TRANSLATION);
            _vector vMinionPos =
                pFront->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

            vMyPos = XMVectorSetY(vMyPos, 0.f);
            vMinionPos = XMVectorSetY(vMinionPos, 0.f);

            const auto& path = CVayne::Get_EnemyLanePath(m_pOwner->Get_LineType());

            if (path.size() >= 2)
            {
                _vector vStart = XMLoadFloat3(&path.front());
                _vector vEnd = XMLoadFloat3(&path.back());
                _vector vDir = XMVector3Normalize(
                    XMVectorSetY(vEnd - vStart, 0.f));

                // ? 미니언이 "내 앞"에 있을 때만
                float forwardDot =
                    XMVectorGetX(XMVector3Dot(vMinionPos - vMyPos, vDir));

                if (forwardDot > 0.f)
                {
                    _vector vFollowPos = vMinionPos - vDir * 2.5f;

                    pTr->Set_Destination(vFollowPos, pNavi);
                    pTr->Move_ByPath(fTimeDelta, pNavi);
                    m_pOwner->Check_Navigation();

                    bDidFollowMinion = true;
                }
            }
        }

        // ? 미니언 추종 안 했으면 무조건 라인 전진
        if (!bDidFollowMinion)
        {
            m_pOwner->Follow_Lane(fTimeDelta);
        }
    }

    if (CModel* pModel = m_pOwner->Get_Model())
    {
        if (m_pOwner->m_bQEmpowered)
        {
            if (pModel->Get_CurrentAnimIndex() != CVayne::ANIM::Q_RUN)
                pModel->Set_CurrentAnimIndex(CVayne::ANIM::Q_RUN);

            pModel->Play_Animation(fTimeDelta, true);
        }
        else
        {
            if (pModel->Get_CurrentAnimIndex() != CVayne::ANIM::RUN)
                pModel->Set_CurrentAnimIndex(CVayne::ANIM::RUN);

            pModel->Play_Animation(fTimeDelta, true);
        }
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

    if (!m_pOwner)
        return;

    CModel* pModel = m_pOwner->Get_Model();
    if (pModel)
    {
        // ?? Q 강화 상태면 Q_RUN
        if (m_pOwner->m_bQEmpowered)
            pModel->Set_CurrentAnimIndex(CVayne::ANIM::Q_RUN);
        else
            pModel->Set_CurrentAnimIndex(CVayne::ANIM::RUN);
    }

    CTransform* pTr = m_pOwner->Get_Transform();
    CNavigation* pNavi = m_pOwner->Get_Navigation();
    if (!pTr || !pNavi)
        return;

    // ?? 카이팅 우선
    if (m_pOwner->m_bDoKiting)
    {
        pTr->Set_Destination(m_pOwner->m_vKitingDest, pNavi);
        m_pOwner->m_bDoKiting = false;
        return;
    }

    // ===============================
    // 2. 기존 라인 이동 (그대로)
    // ===============================
    const auto& path = CVayne::Get_EnemyLanePath(m_pOwner->Get_LineType());
    if (path.empty())
        return;

    _int nodeIdx = m_pOwner->Get_LaneNodeIndex();
    if (nodeIdx < 0 || nodeIdx >= (_int)path.size())
        nodeIdx = 0;

    _float3 vTarget = path[nodeIdx];
    pTr->Set_Destination(XMLoadFloat3(&vTarget), pNavi);
}

void CMoveState::Exit()
{
}
