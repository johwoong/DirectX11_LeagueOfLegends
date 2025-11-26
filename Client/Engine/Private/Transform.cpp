#include "Transform.h"
#include "Navigation.h"
#include "Cell.h"

CTransform::CTransform(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CComponent(pDevice, pContext)
{

}

CTransform::CTransform(const CTransform& rhs)
    : CComponent(rhs)
    , m_WorldMatrix(rhs.m_WorldMatrix)
{

}

void CTransform::Set_Scale(STATE eState, _float fScale)
{
    _matrix     WorldMatrix = XMLoadFloat4x4(&m_WorldMatrix);

    WorldMatrix.r[eState] = XMVector3Normalize(WorldMatrix.r[eState]) * fScale;

    XMStoreFloat4x4(&m_WorldMatrix, WorldMatrix);
}

HRESULT CTransform::Initialize_Prototype()
{
    XMStoreFloat4x4(&m_WorldMatrix, XMMatrixIdentity());

    return S_OK;
}

HRESULT CTransform::Initialize(void* pArg)
{
    if (nullptr != pArg)
        memcpy(&m_TransformDesc, pArg, sizeof(TRANSFORMDESC));

    return S_OK;
}

_bool CTransform::Move_Straight(_float fTimeDelta, CNavigation* pNavigation, _float fRadius)
{
    _vector vPosition = Get_State(CTransform::STATE_TRANSLATION);
    _vector vLook = Get_State(CTransform::STATE_LOOK);

    vPosition += XMVector3Normalize(vLook) * m_TransformDesc.fSpeedPerSec * fTimeDelta;

    if (nullptr == pNavigation)
        Set_State(CTransform::STATE_TRANSLATION, vPosition);
    else if (true == pNavigation->isMove(vPosition + XMVector3Normalize(vLook) * fRadius))
        Set_State(CTransform::STATE_TRANSLATION, vPosition);
    else
        return false;
    
    return true;
}

_bool CTransform::Move_BackWard(_float fTimeDelta, CNavigation* pNavigation, _float fRadius)
{
    _vector		vPosition = Get_State(CTransform::STATE_TRANSLATION);
    _vector		vLook = Get_State(CTransform::STATE_LOOK);

    vPosition -= XMVector3Normalize(vLook) * m_TransformDesc.fSpeedPerSec * fTimeDelta;

    if (nullptr == pNavigation)
        Set_State(CTransform::STATE_TRANSLATION, vPosition);

    else if (true == pNavigation->isMove(vPosition + XMVector3Normalize(vLook) * fRadius))
        Set_State(CTransform::STATE_TRANSLATION, vPosition);
    else
        return false;

    return true;
}

_bool CTransform::Move_Left(_float fTimeDelta, CNavigation* pNavigation, _float fRadius)
{
    _vector		vPosition = Get_State(CTransform::STATE_TRANSLATION);
    _vector		vRight = Get_State(CTransform::STATE_RIGHT);

    vPosition -= XMVector3Normalize(vRight) * m_TransformDesc.fSpeedPerSec * fTimeDelta;

    if (nullptr == pNavigation)
        Set_State(CTransform::STATE_TRANSLATION, vPosition);

    else if (true == pNavigation->isMove(vPosition + XMVector3Normalize(vRight) * fRadius))
        Set_State(CTransform::STATE_TRANSLATION, vPosition);
    else
        return false;

    return true;
}

_bool CTransform::Move_Right(_float fTimeDelta, CNavigation* pNavigation, _float fRadius)
{
    _vector		vPosition = Get_State(CTransform::STATE_TRANSLATION);
    _vector		vRight = Get_State(CTransform::STATE_RIGHT);

    vPosition += XMVector3Normalize(vRight) * m_TransformDesc.fSpeedPerSec * fTimeDelta;

    if (nullptr == pNavigation)
        Set_State(CTransform::STATE_TRANSLATION, vPosition);

    else if (true == pNavigation->isMove(vPosition + XMVector3Normalize(vRight) * fRadius))
        Set_State(CTransform::STATE_TRANSLATION, vPosition);
    else
        return false;

    return true;
}

_bool CTransform::Move_Up(_float fTimeDelta, CNavigation* pNavigation, _float fRadius)
{
    _vector		vPosition = Get_State(CTransform::STATE_TRANSLATION);
    _vector		vUp = Get_State(CTransform::STATE_UP);

    vPosition += XMVector3Normalize(vUp) * m_TransformDesc.fSpeedPerSec * fTimeDelta;

    if (nullptr == pNavigation)
        Set_State(CTransform::STATE_TRANSLATION, vPosition);

    else if (true == pNavigation->isMove(vPosition + XMVector3Normalize(vUp) * fRadius))
        Set_State(CTransform::STATE_TRANSLATION, vPosition);
    else
        return false;

    return true;
}

void CTransform::Jump(_float fTimeDelta, _float fVelocity, _float fGravity, _float fStartHeight)
{
    _vector		vPosition = Get_State(CTransform::STATE_TRANSLATION);
    float fPosY = fStartHeight + fVelocity * fTimeDelta - (0.5f * fGravity * fTimeDelta * fTimeDelta);

    vPosition = XMVectorSetY(vPosition, fPosY);
    float y = XMVectorGetY(vPosition);
    Set_State(CTransform::STATE_TRANSLATION, vPosition);
}

bool CTransform::Move_PosTarget(_float fTimeDelta, _vector TargetPos, CNavigation* pNavigation)
{
    _vector vPosition = Get_State(CTransform::STATE_TRANSLATION);
    _vector vTargetPos = TargetPos;
    _vector vDir = vTargetPos - vPosition;
    _float fLength = XMVectorGetX(XMVector3Length(vDir));
    vDir = XMVector3Normalize(vDir);

    if (fLength < 0.5f)
    {
        Set_State(CTransform::STATE_TRANSLATION, vTargetPos);
        return true;
    }

    vPosition += vDir * fTimeDelta * m_TransformDesc.fSpeedPerSec;

    if (nullptr == pNavigation)
        Set_State(CTransform::STATE_TRANSLATION, vPosition);
    else if (true == pNavigation->isMove(vPosition))
        Set_State(CTransform::STATE_TRANSLATION, vPosition);
    else
        return false;

    return true;
}

bool CTransform::Move_PosDir(_float fTimeDelta, _vector vecDir, CNavigation* pNavigation)
{
    _vector vPosition = Get_State(CTransform::STATE_TRANSLATION);
    _vector vDir = vecDir;

    vDir = XMVector3Normalize(vDir);

    vPosition += vDir * fTimeDelta * m_TransformDesc.fSpeedPerSec;

    if (nullptr == pNavigation)
        Set_State(CTransform::STATE_TRANSLATION, vPosition);
    else if (true == pNavigation->isMove(vPosition))
        Set_State(CTransform::STATE_TRANSLATION, vPosition);
    else
        return false;

    return true;
}

bool CTransform::Move_PosLerp(_float fTimeDelta, _vector TargetPos, _float fVecOffset)
{
    _vector vPos = Get_State(CTransform::STATE_TRANSLATION);
    _vector vDir = TargetPos - vPos;

    _float fLength = XMVectorGetX(XMVector3Length(vPos - TargetPos));
    _float  fVec = (1 - fTimeDelta) * 0 + fTimeDelta * fLength;

    vPos += XMVector3Normalize(vDir) * fVec * m_TransformDesc.fSpeedPerSec * fVecOffset;

    Set_State(CTransform::STATE_TRANSLATION, vPos);

    if (fVec == 0.0f || fLength < 0.1f)
        return true;

    return false;
}

void CTransform::Turn(_fvector vAxis, _float fTimeDelta)
{
    _matrix		RotationMatrix = XMMatrixRotationAxis(vAxis, m_TransformDesc.fRotationPerSec * fTimeDelta);

    Set_State(CTransform::STATE_RIGHT, XMVector3TransformNormal(Get_State(CTransform::STATE_RIGHT), RotationMatrix));
    Set_State(CTransform::STATE_UP, XMVector3TransformNormal(Get_State(CTransform::STATE_UP), RotationMatrix));
    Set_State(CTransform::STATE_LOOK, XMVector3TransformNormal(Get_State(CTransform::STATE_LOOK), RotationMatrix));
}

void CTransform::Rotation(_fvector vAxis, _float fRadian)
{
    _matrix		RotationMatrix = XMMatrixRotationAxis(vAxis, fRadian);

    Set_State(CTransform::STATE_RIGHT, XMVector3TransformNormal(XMVectorSet(1.f, 0.f, 0.f, 0.f) * Get_Scale(CTransform::STATE_RIGHT), RotationMatrix));
    Set_State(CTransform::STATE_UP, XMVector3TransformNormal(XMVectorSet(0.f, 1.f, 0.f, 0.f) * Get_Scale(CTransform::STATE_UP), RotationMatrix));
    Set_State(CTransform::STATE_LOOK, XMVector3TransformNormal(XMVectorSet(0.f, 0.f, 1.f, 0.f) * Get_Scale(CTransform::STATE_LOOK), RotationMatrix));
}

void CTransform::Set_Rotation(_float3 fAngle)
{
    _vector vScale, vRotationQuat, vTranslation;
    XMMatrixDecompose(&vScale, &vRotationQuat, &vTranslation, Get_WorldMatrix());
    _matrix RotationMatrix = XMMatrixRotationQuaternion(vRotationQuat);

    _matrix InverseRotationMatrix = XMMatrixInverse(nullptr, RotationMatrix);
    _matrix WorldMatrixWithoutRotation = XMMatrixMultiply(Get_WorldMatrix(), InverseRotationMatrix);

    _matrix NewRotationMatrix = XMMatrixRotationRollPitchYaw(XMConvertToRadians(fAngle.x), XMConvertToRadians(fAngle.y), XMConvertToRadians(fAngle.z));
    m_fCurrentRotationX = fAngle.x;
    m_fCurrentRotationY = fAngle.y;
    m_fCurrentRotationZ = fAngle.z;

    Set_State(CTransform::STATE_RIGHT, XMVector3TransformNormal(WorldMatrixWithoutRotation.r[0], NewRotationMatrix));
    Set_State(CTransform::STATE_UP, XMVector3TransformNormal(WorldMatrixWithoutRotation.r[1], NewRotationMatrix));
    Set_State(CTransform::STATE_LOOK, XMVector3TransformNormal(WorldMatrixWithoutRotation.r[2], NewRotationMatrix));
}

void CTransform::Set_RotationX(_float fAngleX)
{
    Set_Rotation(_float3(fAngleX, m_fCurrentRotationY, m_fCurrentRotationZ));
}

void CTransform::Set_RotationY(_float fAngleY)
{
    Set_Rotation(_float3(m_fCurrentRotationX, fAngleY, m_fCurrentRotationZ));
}

void CTransform::Set_RotationZ(_float fAngleZ)
{
    Set_Rotation(_float3(m_fCurrentRotationX, m_fCurrentRotationY, fAngleZ));
}

void CTransform::LookAt(_fvector vAt)
{
    _vector		vPosition = Get_State(CTransform::STATE_TRANSLATION);

    _vector		vLook = vAt - vPosition;
    _vector		vAxisY = XMVectorSet(0.f, 1.f, 0.f, 0.f);

    _vector		vRight = XMVector3Cross(vAxisY, vLook);
    _vector		vUp = XMVector3Cross(vLook, vRight);

    Set_State(STATE_RIGHT, XMVector3Normalize(vRight) * Get_Scale(CTransform::STATE_RIGHT));
    Set_State(STATE_UP, XMVector3Normalize(vUp) * Get_Scale(CTransform::STATE_UP));
    Set_State(STATE_LOOK, XMVector3Normalize(vLook) * Get_Scale(CTransform::STATE_LOOK));
}

void CTransform::LookDir(_fvector vDir)
{
    _vector		vPosition = Get_State(CTransform::STATE_TRANSLATION);

    _vector		vLook = vDir;
    _vector		vAxisY = XMVectorSet(0.f, 1.f, 0.f, 0.f);

    _vector		vRight = XMVector3Cross(vAxisY, vLook);
    _vector		vUp = XMVector3Cross(vLook, vRight);

    Set_State(STATE_RIGHT, XMVector3Normalize(vRight) * Get_Scale(CTransform::STATE_RIGHT));
    Set_State(STATE_UP, XMVector3Normalize(vUp) * Get_Scale(CTransform::STATE_UP));
    Set_State(STATE_LOOK, XMVector3Normalize(vLook) * Get_Scale(CTransform::STATE_LOOK));
}

void CTransform::LookAtExceptY(_fvector vAt)
{
    _vector		vLook = XMVectorSetY(vAt, m_WorldMatrix.m[3][1]) - Get_State(STATE_TRANSLATION);
    if (XMVectorGetX(XMVectorEqual(vLook, XMVectorSet(0.f, 0.f, 0.f, 0.f))) != 0)
        return;

    _vector		vAxisY = XMVectorSet(0.f, 1.f, 0.f, 0.f);

    _vector		vRight = XMVector3Cross(vAxisY, vLook);

    Set_State(STATE_RIGHT, XMVector3Normalize(vRight) * Get_Scale(STATE_RIGHT));
    Set_State(STATE_LOOK, XMVector3Normalize(vLook) * Get_Scale(STATE_LOOK));
}

void CTransform::SmoothLookAt(_vector vTargetDir, float fDeltaTime, float fRotateSpeed)
{
    // 1. 현재 WorldMatrix에서 Scale, Rot, Pos 분리
    _vector vScale, vRotQuat, vPos;
    XMMatrixDecompose(&vScale, &vRotQuat, &vPos, Get_WorldMatrix());

    // 2. 목표 회전 만들기
    _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    _vector vLook = XMVector3Normalize(vTargetDir);
    _vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vLook));
    _vector vUpFinal = XMVector3Normalize(XMVector3Cross(vLook, vRight));

    _matrix targetRotMat = XMMatrixIdentity();
    targetRotMat.r[0] = XMVectorSetW(vRight, 0.f);
    targetRotMat.r[1] = XMVectorSetW(vUpFinal, 0.f);
    targetRotMat.r[2] = XMVectorSetW(vLook, 0.f);

    _vector vTargetScale, vTargetRotQuat, vTargetPos;
    XMMatrixDecompose(&vTargetScale, &vTargetRotQuat, &vTargetPos, targetRotMat);

    // 3. 부드러운 회전 (Slerp)
    vRotQuat = XMQuaternionSlerp(vRotQuat, vTargetRotQuat, fDeltaTime * fRotateSpeed);
    vRotQuat = XMQuaternionNormalize(vRotQuat);

    // 4. 합성
    _matrix matWorld =
        XMMatrixScalingFromVector(vScale) *
        XMMatrixRotationQuaternion(vRotQuat) *
        XMMatrixTranslationFromVector(vPos);

    Set_WorldMatrix(matWorld);
}

