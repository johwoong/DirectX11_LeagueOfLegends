#include "CColider.h"
#include "DebugDraw.h"
#include "GameInstance.h"

CColider::CColider(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CComponent(pDevice, pContext)
{
}

CColider::CColider(const CColider& rhs)
    : CComponent(rhs)
    , m_eType(rhs.m_eType)
#ifdef _DEBUG
    , m_pBatch(rhs.m_pBatch)
    , m_pEffect(rhs.m_pEffect)
    , m_pInputLayout(rhs.m_pInputLayout)
    , m_pDepthStencilState(rhs.m_pDepthStencilState)
#endif
{
#ifdef _DEBUG
    Safe_AddRef(m_pInputLayout);
    Safe_AddRef(m_pDepthStencilState);
#endif
}

HRESULT CColider::Initialize_Prototype(TYPE eType)
{
    m_eType = eType;

#ifdef _DEBUG
    m_pBatch = new PrimitiveBatch<VertexPositionColor>(m_pContext);
    m_pEffect = new BasicEffect(m_pDevice);

    m_pEffect->SetVertexColorEnabled(true);

    const void* pShaderbyteCode = nullptr;
    size_t      iShaderByteCodeLength = 0;

    m_pEffect->GetVertexShaderBytecode(&pShaderbyteCode, &iShaderByteCodeLength);

    if (FAILED(m_pDevice->CreateInputLayout(VertexPositionColor::InputElements, VertexPositionColor::InputElementCount, pShaderbyteCode, iShaderByteCodeLength, &m_pInputLayout)))
        return E_FAIL;

    D3D11_DEPTH_STENCIL_DESC			DepthStecilDesc;
    ZeroMemory(&DepthStecilDesc, sizeof(DepthStecilDesc));

    DepthStecilDesc.DepthEnable = TRUE;
    DepthStecilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    DepthStecilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    DepthStecilDesc.StencilEnable = FALSE;

    if (FAILED(m_pDevice->CreateDepthStencilState(&DepthStecilDesc, &m_pDepthStencilState)))
        return E_FAIL;
#endif
     
    return S_OK;
}

HRESULT CColider::Initialize(void* pArg)
{
    if (nullptr == pArg)
        return E_FAIL;

    memcpy(&m_ColiderDesc, pArg, sizeof(COLLIDERDESC));

    _matrix ScaleMatrix, RotationMatrix, TranslationMatrix;

    ScaleMatrix = XMMatrixScaling(m_ColiderDesc.vScale.x, m_ColiderDesc.vScale.y, m_ColiderDesc.vScale.z);
    _matrix RotationXMatrix = XMMatrixRotationX(m_ColiderDesc.vRotation.x);
    _matrix RotationYMatrix = XMMatrixRotationY(m_ColiderDesc.vRotation.y);
    _matrix RotationZMatrix = XMMatrixRotationZ(m_ColiderDesc.vRotation.z);
    RotationMatrix = RotationXMatrix * RotationYMatrix * RotationZMatrix;

    TranslationMatrix = XMMatrixTranslation(m_ColiderDesc.vPosition.x, m_ColiderDesc.vPosition.y, m_ColiderDesc.vPosition.z);

    for (_uint i = 0; i < BOUNDING_END; ++i)
    {
        switch (m_eType)
        {
        case TYPE_AABB:
            m_pAABB[i] = new BoundingBox(_float3(0.f, 0.f, 0.f), _float3(0.5f, 0.5f, 0.5f));
            m_pOriAABB[i] = new BoundingBox(_float3(0.f, 0.f, 0.f), _float3(0.5f, 0.5f, 0.5f));
            m_pOriAABB[i]->Transform(*m_pAABB[i], ScaleMatrix * TranslationMatrix);
            break;
        case TYPE_OBB:
            m_pOBB[i] = new BoundingOrientedBox(m_ColiderDesc.vPosition, _float3(m_ColiderDesc.vScale.x * 0.5f, m_ColiderDesc.vScale.y * 0.5f, m_ColiderDesc.vScale.z * 0.5f), _float4(0.f, 0.f, 0.f, 1.f));
            m_pOriOBB[i] = new BoundingOrientedBox(m_ColiderDesc.vPosition, _float3(m_ColiderDesc.vScale.x * 0.5f, m_ColiderDesc.vScale.y * 0.5f, m_ColiderDesc.vScale.z * 0.5f), _float4(0.f, 0.f, 0.f, 1.f));
            m_pOriOBB[i]->Transform(*m_pOBB[i], RotationMatrix);
            break;
        case TYPE_SPHERE:
            m_pSphere[i] = new BoundingSphere(_float3(0.f, 0.f, 0.f), 50.f);
            m_pOriSphere[i] = new BoundingSphere(_float3(0.f, 0.f, 0.f), 50.f);
            m_pOriSphere[i]->Transform(*m_pSphere[i], ScaleMatrix * RotationMatrix * TranslationMatrix);
            break;
        }
    }
    
    return S_OK;
}

void CColider::Update(_fmatrix WorldMatrix)
{
    switch (m_eType)
    {
    case TYPE_AABB:
        m_pAABB[BOUNDING_ORIGINAL]->Transform(*m_pAABB[BOUNDING_WORLD], Remove_Rotation(WorldMatrix));
        break;
    case TYPE_OBB:
        m_pOBB[BOUNDING_ORIGINAL]->Transform(*m_pOBB[BOUNDING_WORLD], WorldMatrix);
        break;
    case TYPE_SPHERE:
        m_pSphere[BOUNDING_ORIGINAL]->Transform(*m_pSphere[BOUNDING_WORLD], WorldMatrix);
        break;
    }
}


HRESULT CColider::Set_Colider_Desc(void* pArg)
{
    if (nullptr == pArg)
        return E_FAIL;

    memcpy(&m_ColiderDesc, pArg, sizeof(COLLIDERDESC));

    _matrix     ScaleMatrix, RotationMatrix, TranslationMatrix;

    ScaleMatrix = XMMatrixScaling(m_ColiderDesc.vScale.x, m_ColiderDesc.vScale.y, m_ColiderDesc.vScale.z);
    _matrix RotationXMatrix = XMMatrixRotationX(m_ColiderDesc.vRotation.x);
    _matrix RotationYMatrix = XMMatrixRotationY(m_ColiderDesc.vRotation.y);
    _matrix RotationZMatrix = XMMatrixRotationZ(m_ColiderDesc.vRotation.z);
    RotationMatrix = RotationXMatrix * RotationYMatrix * RotationZMatrix;

    TranslationMatrix = XMMatrixTranslation(m_ColiderDesc.vPosition.x, m_ColiderDesc.vPosition.y, m_ColiderDesc.vPosition.z);

    for (_uint i = 0; i < BOUNDING_END; ++i)
    {
        switch (m_eType)
        {
        case TYPE_AABB:
            m_pOriAABB[i]->Transform(*m_pAABB[i], ScaleMatrix * TranslationMatrix);
            break;
        case TYPE_OBB:
            m_pOriOBB[i]->Transform(*m_pOBB[i], RotationMatrix);
            break;
        case TYPE_SPHERE:
            m_pOriSphere[i]->Transform(*m_pSphere[i], ScaleMatrix * RotationMatrix * TranslationMatrix);
            break;
        }
    }

    return S_OK;

}

void CColider::Set_Sphere_Radius(float radius)
{
    if (m_eType != TYPE_SPHERE) return;

    for (int i = 0; i < BOUNDING_END; i++)
    {
        m_pSphere[i]->Radius = radius;
        m_pOriSphere[i]->Radius = radius;
    }
}

void CColider::Set_Scale(_float3 vScale)
{
    m_ColiderDesc.vScale = vScale;
    Set_Colider_Desc(&m_ColiderDesc);
}

bool CColider::RayPick(CTransform* pTransform, _float3* pOutHitPos, float* pOutDist)
{
    if (!pTransform)
        return false;

    // 1) CPicking 에서 "로컬 레이" 정보 받아오기
    CPicking* pPicking = CPicking::GetInstance();
    pPicking->AddRef();

    _float3 vLocalRayDir{}, vLocalRayPos{};
    pPicking->Compute_LocalRayInfo(&vLocalRayDir, &vLocalRayPos, pTransform);

    pPicking->Release();

    // 2) 로컬 레이를 월드 레이로 변환
    XMVECTOR vRayPosLocal = XMLoadFloat3(&vLocalRayPos);
    XMVECTOR vRayDirLocal = XMVector3Normalize(XMLoadFloat3(&vLocalRayDir));

    XMMATRIX world = pTransform->Get_WorldMatrix();

    // 로컬 → 월드
    XMVECTOR vRayPos = XMVector3TransformCoord(vRayPosLocal, world);
    XMVECTOR vRayDir = XMVector3Normalize(
        XMVector3TransformNormal(vRayDirLocal, world));

    float dist = 0.f;
    bool  hit = false;

    // 3) 콜라이더 타입에 따라 레이와 교차 테스트
    switch (m_eType)
    {
    case TYPE_SPHERE:
        if (m_pSphere[BOUNDING_WORLD])
            hit = m_pSphere[BOUNDING_WORLD]->Intersects(vRayPos, vRayDir, dist);
        break;

    case TYPE_OBB:
        if (m_pOBB[BOUNDING_WORLD])
            hit = m_pOBB[BOUNDING_WORLD]->Intersects(vRayPos, vRayDir, dist);
        break;

    case TYPE_AABB:
        if (m_pAABB[BOUNDING_WORLD])
            hit = m_pAABB[BOUNDING_WORLD]->Intersects(vRayPos, vRayDir, dist);
        break;
    }

    if (!hit)
        return false;

    if (pOutDist)
        *pOutDist = dist;

    if (pOutHitPos)
    {
        XMVECTOR vHit = vRayPos + vRayDir * dist;
        XMStoreFloat3(pOutHitPos, vHit);
    }

    return true;
}

#ifdef _DEBUG
HRESULT CColider::Render()
{
    m_pContext->OMSetDepthStencilState(m_pDepthStencilState, 0);

    m_pContext->GSSetShader(nullptr, nullptr, 0);

    m_pBatch->Begin();

    m_pContext->IASetInputLayout(m_pInputLayout);

    m_pEffect->SetWorld(XMMatrixIdentity());

    CGameInstance* pGameInstance = CGameInstance::GetInstance();
    pGameInstance->AddRef();

    m_pEffect->SetView(pGameInstance->Get_TransformMatrix(CPipeLine::D3DTS_VIEW));
    m_pEffect->SetProjection(pGameInstance->Get_TransformMatrix(CPipeLine::D3DTS_PROJ));

    pGameInstance->Release();

    m_pEffect->Apply(m_pContext);

    _vector		vColor = m_isCollision == true ? XMVectorSet(1.f, 0.f, 0.f, 1.f) : XMVectorSet(0.f, 1.f, 0.f, 1.f);

    switch (m_eType)
    {
    case CColider::TYPE_AABB:
        DX::Draw(m_pBatch, *m_pAABB[BOUNDING_WORLD], vColor);
        break;
    case CColider::TYPE_OBB:
        DX::Draw(m_pBatch, *m_pOBB[BOUNDING_WORLD], vColor);
        break;
    case CColider::TYPE_SPHERE:
        DX::Draw(m_pBatch, *m_pSphere[BOUNDING_WORLD], vColor);
        break;
    }

    m_pBatch->End();

    return S_OK;
}
#endif

_bool CColider::Collision(CColider* pTargetColider)
{
    m_isCollision = false;

    if (TYPE_AABB == m_eType)
    {
        if (TYPE_AABB == pTargetColider->m_eType)
            m_isCollision = m_pAABB[BOUNDING_WORLD]->Intersects(*pTargetColider->m_pAABB[BOUNDING_WORLD]);
        if (TYPE_OBB == pTargetColider->m_eType)
            m_isCollision = m_pAABB[BOUNDING_WORLD]->Intersects(*pTargetColider->m_pOBB[BOUNDING_WORLD]);
        if (TYPE_SPHERE == pTargetColider->m_eType)
            m_isCollision = m_pAABB[BOUNDING_WORLD]->Intersects(*pTargetColider->m_pSphere[BOUNDING_WORLD]);
    }

    if (TYPE_OBB == m_eType)
    {
        if (isnan(m_pOBB[BOUNDING_WORLD]->Center.x))
            return false;

        if (TYPE_AABB == pTargetColider->m_eType)
            m_isCollision = m_pOBB[BOUNDING_WORLD]->Intersects(*pTargetColider->m_pAABB[BOUNDING_WORLD]);
        if (TYPE_OBB == pTargetColider->m_eType)
            m_isCollision = m_pOBB[BOUNDING_WORLD]->Intersects(*pTargetColider->m_pOBB[BOUNDING_WORLD]);
        if (TYPE_SPHERE == pTargetColider->m_eType)
            m_isCollision = m_pOBB[BOUNDING_WORLD]->Intersects(*pTargetColider->m_pSphere[BOUNDING_WORLD]);
    }

    if (TYPE_SPHERE == m_eType)
    {
        if (TYPE_AABB == pTargetColider->m_eType)
            m_isCollision = m_pSphere[BOUNDING_WORLD]->Intersects(*pTargetColider->m_pAABB[BOUNDING_WORLD]);
        if (TYPE_OBB == pTargetColider->m_eType)
        {

            if (isnan(pTargetColider->m_pOBB[BOUNDING_WORLD]->Center.x))
                return false;

            m_isCollision = m_pSphere[BOUNDING_WORLD]->Intersects(*pTargetColider->m_pOBB[BOUNDING_WORLD]);
        }

        if (TYPE_SPHERE == pTargetColider->m_eType)
            m_isCollision = m_pSphere[BOUNDING_WORLD]->Intersects(*pTargetColider->m_pSphere[BOUNDING_WORLD]);
    }


    return m_isCollision;
}



_bool CColider::Collision_AABB(CColider* pTargetColider)
{
    _float3     vSourMin, vSourMax;
    _float3     vDestMin, vDestMax;

    vSourMin = Compute_Min();
    vSourMax = Compute_Max();

    vDestMin = pTargetColider->Compute_Min();
    vDestMax = pTargetColider->Compute_Max();

    m_isCollision = true;

    // 너비 비교
    if (max(vSourMin.x, vSourMax.x) > min(vSourMax.x, vDestMax.x))
        m_isCollision = false;

    if (max(vSourMin.y, vDestMin.y) > min(vSourMax.y, vDestMax.y))
        m_isCollision = false;

    if (max(vSourMin.z, vDestMin.z) > min(vSourMax.z, vDestMax.z))
        m_isCollision = false;

    return m_isCollision;
}

_bool CColider::Collision_OBB(CColider* pTargetColider)
{
    OBBDESC     OBBDesc[2];

    OBBDesc[0] = Compute_OBBDesc();
    OBBDesc[1] = pTargetColider->Compute_OBBDesc();

    m_isCollision = true;

    _float fDistance[3];

    for (_uint i = 0; i < 2; ++i)
    {
        for (_uint j = 0; j < 3; ++j)
        {
            fDistance[0] = fabs(XMVectorGetX(XMVector3Dot(XMLoadFloat3(&OBBDesc[1].vCenter) - XMLoadFloat3(&OBBDesc[0].vCenter),
                XMLoadFloat3(&OBBDesc[i].vAliginAxis[j]))));

            fDistance[1] = fabs(XMVectorGetX(XMVector3Dot(XMLoadFloat3(&OBBDesc[0].vCenterAxis[0]), XMLoadFloat3(&OBBDesc[i].vAliginAxis[j])))) +
                fabs(XMVectorGetX(XMVector3Dot(XMLoadFloat3(&OBBDesc[0].vCenterAxis[1]), XMLoadFloat3(&OBBDesc[i].vAliginAxis[j])))) +
                fabs(XMVectorGetX(XMVector3Dot(XMLoadFloat3(&OBBDesc[0].vCenterAxis[2]), XMLoadFloat3(&OBBDesc[i].vAliginAxis[j]))));

            fDistance[2] = fabs(XMVectorGetX(XMVector3Dot(XMLoadFloat3(&OBBDesc[1].vCenterAxis[0]), XMLoadFloat3(&OBBDesc[i].vAliginAxis[j])))) +
                fabs(XMVectorGetX(XMVector3Dot(XMLoadFloat3(&OBBDesc[1].vCenterAxis[1]), XMLoadFloat3(&OBBDesc[i].vAliginAxis[j])))) +
                fabs(XMVectorGetX(XMVector3Dot(XMLoadFloat3(&OBBDesc[1].vCenterAxis[2]), XMLoadFloat3(&OBBDesc[i].vAliginAxis[j]))));

            if (fDistance[0] > fDistance[1] + fDistance[2])
                m_isCollision = false;
        }
    }

    return m_isCollision;
}

_matrix CColider::Remove_Rotation(_fmatrix Matrix)
{
    _matrix		TransformMatrix = Matrix;

    TransformMatrix.r[0] = XMVectorSet(1.f, 0.f, 0.f, 0.f) * XMVectorGetX(XMVector3Length(Matrix.r[0]));
    TransformMatrix.r[1] = XMVectorSet(0.f, 1.f, 0.f, 0.f) * XMVectorGetX(XMVector3Length(Matrix.r[1]));
    TransformMatrix.r[2] = XMVectorSet(0.f, 0.f, 1.f, 0.f) * XMVectorGetX(XMVector3Length(Matrix.r[2]));

    return TransformMatrix;
}

_float3 CColider::Compute_Min()
{
    if (nullptr == m_pAABB[BOUNDING_WORLD])
        return _float3(0.f, 0.f, 0.f);

    return _float3(m_pAABB[BOUNDING_WORLD]->Center.x - m_pAABB[BOUNDING_WORLD]->Extents.x,
        m_pAABB[BOUNDING_WORLD]->Center.y - m_pAABB[BOUNDING_WORLD]->Extents.y,
        m_pAABB[BOUNDING_WORLD]->Center.z - m_pAABB[BOUNDING_WORLD]->Extents.z);
}

_float3 CColider::Compute_Max()
{
    if (nullptr == m_pAABB[BOUNDING_WORLD])
        return _float3(0.f, 0.f, 0.f);

    return _float3(m_pAABB[BOUNDING_WORLD]->Center.x + m_pAABB[BOUNDING_WORLD]->Extents.x,
        m_pAABB[BOUNDING_WORLD]->Center.y + m_pAABB[BOUNDING_WORLD]->Extents.y,
        m_pAABB[BOUNDING_WORLD]->Center.z + m_pAABB[BOUNDING_WORLD]->Extents.z);
}

CColider::OBBDESC CColider::Compute_OBBDesc()
{
    OBBDESC		OBBDesc;
    ZeroMemory(&OBBDesc, sizeof(OBBDESC));

    _float3		vPoints[8];

    m_pOBB[BOUNDING_WORLD]->GetCorners(vPoints);

    XMStoreFloat3(&OBBDesc.vCenter, (XMLoadFloat3(&vPoints[2]) + XMLoadFloat3(&vPoints[4])) * 0.5f);

    XMStoreFloat3(&OBBDesc.vAliginAxis[0], XMVector3Normalize(XMLoadFloat3(&vPoints[2]) - XMLoadFloat3(&vPoints[3])));
    XMStoreFloat3(&OBBDesc.vAliginAxis[1], XMVector3Normalize(XMLoadFloat3(&vPoints[2]) - XMLoadFloat3(&vPoints[1])));
    XMStoreFloat3(&OBBDesc.vAliginAxis[2], XMVector3Normalize(XMLoadFloat3(&vPoints[2]) - XMLoadFloat3(&vPoints[6])));

    XMStoreFloat3(&OBBDesc.vCenterAxis[0], XMLoadFloat3(&OBBDesc.vAliginAxis[0]) * m_pOBB[BOUNDING_WORLD]->Extents.x);
    XMStoreFloat3(&OBBDesc.vCenterAxis[1], XMLoadFloat3(&OBBDesc.vAliginAxis[1]) * m_pOBB[BOUNDING_WORLD]->Extents.y);
    XMStoreFloat3(&OBBDesc.vCenterAxis[2], XMLoadFloat3(&OBBDesc.vAliginAxis[2]) * m_pOBB[BOUNDING_WORLD]->Extents.z);

    return OBBDesc;
}

CColider* CColider::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, TYPE eType)
{
    CColider* pInstance = new CColider(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype(eType)))
    {
        MSG_BOX("Failed to Created : CCollider");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CComponent* CColider::Clone(void* pArg)
{
    CColider* pInstance = new CColider(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CCollider");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CColider::Free()
{
    __super::Free();

#ifdef _DEBUG
    if (false == m_isCloned)
    {
        Safe_Delete(m_pBatch);
        Safe_Delete(m_pEffect);
    }

    Safe_Release(m_pDepthStencilState);
    Safe_Release(m_pInputLayout);
#endif // _DEBUG

    for (_uint i = 0; i < BOUNDING_END; ++i)
    {
        Safe_Delete(m_pAABB[i]);
        Safe_Delete(m_pOBB[i]);
        Safe_Delete(m_pSphere[i]);
    }

    for (_uint i = 0; i < BOUNDING_END; ++i)
    {
        Safe_Delete(m_pOriAABB[i]);
        Safe_Delete(m_pOriOBB[i]);
        Safe_Delete(m_pOriSphere[i]);
    }
}
