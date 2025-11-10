#include "PlayerRunState.h"

#include "GameInstance.h"
#include "PlayerIdleState.h"
#include "PickingManager.h"
#include "Terrian.h"

using namespace Player;

CRunState::CRunState(CPlayer* pPlayer, DIRID eDir)
    : m_eDirection(eDir)
{
    m_pOwner = pPlayer;
    m_ePlayerID = m_pOwner->Get_PlayerID();
    m_vTargetPos = m_pOwner->Get_TargetPosition();
}

CPlayerState* CRunState::HandleInput()
{
    CGameInstance* pGameInstance = CGameInstance::GetInstance();

    if (pGameInstance->Mouse_Down(DIMK_RBUTTON))
    {
        _float3 vClickPos = {};

        if (CPickingManager::GetInstance()->Picking())
        { 
            vClickPos = CPickingManager::GetInstance()->Get_PickingPos();
            m_pOwner->Set_TargetPosition(vClickPos);
            m_vTargetPos = vClickPos; // ? 즉시 갱신
        }
    }

    return nullptr;
}

CPlayerState* CRunState::Update(_float fTimeDelta)
{
    HandleInput();
    Move(fTimeDelta);

    m_bIsAnimationFinished = m_pOwner->Get_Model()->Play_Animation(fTimeDelta, m_pOwner->Is_AnimationLoop(m_pOwner->Get_Model()->Get_CurrentAnimIndex()));
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
    m_eState = STATE_RUN; 
    m_eStateType = STATETYPE_START;
    switch (m_ePlayerID)
    {
    case CPlayer::EZREAL:
        if (STATETYPE_START == m_eStateType)
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::RUN);
        break;
    case CPlayer::RENEKTON:
        if (STATETYPE_START == m_eStateType)
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CRenekton::ANIM::RUN1);
    case CPlayer::VI:
        if (STATETYPE_START == m_eStateType)
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CRenekton::ANIM::ANIM_END);
    }
}

void CRunState::Exit()
{
}

void CRunState::Move(_float fTimeDelta)
{
    CTransform* pTr = m_pOwner->Get_Transform();
    if (!pTr) return;

    _vector pos = pTr->Get_State(CTransform::STATE_TRANSLATION);
    _vector tgt = XMLoadFloat3(&m_vTargetPos);

    // Y 고정 (평면 이동)
    tgt = XMVectorSetY(tgt, XMVectorGetY(pos));

    _vector toTgt = tgt - pos;
    float dist = XMVectorGetX(XMVector3Length(toTgt));

    if (dist < 0.1f)
    {
        pTr->Set_State(CTransform::STATE_TRANSLATION, tgt);
        m_eStateType = STATETYPE_END;
        return;
    }

    // 이동 방향
    _vector moveDir = XMVector3Normalize(toTgt);

    // 모델의 실제 정면은 -LOOK 이므로, 회전은 반대로
    _vector targetLook = XMVectorNegate(moveDir);

    // 부드러운 회전
    _vector curLook = XMVector3Normalize(pTr->Get_State(CTransform::STATE_LOOK));
    float t = std::clamp(XMConvertToRadians(720.f) * fTimeDelta, 0.f, 1.f);
    _vector smoothLook = XMVector3Normalize(XMVectorLerp(curLook, targetLook, t));

    // 직교 기저 재구성 포함 (스케일 유지)
    pTr->LookDir(smoothLook);

    // 회전 적용 후 → 이동 적용 (순서 중요)
    pTr->Move_PosDir(fTimeDelta, moveDir);
}
