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

    // 1) 도망치는 시간: 0.4 ~ 0.8초 정도 (짧게 깔짝)
    //    rand() % 401 -> 0~400 / 1000.f -> 0.0 ~ 0.4  + 0.4 -> 0.4 ~ 0.8
    m_fReturnTime = 0.4f + (rand() % 401) / 1000.f;

    // 2) RUN 애니메이션 설정
    if (CModel* pModel = m_pOwner->Get_Model())
        pModel->Set_CurrentAnimIndex(CVayne::ANIM::RUN);

    CTransform* pTr = m_pOwner->Get_Transform();
    CNavigation* pNavi = m_pOwner->Get_Navigation();
    if (!pTr || !pNavi)
        return;

    // 3) 현재 바라보는 방향 기준으로 "뒤쪽"으로 살짝 빠지기
    _vector vMyPos = pTr->Get_State(CTransform::STATE_TRANSLATION);
    _vector vLook = pTr->Get_State(CTransform::STATE_LOOK);

    // XZ 평면만 사용
    vLook = XMVectorSetY(vLook, 0.f);

    // 혹시 길이 0이면 기본 방향 하나 세팅
    if (XMVectorGetX(XMVector3LengthSq(vLook)) < 0.0001f)
        vLook = XMVectorSet(0.f, 0.f, 1.f, 0.f);   // 임의 Z+

    vLook = XMVector3Normalize(vLook);

    // ?? 뒤로 빠질 방향 = -Look
    _vector vBackDir = -vLook;

    // ?? 얼마나 빠질지(거리): 3.0f 정도 (원하면 2~4로 조정)
    const float fRetreatDist = 3.0f;

    _vector vTargetPos = vMyPos + vBackDir * fRetreatDist;

    // 목적지 Y는 네비에서 처리하게 XZ만 맞춰도 됨
    vTargetPos = XMVectorSetY(vTargetPos, XMVectorGetY(vMyPos));

    // 4) 네비게이션 목적지 설정 (여기로 살짝 도망)
    pTr->Set_Destination(vTargetPos, pNavi);
}

void CReturnState::Exit()
{
    // 특별히 초기화할 것은 현재 없음
}