#include "Navigation.h"
#include "Cell.h"
#include "Shader.h"
#include "PipeLine.h"

CNavigation::CNavigation(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CComponent(pDevice, pContext)
{
}

CNavigation::CNavigation(const CNavigation& rhs)
    : CComponent(rhs)
    , m_Cells(rhs.m_Cells)
#ifdef _DEBUG
    , m_pShader(rhs.m_pShader)
#endif
{
    for (auto& pCell : m_Cells)
        Safe_AddRef(pCell);

}

HRESULT CNavigation::Initialize_Prototype(const _tchar* pNavigationData)
{
    _ulong dwByte = 0;
    HANDLE  hFile = CreateFile(pNavigationData, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (0 == hFile)
        return E_FAIL;

    _uint iNum = 0;
    CCell::CELLTYPE eCellType;
    _float3 vPoints[3];

    ReadFile(hFile, &(iNum), sizeof(_uint), &dwByte, nullptr);

    for (_uint i = 0; i < iNum; ++i)
    {
        ReadFile(hFile, vPoints, sizeof(_float3) * 3, &dwByte, nullptr);
        ReadFile(hFile, &eCellType, sizeof(CCell::CELLTYPE), &dwByte, nullptr);
        if (0 == dwByte)
            break;

        CCell* pCell = CCell::Create(m_pDevice, m_pContext, vPoints, (int)m_Cells.size());
        if (nullptr == pCell)
            return E_FAIL;

        m_Cells.push_back(pCell);
        m_Cells.back()->Set_CellType(eCellType);
    }

    CloseHandle(hFile);

    if (FAILED(SetUp_Neighbor()))
        return E_FAIL;

#ifdef _DEBUG
    m_pShader = CShader::Create(m_pDevice, m_pContext, TEXT("../../../Bin/ShaderFiles/Shader_Nav.hlsl"), VTXPOS_DECLARATION::Elements, VTXPOS_DECLARATION::iNumElements);
    if (nullptr == m_pShader)
        return E_FAIL;

#endif

    return S_OK;
}

HRESULT CNavigation::Initialize(void* pArg)
{
    if (nullptr != pArg)
        memcpy(&m_NaviDesc, pArg, sizeof(m_NaviDesc));

    return S_OK;
}

_bool CNavigation::isMove(_fvector vPosition)
{
    _int       iNeighborIndex = -1;

    // 현재 존재하는 셀안에서 움직임
    if (true == m_Cells[m_NaviDesc.iCurrentCellIndex]->isIn(vPosition, &iNeighborIndex, &m_vLastNormal))
        return true;

    // 현재 존재하는 셀에서 벗어남
    else
    {
        // 나간 방향이 이웃셀이 존재하면?
        if (0 <= iNeighborIndex)
        {
            while (true)
            {
                if (-1 == iNeighborIndex)
                    return false;

                if (true == m_Cells[iNeighborIndex]->isIn(vPosition, &iNeighborIndex, &m_vLastNormal))
                    break;
            }

            m_NaviDesc.iCurrentCellIndex = iNeighborIndex;
            return true;
        }
        // 나간 방향에 이웃셀이 존재하지않음
        else
        {
            return false;
        }
    }
    return _bool();
}

_float CNavigation::Compute_Height(_vector vPosition, _float foffset)
{
    if (m_NaviDesc.iCurrentCellIndex == -1)
        return XMVectorGetY(vPosition);

    _vector PointA = XMLoadFloat3(&m_Cells[m_NaviDesc.iCurrentCellIndex]->Get_PointValue(CCell::POINT_A));
    PointA = XMVectorSetW(PointA, 1.f);
    _vector PointB = XMLoadFloat3(&m_Cells[m_NaviDesc.iCurrentCellIndex]->Get_PointValue(CCell::POINT_B));
    PointB = XMVectorSetW(PointB, 1.f);
    _vector PointC = XMLoadFloat3(&m_Cells[m_NaviDesc.iCurrentCellIndex]->Get_PointValue(CCell::POINT_C));
    PointC = XMVectorSetW(PointC, 1.f);

    _vector vPlane = XMPlaneFromPoints(PointA, PointB, PointC);
    
    _float		fHeight = (-XMVectorGetX(vPlane) * XMVectorGetX(vPosition) - XMVectorGetZ(vPlane) * XMVectorGetZ(vPosition) - XMVectorGetW(vPlane)) / XMVectorGetY(vPlane) + foffset;

    return fHeight;
}

void CNavigation::Compute_CurrentIndex_byXZ(_vector vPosition)
{
    int iIndexNum = 0;
    _float fMinDistance = 9999;
    _float fDistance;

    for (int i = 0; i < m_Cells.size(); ++i)
    {
        _vector vCellCenter = m_Cells[i]->Get_Center();
        _vector vPosYCell = XMVectorSetY(vPosition, XMVectorGetY(vCellCenter));

        fDistance = XMVectorGetX(XMVector3Length(vPosYCell - vCellCenter));

        if (fMinDistance > fDistance)
        {
            fMinDistance = fDistance;
            iIndexNum = i;
        }
    }

    m_NaviDesc.iCurrentCellIndex = iIndexNum;
}

_uint CNavigation::Get_CurrentCellType()
{
    if (m_NaviDesc.iCurrentCellIndex == 1)
        return false;

    return static_cast<_uint>(m_Cells[m_NaviDesc.iCurrentCellIndex]->Get_CellType());
}

_vector CNavigation::Get_CurrentCellCenter()
{
    return m_Cells[m_NaviDesc.iCurrentCellIndex]->Get_Center();
}

#ifdef _DEBUG
HRESULT CNavigation::Render()
{
    _float4x4 WorldMatrix;
    XMStoreFloat4x4(&WorldMatrix, XMMatrixIdentity());

    CPipeLine* pPipeLine = CPipeLine::GetInstance();

    m_pShader->Set_RawValue("g_ViewMatrix",
        &pPipeLine->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_VIEW),
        sizeof(_float4x4));

    m_pShader->Set_RawValue("g_ProjMatrix",
        &pPipeLine->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_PROJ),
        sizeof(_float4x4));

    pPipeLine->Release();


    // 모든 셀 렌더링 (조건 제거)
    for (UINT i = 0; i < m_Cells.size(); i++)
    {
        CCell* pCell = m_Cells[i];
        if (!pCell) continue;

        XMVECTOR vColor;

        // 플레이어가 위치한 셀 → 빨강
        if (i == m_NaviDesc.iCurrentCellIndex)
        {
            vColor = XMVectorSet(1.f, 0.f, 0.f, 1.f); // RED
        }
        else
        {
            vColor = XMVectorSet(0.f, 1.f, 0.f, 1.f); // GREEN
        }

        m_pShader->Set_RawValue("g_WorldMatrix", &WorldMatrix, sizeof(_float4x4));
        m_pShader->Set_RawValue("g_vColor", &vColor, sizeof(_float4));

        m_pShader->Begin(0);
        pCell->Render();
    }

    return S_OK;
}
#endif

