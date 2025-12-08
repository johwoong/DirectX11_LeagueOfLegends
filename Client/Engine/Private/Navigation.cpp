// Navigation.cpp
#include "Navigation.h"
#include "Cell.h"
#include "Shader.h"
#include "PipeLine.h"

// ---------------------------------
// 로컬 유틸 함수
// ---------------------------------
namespace
{
    // XZ 평면 거리
    inline float DistanceXZ(_vector a, _vector b)
    {
        _vector aXZ = XMVectorSetY(a, 0.f);
        _vector bXZ = XMVectorSetY(b, 0.f);
        return XMVectorGetX(XMVector3Length(bXZ - aXZ));
    }
}

CNavigation::CNavigation(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CComponent(pDevice, pContext)
{
}

CNavigation::CNavigation(const CNavigation& rhs)
    : CComponent(rhs)
    , m_Cells(rhs.m_Cells)
    , m_NaviDesc(rhs.m_NaviDesc)
    , m_vLastNormal(rhs.m_vLastNormal)
    , m_iLastGoalCell(rhs.m_iLastGoalCell)
    , m_bAdjustedGoal(rhs.m_bAdjustedGoal)
#ifdef _DEBUG
    , m_pShader(rhs.m_pShader)
#endif
{
    for (auto& pCell : m_Cells)
        Safe_AddRef(pCell);

#ifdef _DEBUG
    if (m_pShader)
        Safe_AddRef(m_pShader);  
#endif
}

HRESULT CNavigation::Initialize_Prototype(const _tchar* pNavigationData)
{
    _ulong dwByte = 0;

    HANDLE hFile = CreateFile(
        pNavigationData,
        GENERIC_READ, 0, nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (hFile == INVALID_HANDLE_VALUE)
        return E_FAIL;

    _uint            iNum = 0;
    CCell::CELLTYPE  eCellType = {};
    _float3          vPoints[3]{};

    ReadFile(hFile, &iNum, sizeof(_uint), &dwByte, nullptr);

    for (_uint i = 0; i < iNum; ++i)
    {
        ReadFile(hFile, vPoints, sizeof(_float3) * 3, &dwByte, nullptr);
        ReadFile(hFile, &eCellType, sizeof(CCell::CELLTYPE), &dwByte, nullptr);
        if (dwByte == 0)
            break;

        CCell* pCell = CCell::Create(m_pDevice, m_pContext, vPoints, static_cast<int>(m_Cells.size()));
        if (!pCell)
        {
            CloseHandle(hFile);
            return E_FAIL;
        }

        pCell->Set_CellType(eCellType);
        m_Cells.push_back(pCell);
    }

    CloseHandle(hFile);

    if (FAILED(SetUp_Neighbor()))
        return E_FAIL;

#ifdef _DEBUG
    m_pShader = CShader::Create(
        m_pDevice, m_pContext,
        TEXT("../../../Bin/ShaderFiles/Shader_Nav.hlsl"),
        VTXPOS_DECLARATION::Elements,
        VTXPOS_DECLARATION::iNumElements);
    if (!m_pShader)
        return E_FAIL;
#endif

    return S_OK;
}

HRESULT CNavigation::Initialize(void* pArg)
{
    if (pArg)
        memcpy(&m_NaviDesc, pArg, sizeof(m_NaviDesc));

    return S_OK;
}

// ---------------------------------
// 기본 네비게이션 기능
// ---------------------------------

_bool CNavigation::isMove(_fvector vPosition)
{
    if (m_NaviDesc.iCurrentCellIndex < 0 ||
        m_NaviDesc.iCurrentCellIndex >= static_cast<_int>(m_Cells.size()))
        return false;

    _int   iNeighborIndex = -1;
    CCell* pCurCell = m_Cells[m_NaviDesc.iCurrentCellIndex];

    // 현재 셀 안에서 움직임
    if (pCurCell->isIn(vPosition, &iNeighborIndex, &m_vLastNormal))
        return true;

    // 셀 밖으로 나감
    if (iNeighborIndex < 0)
        return false;

    // 이웃 셀 체인 따라가기
    while (true)
    {
        if (iNeighborIndex < 0)
            return false;

        CCell* pNextCell = m_Cells[iNeighborIndex];
        if (pNextCell->isIn(vPosition, &iNeighborIndex, &m_vLastNormal))
            break;
    }

    m_NaviDesc.iCurrentCellIndex = iNeighborIndex;
    return true;
}

_float CNavigation::Compute_Height(_vector vPosition, _float fOffset)
{
    if (m_NaviDesc.iCurrentCellIndex < 0 ||
        m_NaviDesc.iCurrentCellIndex >= static_cast<_int>(m_Cells.size()))
        return XMVectorGetY(vPosition);

    CCell* pCell = m_Cells[m_NaviDesc.iCurrentCellIndex];

    _vector pointA = XMLoadFloat3(&pCell->Get_PointValue(CCell::POINT_A));
    _vector pointB = XMLoadFloat3(&pCell->Get_PointValue(CCell::POINT_B));
    _vector pointC = XMLoadFloat3(&pCell->Get_PointValue(CCell::POINT_C));

    pointA = XMVectorSetW(pointA, 1.f);
    pointB = XMVectorSetW(pointB, 1.f);
    pointC = XMVectorSetW(pointC, 1.f);

    _vector vPlane = XMPlaneFromPoints(pointA, pointB, pointC);

    const float A = XMVectorGetX(vPlane);
    const float B = XMVectorGetY(vPlane);
    const float C = XMVectorGetZ(vPlane);
    const float D = XMVectorGetW(vPlane);

    const float x = XMVectorGetX(vPosition);
    const float z = XMVectorGetZ(vPosition);

    const float height = (-A * x - C * z - D) / B + fOffset;
    return height;
}

void CNavigation::Compute_CurrentIndex_byXZ(_vector vPosition)
{
    if (m_Cells.empty())
    {
        m_NaviDesc.iCurrentCellIndex = -1;
        return;
    }

    int   bestIndex = 0;
    float bestDist = FLT_MAX;

    for (int i = 0; i < static_cast<int>(m_Cells.size()); ++i)
    {
        _vector center = m_Cells[i]->Get_Center();
        float   dist = DistanceXZ(vPosition, center);

        if (dist < bestDist)
        {
            bestDist = dist;
            bestIndex = i;
        }
    }

    m_NaviDesc.iCurrentCellIndex = bestIndex;
}

_uint CNavigation::Get_CurrentCellType()
{
    if (m_NaviDesc.iCurrentCellIndex < 0 ||
        m_NaviDesc.iCurrentCellIndex >= static_cast<_int>(m_Cells.size()))
        return 0; // DEFAULT

    return static_cast<_uint>(m_Cells[m_NaviDesc.iCurrentCellIndex]->Get_CellType());
}

_vector CNavigation::Get_CurrentCellCenter()
{
    if (m_NaviDesc.iCurrentCellIndex < 0 ||
        m_NaviDesc.iCurrentCellIndex >= static_cast<_int>(m_Cells.size()))
        return XMVectorZero();

    return m_Cells[m_NaviDesc.iCurrentCellIndex]->Get_Center();
}


// A* + Funnel

bool CNavigation::FindPath(_vector startPos, _vector endPos, std::vector<int>& outCellPath)
{
    outCellPath.clear();

    m_iLastGoalCell = -1;
    m_bAdjustedGoal = false;

    const int startCell = m_NaviDesc.iCurrentCellIndex;
    if (startCell < 0 || startCell >= static_cast<int>(m_Cells.size()))
        return false;

    const int rawGoal = FindCellIndexFromPos(endPos);
    if (rawGoal < 0)
        return false;

    // 원래 목표 셀까지 먼저 시도
    if (FindPath_NoGoalAdjust(startCell, rawGoal, outCellPath))
    {
        m_iLastGoalCell = rawGoal;
        return true;
    }

    // 실패하면, 도달 가능한 셀 중 endPos와 가장 가까운 셀로 목표 조정
    const int goalCell = FindNearestReachableCell(endPos, startCell);
    if (goalCell < 0)
        return false;

    m_bAdjustedGoal = true;
    m_iLastGoalCell = goalCell;

    return FindPath_NoGoalAdjust(startCell, goalCell, outCellPath);
}

bool CNavigation::BuildFunnelPath(const std::vector<int>& cellPath,
    _vector startPos,
    _vector endPos,
    std::vector<_vector>& outPoints)
{
    outPoints.clear();
    if (cellPath.empty())
        return false;

    if (cellPath.size() < 2)
    {
        _vector fixedEnd = endPos;
        if (!IsPointOnNavMesh(fixedEnd))
        {
            const int goalCell = cellPath.back();
            fixedEnd = m_Cells[goalCell]->Get_Center();
            fixedEnd = XMVectorSetY(fixedEnd, XMVectorGetY(startPos));
        }

        outPoints.push_back(fixedEnd);
        return true;
    }

    outPoints.push_back(startPos);

    _vector apex = startPos;
    _vector left = startPos;
    _vector right = startPos;

    int apexIndex = 0;
    int leftIndex = 0;
    int rightIndex = 0;

    const int lastPortal = static_cast<int>(cellPath.size()) - 1;

    for (int i = 0; i < lastPortal; ++i)
    {
        _float3 pA3{}, pB3{};
        if (!GetPortal(cellPath[i], cellPath[i + 1], pA3, pB3))
            continue;

        _vector leftPortal = XMLoadFloat3(&pA3);
        _vector rightPortal = XMLoadFloat3(&pB3);

        const float apexY = XMVectorGetY(apex);
        leftPortal = XMVectorSetY(leftPortal, apexY);
        rightPortal = XMVectorSetY(rightPortal, apexY);

        // 포탈 살짝 안쪽으로 줄이기
        _vector mid = 0.5f * (leftPortal + rightPortal);
        const float shrink = 0.75f;
        leftPortal = XMVectorLerp(mid, leftPortal, shrink);
        rightPortal = XMVectorLerp(mid, rightPortal, shrink);

        // Right  검사
        if (TriangleArea2(apex, right, rightPortal) <= 0.0f)
        {
            if (XMVector3Equal(right, apex) ||
                TriangleArea2(apex, left, rightPortal) > 0.0f)
            {
                right = rightPortal;
                rightIndex = i + 1;
            }
            else
            {
                outPoints.push_back(left);

                apex = left;
                apexIndex = leftIndex;
                left = apex;
                right = apex;
                leftIndex = apexIndex;
                rightIndex = apexIndex;

                --i;
                continue;
            }
        }

        // Left 검사 
        if (TriangleArea2(apex, left, leftPortal) >= 0.0f)
        {
            if (XMVector3Equal(left, apex) ||
                TriangleArea2(apex, right, leftPortal) < 0.0f)
            {
                left = leftPortal;
                leftIndex = i + 1;
            }
            else
            {
                outPoints.push_back(right);

                apex = right;
                apexIndex = rightIndex;
                left = apex;
                right = apex;
                leftIndex = apexIndex;
                rightIndex = apexIndex;

                --i;
                continue;
            }
        }
    }

    // 마지막 도착점 보정
    _vector fixedEnd = endPos;
    if (!IsPointOnNavMesh(fixedEnd))
    {
        const int goalCell = cellPath.back();
        fixedEnd = m_Cells[goalCell]->Get_Center();
    }

    fixedEnd = XMVectorSetY(fixedEnd, XMVectorGetY(apex));
    outPoints.push_back(fixedEnd);

    return true;
}

// 경유지 정리 
void CNavigation::SimplifyLinearPoints(std::vector<_vector>& points, float cosThreshold)
{
    if (points.size() <= 2)
        return;

    std::vector<_vector> result;
    result.reserve(points.size());

    result.push_back(points.front());

    for (size_t i = 1; i + 1 < points.size(); ++i)
    {
        _vector a = result.back();
        _vector b = points[i];
        _vector c = points[i + 1];

        _vector ab = XMVector3Normalize(XMVectorSetY(b - a, 0.f));
        _vector bc = XMVector3Normalize(XMVectorSetY(c - b, 0.f));

        float dot = XMVectorGetX(XMVector3Dot(ab, bc));

        // 거의 일직선이고, a->c가 NavMesh 위에 있으면 b 제거
        if (dot > cosThreshold && IsSegmentOnNavMesh(a, c))
            continue;

        result.push_back(b);
    }

    result.push_back(points.back());
    points.swap(result);
}


int CNavigation::FindCellIndexFromPos(_vector pos)
{
    if (m_Cells.empty())
        return -1;

    int   bestIndex = -1;
    float bestDist = FLT_MAX;

    for (int i = 0; i < static_cast<int>(m_Cells.size()); ++i)
    {
        _vector c = m_Cells[i]->Get_Center();
        float   dist = DistanceXZ(pos, c);

        if (dist < bestDist)
        {
            bestDist = dist;
            bestIndex = i;
        }
    }

    return bestIndex;
}

bool CNavigation::GetPortal(int cellA, int cellB, _float3& outLeft, _float3& outRight)
{
    CCell* A = m_Cells[cellA];
    CCell* B = m_Cells[cellB];

    std::vector<_float3> shared;
    shared.reserve(3);

    for (int i = 0; i < 3; ++i)
    {
        const _float3& p1 = A->Get_PointValue(static_cast<CCell::POINT>(i));

        for (int j = 0; j < 3; ++j)
        {
            const _float3& p2 = B->Get_PointValue(static_cast<CCell::POINT>(j));

            if (XMVector3NearEqual(XMLoadFloat3(&p1), XMLoadFloat3(&p2), XMVectorSplatEpsilon()))
                shared.push_back(p1);
        }
    }

    if (shared.size() != 2)
        return false;

    XMVECTOR p0 = XMLoadFloat3(&shared[0]);
    XMVECTOR p1 = XMLoadFloat3(&shared[1]);
    XMVECTOR dir = XMVector3Normalize(B->Get_Center() - A->Get_Center());

    XMVECTOR right = XMVector3Cross(XMVectorSet(0, 1, 0, 0), dir);
    XMVECTOR toP0 = XMVector3Dot(right, p0 - A->Get_Center());
    XMVECTOR toP1v = XMVector3Dot(right, p1 - A->Get_Center());

    if (XMVectorGetX(toP0) < XMVectorGetX(toP1v))
    {
        outLeft = shared[0];
        outRight = shared[1];
    }
    else
    {
        outLeft = shared[1];
        outRight = shared[0];
    }

    return true;
}

bool CNavigation::IsPointOnNavMesh(_vector pos)
{
    int     dummyNeighbor = -1;
    _float3 dummyNormal{};

    for (auto& cell : m_Cells)
    {
        if (cell->isIn(pos, &dummyNeighbor, &dummyNormal))
            return true;
    }

    return false;
}

bool CNavigation::IsSegmentOnNavMesh(_vector a, _vector b)
{
    float len = DistanceXZ(a, b);

    int sampleCount = static_cast<int>(len * 4.f);
    if (sampleCount < 8)   sampleCount = 8;
    if (sampleCount > 256) sampleCount = 256;

    for (int i = 1; i <= sampleCount; ++i)
    {
        float   t = static_cast<float>(i) / static_cast<float>(sampleCount);
        _vector p = XMVectorLerp(a, b, t);

        if (!IsPointOnNavMesh(p))
            return false;
    }

    return true;
}

void CNavigation::SmoothPathPoints(std::vector<_vector>& points)
{
    if (points.size() <= 2)
        return;

    const size_t MAX_SKIP = 6;

    std::vector<_vector> result;
    result.reserve(points.size());

    size_t i = 0;
    result.push_back(points[0]);

    while (i < points.size() - 1)
    {
        size_t furthest = i + 1;
        size_t maxJ = min(points.size() - 1, i + MAX_SKIP);

        for (size_t j = maxJ; j > i + 1; --j)
        {
            if (IsSegmentOnNavMesh(points[i], points[j]))
            {
                furthest = j;
                break;
            }
        }

        result.push_back(points[furthest]);
        i = furthest;
    }

    points.swap(result);
}



float CNavigation::TriangleArea2(_vector a, _vector b, _vector c)
{
    _vector ab = b - a;
    _vector ac = c - a;
    _vector cross = XMVector3Cross(ab, ac);
    return XMVectorGetY(cross);
}

int CNavigation::FindNearestReachableCell(_vector endPos, int startCell)
{
    struct Candidate
    {
        float dist;
        int   idx;
    };

    std::vector<Candidate> candidates;
    candidates.reserve(m_Cells.size());

    for (int i = 0; i < static_cast<int>(m_Cells.size()); ++i)
    {
        _vector c = m_Cells[i]->Get_Center();
        float   dist = DistanceXZ(endPos, c);
        candidates.push_back({ dist, i });
    }

    std::sort(
        candidates.begin(), candidates.end(),
        [](const Candidate& a, const Candidate& b) { return a.dist < b.dist; });

    std::vector<int> tempPath;
    for (const auto& c : candidates)
    {
        tempPath.clear();
        if (FindPath_NoGoalAdjust(startCell, c.idx, tempPath))
            return c.idx;
    }

    return -1;
}

bool CNavigation::FindPath_NoGoalAdjust(int startCell, int goalCell, std::vector<int>& outPath)
{
    outPath.clear();

    if (startCell < 0 || goalCell < 0 ||
        startCell >= static_cast<int>(m_Cells.size()) ||
        goalCell >= static_cast<int>(m_Cells.size()))
        return false;

    std::priority_queue<NAV_NODE, std::vector<NAV_NODE>, CompareNode> open;
    std::vector<bool>  closed(m_Cells.size(), false);
    std::vector<int>   parent(m_Cells.size(), -1);
    std::vector<float> gCost(m_Cells.size(), FLT_MAX);

    _vector vGoalCenter = m_Cells[goalCell]->Get_Center();

    NAV_NODE st{};
    st.cellIndex = startCell;
    st.gCost = 0.f;
    st.hCost = XMVectorGetX(XMVector3Length(m_Cells[startCell]->Get_Center() - vGoalCenter));
    st.fCost = st.gCost + st.hCost;
    st.parent = -1;

    open.push(st);
    gCost[startCell] = 0.f;

    while (!open.empty())
    {
        NAV_NODE cur = open.top();
        open.pop();

        if (closed[cur.cellIndex])
            continue;

        closed[cur.cellIndex] = true;

        if (cur.cellIndex == goalCell)
        {
            int idx = goalCell;
            while (idx != -1)
            {
                outPath.push_back(idx);
                idx = parent[idx];
            }
            std::reverse(outPath.begin(), outPath.end());
            return true;
        }

        CCell* cell = m_Cells[cur.cellIndex];

        for (int i = 0; i < 3; ++i)
        {
            int n = cell->m_iNeighborIndices[i];
            if (n == -1)
                continue;

            float move = XMVectorGetX(
                XMVector3Length(cell->Get_Center() - m_Cells[n]->Get_Center()));
            float g = cur.gCost + move;

            if (g < gCost[n])
            {
                gCost[n] = g;
                parent[n] = cur.cellIndex;

                NAV_NODE next{};
                next.cellIndex = n;
                next.gCost = g;
                next.hCost = XMVectorGetX(
                    XMVector3Length(m_Cells[n]->Get_Center() - vGoalCenter));
                next.fCost = next.gCost + next.hCost;
                next.parent = cur.cellIndex;

                open.push(next);
            }
        }
    }

    return false;
}

#ifdef _DEBUG
HRESULT CNavigation::Render()
{
    _float4x4 world;
    XMStoreFloat4x4(&world, XMMatrixIdentity());

    CPipeLine* pPipeLine = CPipeLine::GetInstance();

    m_pShader->Set_RawValue(
        "g_ViewMatrix",
        &pPipeLine->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_VIEW),
        sizeof(_float4x4));

    m_pShader->Set_RawValue(
        "g_ProjMatrix",
        &pPipeLine->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_PROJ),
        sizeof(_float4x4));

    pPipeLine->Release();

    for (UINT i = 0; i < m_Cells.size(); ++i)
    {
        CCell* pCell = m_Cells[i];
        if (!pCell)
            continue;

        XMVECTOR vColor;
        if (static_cast<int>(i) == m_NaviDesc.iCurrentCellIndex)
            vColor = XMVectorSet(1.f, 0.f, 0.f, 1.f); // RED
        else
            vColor = XMVectorSet(0.f, 1.f, 0.f, 1.f); // GREEN

        m_pShader->Set_RawValue("g_WorldMatrix", &world, sizeof(_float4x4));
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

            if (pDestCell->Compare_Points(
                pSourCell->Get_Point(CCell::POINT_A),
                pSourCell->Get_Point(CCell::POINT_B)))
            {
                pSourCell->SetUp_Neighbor(CCell::LINE_AB, pDestCell);
            }

            if (pDestCell->Compare_Points(
                pSourCell->Get_Point(CCell::POINT_B),
                pSourCell->Get_Point(CCell::POINT_C)))
            {
                pSourCell->SetUp_Neighbor(CCell::LINE_BC, pDestCell);
            }

            if (pDestCell->Compare_Points(
                pSourCell->Get_Point(CCell::POINT_C),
                pSourCell->Get_Point(CCell::POINT_A)))
            {
                pSourCell->SetUp_Neighbor(CCell::LINE_CA, pDestCell);
            }
        }
    }

    return S_OK;
}

CNavigation* CNavigation::Create(ID3D11Device* pDevice,
    ID3D11DeviceContext* pContext,
    const _tchar* pNavigationData)
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
#endif
}
