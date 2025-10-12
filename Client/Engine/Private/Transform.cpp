#include "Transform.h"
#include "Navigation.h"

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

void CTransform::RoamingTurn(_matrix WorldMatrix, _float fRadian)
{
    _matrix		RotationMatrix = XMMatrixRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), fRadian);

    XMStoreFloat4x4(&m_WorldMatrix, XMLoadFloat4x4(&m_WorldMatrix) * RotationMatrix);
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