void CTransform::RoamingTurn(_matrix WorldMatrix, _float fRadian)
{
    _matrix		RotationMatrix = XMMatrixRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), fRadian);

    XMStoreFloat4x4(&m_WorldMatrix, XMLoadFloat4x4(&m_WorldMatrix) * RotationMatrix);
}

void CTransform::Set_Destination(_vector vDest, CNavigation* pNavi)
{
    if (!pNavi) return;

    m_PathPoints.clear();
    vector<int> cellPath;

    _vector vStart = Get_State(CTransform::STATE_TRANSLATION);

    // 1) A* 로 셀 경로 찾기
    if (!pNavi->FindPath(vStart, vDest, cellPath))
    {
        m_bHasDestination = false;
        return;
    }

    if (cellPath.empty())
    {
        m_bHasDestination = false;
        return;
    }

    // ============================
    // 2) 최종 목적지 NavMesh 안으로 보정
    // ============================
    int goalCellIdx = cellPath.back();
    CCell* pGoalCell = pNavi->m_Cells[goalCellIdx];   // friend 라서 접근 가능

    // 셀 정점
    _vector A = XMLoadFloat3(&pGoalCell->Get_PointValue(CCell::POINT_A));
    _vector B = XMLoadFloat3(&pGoalCell->Get_PointValue(CCell::POINT_B));
    _vector C = XMLoadFloat3(&pGoalCell->Get_PointValue(CCell::POINT_C));

    // (1) vDest 를 삼각형 평면으로 투영
    _vector n = XMVector3Normalize(XMVector3Cross(B - A, C - A)); // 법선
    float distToPlane = XMVectorGetX(XMVector3Dot(vDest - A, n));
    _vector proj = vDest - n * distToPlane;                        // 평면 위 점

    // (2) barycentric 좌표 계산
    _vector v0 = B - A;
    _vector v1 = C - A;
    _vector v2 = proj - A;

    float d00 = XMVectorGetX(XMVector3Dot(v0, v0));
    float d01 = XMVectorGetX(XMVector3Dot(v0, v1));
    float d11 = XMVectorGetX(XMVector3Dot(v1, v1));
    float d20 = XMVectorGetX(XMVector3Dot(v2, v0));
    float d21 = XMVectorGetX(XMVector3Dot(v2, v1));

    float denom = d00 * d11 - d01 * d01;
    float u = 1.f, v = 0.f, w = 0.f;

    if (fabsf(denom) > FLT_EPSILON)
    {
        float invDenom = 1.f / denom;
        v = (d11 * d20 - d01 * d21) * invDenom;
        w = (d00 * d21 - d01 * d20) * invDenom;
        u = 1.f - v - w;
    }

    // (3) 삼각형 밖이면 가장 가까운 점으로 clamp
    u = max(0.f, min(1.f, u));
    v = max(0.f, min(1.f, v));
    w = max(0.f, min(1.f, w));
    float sum = u + v + w;
    if (sum > 0.f)
    {
        u /= sum;
        v /= sum;
        w /= sum;
    }

    _vector vGoalOnCell = A * u + B * v + C * w;

    // Nav 높이에 맞추기
    float h = pNavi->Compute_Height(vGoalOnCell, 0.f);
    vGoalOnCell = XMVectorSetY(vGoalOnCell, h);

    // ============================
    // 3) Funnel 은 "보정된 목적지" 로 수행
    // ============================
    if (!pNavi->BuildFunnelPath(cellPath, vStart, vGoalOnCell, m_PathPoints))
    {
        m_bHasDestination = false;
        return;
    }

    // 1) 포탈 shrink 적용된 Funnel 결과에 대해, 과도하지 않게 스무딩
    pNavi->SmoothPathPoints(m_PathPoints);

    // 2) 거의 직선인 것만 합치기 (각도 20~25도 기준)
    pNavi->SimplifyLinearPoints(
        m_PathPoints,
        cosf(XMConvertToRadians(22.f))   // 필요하면 18~25 사이에서 튜닝
    );

    for (auto& point : m_PathPoints)
    {
        float ph = pNavi->Compute_Height(point, 0.f);
        point = XMVectorSetY(point, ph);
    }


    m_iPathIndex = 0;
    m_bHasDestination = true;
}

