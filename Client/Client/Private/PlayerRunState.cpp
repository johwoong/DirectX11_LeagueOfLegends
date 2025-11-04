#include "PlayerRunState.h"

#include "GameInstance.h"
#include "PlayerIdleState.h"
#include "PickingManager.h"
#include "Terrian.h"

using namespace Player;

CRunState::CRunState(CPlayer* pPlayer)
{
    m_pOwner = pPlayer;
    m_ePlayerID = m_pOwner->Get_PlayerID();
    m_vTargetPos = m_pOwner->Get_TargetPosition();
}

CPlayerState* CRunState::HandleInput()
{
    // ? 이동 중에도 새로운 클릭 입력 가능
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
}

void CRunState::Exit()
{
}

void CRunState::Move(_float fTimeDelta)
{
    if (!m_pOwner)
        return;

    CGameInstance* pGameInstance = CGameInstance::GetInstance();
    CTransform* pTransform = m_pOwner->Get_Transform();
    if (!pTransform)
        return;

    // [1] 현재 위치, 목표 위치
    _vector vCurPos = pTransform->Get_State(CTransform::STATE_TRANSLATION);
    _vector vTarget = XMLoadFloat3(&m_vTargetPos);
    vTarget = XMVectorSetY(vTarget, XMVectorGetY(vCurPos)); // 평면 이동만

    _vector vDir = vTarget - vCurPos;
    if (XMVector3Equal(vDir, XMVectorZero()))
        return;
    vDir = XMVector3Normalize(vDir);

    // [2] Left-handed 좌표계 기준으로 회전 행렬 구성
    _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    _vector vRight = XMVector3Normalize(XMVector3Cross(vDir, vUp)); // ? 순서 수정 (Look, Up)
    _vector vLook = vDir;
    vUp = XMVector3Normalize(XMVector3Cross(vRight, vLook));

    // [3] 회전 목표 행렬 구성
    _matrix matTargetRot = XMMatrixIdentity();
    matTargetRot.r[0] = vRight;
    matTargetRot.r[1] = vUp;
    matTargetRot.r[2] = vLook;

    // [4] 현재 월드 행렬 분해
    _matrix matWorld = pTransform->Get_WorldMatrix();
    _vector vScale, vRotQuat, vTrans;
    XMMatrixDecompose(&vScale, &vRotQuat, &vTrans, matWorld);

    // [5] 목표 회전 쿼터니언 계산
    _vector vTargetScale, vTargetRotQuat, vTargetTrans;
    XMMatrixDecompose(&vTargetScale, &vTargetRotQuat, &vTargetTrans, matTargetRot);

    // [6] 회전 보간
    _vector vNewRot = XMQuaternionSlerp(vRotQuat, vTargetRotQuat, 0.2f);

    // [7] 새로운 월드 행렬 구성
    _matrix matNewWorld =
        XMMatrixScalingFromVector(vScale) *
        XMMatrixRotationQuaternion(vNewRot) *
        XMMatrixTranslationFromVector(vCurPos);

    pTransform->Set_WorldMatrix(matNewWorld);

    // [8] Look 방향 기준 이동
    _vector vLookDir = XMVector3Normalize(pTransform->Get_State(CTransform::STATE_LOOK));
    _float fSpeed = 6.f;
    _vector vNextPos = vCurPos + vLookDir * fSpeed * fTimeDelta;

    // [9] Terrain 높이 보정
    if (CTerrian* pTerrain = dynamic_cast<CTerrian*>(
        pGameInstance->Get_Object(LEVEL_GAMEPLAY, TEXT("Layer_BackGround"), 0)))
    {
        _float fY = XMVectorGetY(
            pTerrain->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION));
        vNextPos = XMVectorSetY(vNextPos, fY);
    }

    pTransform->Set_State(CTransform::STATE_TRANSLATION, vNextPos);

    // [10] 도착 판정 (Y는 무시)
    _float fDist = XMVectorGetX(
        XMVector3Length(XMVectorSetY(vTarget, 0.f) - XMVectorSetY(vNextPos, 0.f))
    );

    if (fDist < 0.25f)
    {
        // 안전하게 위치 고정
        pTransform->Set_State(CTransform::STATE_TRANSLATION, vTarget);

        // ? 현재 스케일과 회전 유지한 채 위치만 반영
        _vector vScale, vRot, vTrans;
        _matrix matWorld = pTransform->Get_WorldMatrix();
        XMMatrixDecompose(&vScale, &vRot, &vTrans, matWorld);
        _matrix matNew = XMMatrixScalingFromVector(vScale) * XMMatrixRotationQuaternion(vRot) * XMMatrixTranslationFromVector(vTarget);
        pTransform->Set_WorldMatrix(matNew);

        m_eStateType = STATETYPE_END;
    }

}
