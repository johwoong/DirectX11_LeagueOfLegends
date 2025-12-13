#include "VayneReturnState.h"
#include "VayneMoveState.h"
#include "Vayne.h"
#include "Transform.h"
#include "Navigation.h"
#include "Model.h"

USING(Client);
using namespace Vayne;

CReturnState::CReturnState(CVayne* pVayne)
{
    m_pOwner = pVayne;
}

CVayneState* CReturnState::AI_Behaviour(_float fTimeDelta)
{
    UNREFERENCED_PARAMETER(fTimeDelta);
    return nullptr;
}

CVayneState* CReturnState::Update(_float fTimeDelta)
{
    if (!m_pOwner)
        return nullptr;

    CTransform* pTr = m_pOwner->Get_Transform();
    CNavigation* pNavi = m_pOwner->Get_Navigation();
    if (!pTr || !pNavi)
        return nullptr;

    pTr->Move_ByPath(fTimeDelta, pNavi);

    m_pOwner->Check_Navigation();
    if (CModel* pModel = m_pOwner->Get_Model())
    {
        m_bIsAnimationFinished =
            pModel->Play_Animation(
                fTimeDelta,
                m_pOwner->Is_AnimationLoop(
                    pModel->Get_CurrentAnimIndex()));
    }

    m_fReturnTimeAcc += fTimeDelta;
    if (m_fReturnTimeAcc >= m_fReturnTime)
    {
        m_pOwner->RefreshLaneNodeByPosition();  // Enemy 쪽 함수 재사용
        return new CMoveState(m_pOwner);
    }
    return nullptr;
}

CVayneState* CReturnState::LateUpdate(_float fTimeDelta)
{
    UNREFERENCED_PARAMETER(fTimeDelta);
    return nullptr;
}

void CReturnState::Enter()
{
    m_eStateId = STATE_MOVE;
    m_fReturnTimeAcc = 0.f;

    if (!m_pOwner)
        return;

    // 1) 도망치는 시간: 0.5 ~ 1.0초 정도
    m_fReturnTime = 0.5f + (rand() % 501) / 1000.f; // 0.5 ~ 1.0

    // 2) RUN 애니메이션
    if (CModel* pModel = m_pOwner->Get_Model())
        pModel->Set_CurrentAnimIndex(CVayne::ANIM::RUN);

    CTransform* pTr = m_pOwner->Get_Transform();
    CNavigation* pNavi = m_pOwner->Get_Navigation();
    if (!pTr || !pNavi)
        return;

    _vector vMyPos = pTr->Get_State(CTransform::STATE_TRANSLATION);
    _vector vLook = pTr->Get_State(CTransform::STATE_LOOK);
    _vector vRight = pTr->Get_State(CTransform::STATE_RIGHT);

    // XZ 평면만
    vLook = XMVectorSetY(vLook, 0.f);
    vRight = XMVectorSetY(vRight, 0.f);

    if (XMVectorGetX(XMVector3LengthSq(vLook)) < 0.0001f)
        vLook = XMVectorSet(0.f, 0.f, 1.f, 0.f);

    vLook = XMVector3Normalize(vLook);
    vRight = XMVector3Normalize(vRight);

    // 기본은 뒤로
    _vector vBackDir = vLook;    // <-- 여기 중요: 원래는 -vLook 이었음

    // 3) 옆으로 살짝 섞어서 "대각선 도망"
    float sideSign = (rand() % 2 == 0) ? 1.f : -1.f; // 왼/오 랜덤
    float sideWeight = 0.5f;                            // 옆으로 섞는 비율
    vBackDir = XMVector3Normalize(vBackDir + vRight * (sideWeight * sideSign));

    // 4) 도망 거리: 4.0 ~ 7.0 정도
    float fRetreatDist = 4.0f + (rand() % 300) / 100.f; // 4.0 ~ 7.0

    _vector vTargetPos = vMyPos + vBackDir * fRetreatDist;

    // Y는 네비게이션에서 맞추게 두고, 기본은 현재 높이 유지
    vTargetPos = XMVectorSetY(vTargetPos, XMVectorGetY(vMyPos));

    // 네비에 목적지 등록
    pTr->Set_Destination(vTargetPos, pNavi);
}

void CReturnState::Exit()
{
    // 특별히 초기화할 것은 현재 없음
}