HRESULT CNavigation::SetUp_Neighbor()
{
    for (auto& pSourCell : m_Cells)
    {
        for (auto& pDestCell : m_Cells)
        {
            if (pSourCell == pDestCell)
                continue;

            if (true == pDestCell->Compare_Points(pSourCell->Get_Point(CCell::POINT_A), pSourCell->Get_Point(CCell::POINT_B)))
            {
                pSourCell->SetUp_Neighbor(CCell::LINE_AB, pDestCell);
            }

            if (true == pDestCell->Compare_Points(pSourCell->Get_Point(CCell::POINT_B), pSourCell->Get_Point(CCell::POINT_C)))
            {
                pSourCell->SetUp_Neighbor(CCell::LINE_BC, pDestCell);
            }

            if (true == pDestCell->Compare_Points(pSourCell->Get_Point(CCell::POINT_C), pSourCell->Get_Point(CCell::POINT_A)))
            {
                pSourCell->SetUp_Neighbor(CCell::LINE_CA, pDestCell);
            }

        }
    }

    return S_OK;
}

CNavigation* CNavigation::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _tchar* pNavigationData)
{
    CNavigation* pInstance = new CNavigation(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype(pNavigationData)))
    {
        MSG_BOX("Failed to Created : CNavigation");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CComponent* CNavigation::Clone(void* pArg)
{
    CNavigation* pInstance = new CNavigation(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CNavigation");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CNavigation::Free()
{
    __super::Free();

    for (auto& pCell : m_Cells)
        Safe_Release(pCell);

    m_Cells.clear();

#ifdef _DEBUG
    Safe_Release(m_pShader);
#endif // DEBUG
}
