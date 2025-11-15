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
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CRenekton::ANIM::RUN);
    case CPlayer::VI:
        if (STATETYPE_START == m_eStateType)
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CRenekton::ANIM::RUN);
    }
}

void CRunState::Exit()
{
}

void CRunState::Move(_float fTimeDelta)
{
    CTransform* pTr = m_pOwner->Get_Transform();
    if (!pTr) return;

    _vector vPos = pTr->Get_State(CTransform::STATE_TRANSLATION);
    _vector vTarget = XMLoadFloat3(&m_vTargetPos);
    vTarget = XMVectorSetY(vTarget, XMVectorGetY(vPos));

    _vector vToTarget = vTarget - vPos;
    float fDist = XMVectorGetX(XMVector3Length(vToTarget));

    if (fDist < 0.05f)
    {
        m_eStateType = STATETYPE_END;
        return;
    }


    _vector vDir = XMVector3Normalize(vToTarget);
    _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);


    _vector vLook = -vDir;
    _vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vLook));
    _vector vUpFinal = XMVector3Normalize(XMVector3Cross(vLook, vRight));


    _matrix rotMat = XMMatrixIdentity();
    rotMat.r[0] = XMVectorSetW(vRight, 0.f);
    rotMat.r[1] = XMVectorSetW(vUpFinal, 0.f);
    rotMat.r[2] = XMVectorSetW(vLook, 0.f);

    _vector vScale, vRotQuat, vCurPos;
    XMMatrixDecompose(&vScale, &vRotQuat, &vCurPos, pTr->Get_WorldMatrix());

    _vector vTargetScale, vTargetRotQuat, vTargetPos;
    XMMatrixDecompose(&vTargetScale, &vTargetRotQuat, &vTargetPos, rotMat);

    vRotQuat = XMQuaternionSlerp(vRotQuat, vTargetRotQuat, fTimeDelta * 10.f);
    vRotQuat = XMQuaternionNormalize(vRotQuat);


    float fSpeed = 6.f;
    vPos += vDir * fSpeed * fTimeDelta;

    _matrix matWorld =
        XMMatrixScalingFromVector(vScale) *
        XMMatrixRotationQuaternion(vRotQuat) *
        XMMatrixTranslationFromVector(vPos);

    pTr->Set_WorldMatrix(matWorld);

}