bool CTransform::Move_ByPath(_float fDeltaTime, CNavigation* pNavi)
{
    if (!m_bHasDestination || !pNavi)
        return false;

    if (m_iPathIndex >= (int)m_PathPoints.size())
    {
        m_bHasDestination = false;
        return false;
    }

    _vector vPos = Get_State(STATE_TRANSLATION);
    _vector vTarget = m_PathPoints[m_iPathIndex];

    // XZ 거리만 체크
    float distXZ = XMVectorGetX(
        XMVector3Length(
            XMVectorSetY(vTarget, XMVectorGetY(vPos)) -
            XMVectorSetY(vPos, XMVectorGetY(vPos))));

    // 현재 타겟 경유점에 거의 도착했다면 다음 포인트로 넘김
    if (distXZ < 0.15f)
    {
        ++m_iPathIndex;
        if (m_iPathIndex >= (int)m_PathPoints.size())
        {
            m_bHasDestination = false;
            return true;
        }

        vTarget = m_PathPoints[m_iPathIndex];

        distXZ = XMVectorGetX(
            XMVector3Length(
                XMVectorSetY(vTarget, XMVectorGetY(vPos)) -
                XMVectorSetY(vPos, XMVectorGetY(vPos))));
    }

    // XZ 방향
    _vector vDir = XMVector3Normalize(
        XMVectorSetY(vTarget - vPos, 0.f));

    // 이번 프레임 이동 가능 거리
    float step = m_TransformDesc.fSpeedPerSec * fDeltaTime;

    // step 이 남은 거리보다 크면, 정확히 타겟까지만 이동
    _vector vNextPos;
    if (distXZ <= step)
        vNextPos = XMVectorSetY(vTarget, XMVectorGetY(vPos));
    else
        vNextPos = vPos + vDir * step;


    // 현재 위치 기준으로 가장 가까운 셀 인덱스 갱신
    pNavi->Compute_CurrentIndex_byXZ(vNextPos);

    // 그 셀 기준 높이 보정
    float newY = pNavi->Compute_Height(vNextPos, 0.f);
    vNextPos = XMVectorSetY(vNextPos, newY);

    // 실제 위치 반영
    Set_State(STATE_TRANSLATION, vNextPos);

    // 회전도 살짝
    SmoothLookAt(-vDir, fDeltaTime, 16.f);

    return true;
}

CTransform* CTransform::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CTransform* pInstance = new CTransform(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CTransform");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CComponent* CTransform::Clone(void* pArg)
{
    CTransform* pInstance = new CTransform(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CTransform");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CTransform::Free()
{
    __super::Free();
}
