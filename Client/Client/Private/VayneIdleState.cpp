#include "VayneIdleState.h"
#include "VayneMoveState.h"

using namespace Vayne;

CIdleState::CIdleState(CVayne* pVayne)
{
    m_pOwner = pVayne;
    m_fTimeDeltaAcc = 0.f;
    m_fIdleTime = 0.f;   // 실제 값은 Enter()에서 랜덤 설정
}

CVayneState* CIdleState::AI_Behaviour(_float fTimeDelta)
{
    // 상태 전환 조건 담당
    m_fTimeDeltaAcc += fTimeDelta;

    // IdleTime(0~5초 랜덤)만큼 지나면 MoveState로 전환
    if (m_fTimeDeltaAcc >= m_fIdleTime)
    {
        return new CMoveState(m_pOwner);
    }

    return nullptr;
}

CVayneState* CIdleState::Update(_float fTimeDelta)
{
    m_bIsAnimationFinished =
        m_pOwner->Get_Model()->Play_Animation(
            fTimeDelta,
            m_pOwner->Is_AnimationLoop(
                m_pOwner->Get_Model()->Get_CurrentAnimIndex())
        );
    return nullptr;
}

CVayneState* CIdleState::LateUpdate(_float fTimeDelta)
{
    m_pOwner->Check_Navigation();
    return nullptr;
}

void CIdleState::Enter()
{
    m_eStateId = STATE_ID::STATE_IDLE;

    // 1) Idle 애니메이션 랜덤 선택 (IDLE ~ IDLE4)
    static const CVayne::ANIM sIdleAnims[] =
    {
        CVayne::ANIM::IDLE,
        CVayne::ANIM::IDLE1,
        CVayne::ANIM::IDLE2,
        CVayne::ANIM::IDLE3,
        CVayne::ANIM::IDLE4
    };

    int idx = rand() % (_countof(sIdleAnims));
    m_pOwner->Get_Model()->Set_CurrentAnimIndex(sIdleAnims[idx]);

    // 2) Idle 유지 시간 0~5초 랜덤
    m_fIdleTime = (rand() % 5001) / 1000.f;   // 0.000 ~ 5.000

    m_fTimeDeltaAcc = 0.f;
}

void CIdleState::Exit()
{
    // 필요 시 정리작업
}