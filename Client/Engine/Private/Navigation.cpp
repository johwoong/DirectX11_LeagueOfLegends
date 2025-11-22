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

bool CNavigation::FindPath(_vector startPos, _vector endPos, vector<int>& outCellPath)
{
    outCellPath.clear();

    // 기록 초기화
    m_iLastGoalCell = -1;
    m_bAdjustedGoal = false;

    int startCell = m_NaviDesc.iCurrentCellIndex;
    if (startCell < 0)
        return false;

    int rawGoal = FindCellIndexFromPos(endPos);
    if (rawGoal < 0)
        return false;

    // 1) 원래 목표 셀까지 먼저 시도
    if (FindPath_NoGoalAdjust(startCell, rawGoal, outCellPath))
    {
        m_iLastGoalCell = rawGoal;     // 그대로 사용
        return true;
    }

    // 2) 실패하면, 도달 가능한 셀 중 endPos와 가장 가까운 셀로 목표 조정
    int goalCell = FindNearestReachableCell(endPos, startCell);
    if (goalCell < 0)
        return false;

    m_bAdjustedGoal = true;            // ★ 목표가 바뀌었음을 표시
    m_iLastGoalCell = goalCell;

    return FindPath_NoGoalAdjust(startCell, goalCell, outCellPath);
}

bool CNavigation::BuildFunnelPath(const vector<int>& cellPath, _vector startPos, _vector endPos, vector<_vector>& outPoints)
{
    outPoints.clear();

    if (cellPath.size() < 2)
    {
        // ★ 여기도 endPos가 NavMesh 밖이면 보정
        _vector fixedEnd = endPos;
        if (!IsPointOnNavMesh(fixedEnd))
        {
            int goalCell = cellPath.back();
            fixedEnd = m_Cells[goalCell]->Get_Center();
            fixedEnd = XMVectorSetY(fixedEnd, XMVectorGetY(startPos));
        }

        outPoints.push_back(fixedEnd);
        return true;
    }

    // 시작점 추가
    outPoints.push_back(startPos);

    _vector apex = startPos;
    _vector left = startPos;
    _vector right = startPos;

    int apexIndex = 0;
    int leftIndex = 0;
    int rightIndex = 0;

    const int lastPortal = (int)cellPath.size() - 1;

    for (int i = 0; i < lastPortal; ++i)
    {
        _float3 pA3, pB3;
        if (!GetPortal(cellPath[i], cellPath[i + 1], pA3, pB3))
            continue;

        _vector leftPortal = XMLoadFloat3(&pA3);
        _vector rightPortal = XMLoadFloat3(&pB3);

        leftPortal = XMVectorSetY(leftPortal, XMVectorGetY(apex));
        rightPortal = XMVectorSetY(rightPortal, XMVectorGetY(apex));

        // =====================
// ★ 포탈 살짝 안쪽으로 줄이기 ★
// =====================
        _vector mid = 0.5f * (leftPortal + rightPortal); // 포탈 중앙
        const float portalShrink = 0.75f; // 1.0 = 기존, 0.7~0.85 사이 튜닝

        leftPortal = XMVectorLerp(mid, leftPortal, portalShrink);
        rightPortal = XMVectorLerp(mid, rightPortal, portalShrink);

        // ----- Right Test -----
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

        // ----- Left Test -----
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

    // ★★★ 여기 핵심 수정 ★★★
    // 마지막 도착점은 반드시 NavMesh 안에 있는 점을 사용
    _vector fixedEnd = endPos;

    // 클릭 좌표가 NavMesh 밖이거나, 셀 구멍 쪽이면 여기서 보정됨
    if (!IsPointOnNavMesh(fixedEnd))
    {
        int goalCell = cellPath.back();                     // 실제 마지막 셀
        fixedEnd = m_Cells[goalCell]->Get_Center();         // 셀 센터 사용 (원하면 나중에 삼각형 투영으로 바꿔도 됨)
    }

    // 높이는 현재 apex 기준으로 맞춰줌
    fixedEnd = XMVectorSetY(fixedEnd, XMVectorGetY(apex));

    outPoints.push_back(fixedEnd);

    return true;
}

void CNavigation::SimplifyLinearPoints(vector<_vector>& points, float cosThreshold)
{
    if (points.size() <= 2)
        return;

    vector<_vector> result;
    result.reserve(points.size());

    // 시작점은 무조건 유지
    result.push_back(points.front());

    for (size_t i = 1; i + 1 < points.size(); ++i)
    {
        _vector a = result.back();
        _vector b = points[i];
        _vector c = points[i + 1];

        // XZ 평면 기준으로 방향 계산
        _vector ab = XMVector3Normalize(XMVectorSetY(b - a, 0.f));
        _vector bc = XMVector3Normalize(XMVectorSetY(c - b, 0.f));

        float dot = XMVectorGetX(XMVector3Dot(ab, bc));

        // ★ 조건:
        // 1) 거의 일직선 (dot > cosThreshold)
        // 2) a -> c 직선이 NavMesh 위에 완전히 존재할 때만, b 삭제
        if (dot > cosThreshold && IsSegmentOnNavMesh(a, c))
        {
            // b는 스킵 (추가 안 함)
            continue;
        }

        // 그렇지 않으면 b 유지
        result.push_back(b);
    }

    // 마지막 점은 무조건 유지
    result.push_back(points.back());

    points.swap(result);
}

int CNavigation::FindCellIndexFromPos(_vector pos)
{
    int bestIndex = -1;
    float bestDist = FLT_MAX;

    for (int i = 0; i < m_Cells.size(); ++i)
    {
        // 1) 셀 중심 가져오기
        _vector c = m_Cells[i]->Get_Center();

        // 2) Y는 무시(XZ 거리만 비교)
        _vector c2 = XMVectorSetY(c, XMVectorGetY(pos));
        float dist = XMVectorGetX(XMVector3Length(c2 - pos));

        // 3) 가장 가까운 셀 찾기
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

    vector<_float3> shared;

    for (int i = 0; i < 3; i++)
    {
        const _float3& p1 = A->Get_PointValue((CCell::POINT)i);

        for (int j = 0; j < 3; j++)
        {
            const _float3& p2 = B->Get_PointValue((CCell::POINT)j);

            // 좌표 비교는 float 오차 존재 → Equal 제거 (큰 문제 원인)
            if (XMVector3NearEqual(XMLoadFloat3(&p1), XMLoadFloat3(&p2), XMVectorSplatEpsilon()))
                shared.push_back(p1);
        }
    }

    if (shared.size() != 2)
        return false;

    // 좌우 정렬
    XMVECTOR p0 = XMLoadFloat3(&shared[0]);
    XMVECTOR p1 = XMLoadFloat3(&shared[1]);

    // A에서 B 방향
    XMVECTOR dir = XMVector3Normalize(m_Cells[cellB]->Get_Center() - m_Cells[cellA]->Get_Center());

    XMVECTOR right = XMVector3Cross(XMVectorSet(0, 1, 0, 0), dir);
    XMVECTOR toP0 = XMVector3Dot(right, p0 - m_Cells[cellA]->Get_Center());
    XMVECTOR toP1 = XMVector3Dot(right, p1 - m_Cells[cellA]->Get_Center());

    if (XMVectorGetX(toP0) < XMVectorGetX(toP1))
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
    int dummyNeighbor = -1;
    _float3 dummyNormal;

    for (auto& cell : m_Cells)
    {
        if (cell->isIn(pos, &dummyNeighbor, &dummyNormal))
            return true;
    }
    return false;
}

bool CNavigation::IsSegmentOnNavMesh(_vector a, _vector b)
{
    // 높이는 크게 중요하지 않지만, 혹시 몰라서 XZ 기준 거리만 사용
    _vector aXZ = XMVectorSetY(a, 0.f);
    _vector bXZ = XMVectorSetY(b, 0.f);

    float len = XMVectorGetX(XMVector3Length(bXZ - aXZ));

    // 거리 기반으로 샘플 수 결정 (약 0.25 단위당 1샘플)
    int SAMPLE_COUNT = (int)(len * 4.f);
    if (SAMPLE_COUNT < 8)   SAMPLE_COUNT = 8;    // 최소 8
    if (SAMPLE_COUNT > 256) SAMPLE_COUNT = 256;  // 과도한 연산 방지

    for (int i = 1; i <= SAMPLE_COUNT; ++i)
    {
        float t = (float)i / (float)SAMPLE_COUNT;
        _vector p = XMVectorLerp(a, b, t);

        if (!IsPointOnNavMesh(p))
            return false;
    }

    return true;
}

void CNavigation::SmoothPathPoints(vector<_vector>& points)
{
    if (points.size() <= 2)
        return;

    vector<_vector> result;
    result.reserve(points.size());

    size_t i = 0;
    result.push_back(points[0]); // 시작점

    const size_t MAX_SKIP = 6;   // 한 번에 최대 6포인트까지만 건너뛰기 (상황보고 4~8 사이 튜닝)

    while (i < points.size() - 1)
    {
        size_t furthest = i + 1;

        size_t maxJ = min(points.size() - 1, i + MAX_SKIP);

        // i -> j 직선으로 갈 수 있는 가장 먼 j (단, MAX_SKIP 안에서만)
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
    struct Cand { float dist; int idx; };
    vector<Cand> candidates;
    candidates.reserve(m_Cells.size());

    // 1) endPos와의 거리 기준으로 후보 정렬
    for (int i = 0; i < (int)m_Cells.size(); ++i)
    {
        _vector c = m_Cells[i]->Get_Center();
        _vector c2 = XMVectorSetY(c, XMVectorGetY(endPos));
        float dist = XMVectorGetX(XMVector3Length(c2 - endPos));
        candidates.push_back({ dist, i });
    }

    sort(candidates.begin(), candidates.end(),
        [](const Cand& a, const Cand& b) { return a.dist < b.dist; });

    // 2) 가장 가까운 셀부터, "실제로 startCell에서 갈 수 있는지" 검사
    vector<int> tempPath;

    for (auto& c : candidates)
    {
        tempPath.clear();

        if (FindPath_NoGoalAdjust(startCell, c.idx, tempPath))
            return c.idx;   // 여기까지 갈 수 있으면 OK
    }

    // 3) 아무 셀도 도달 불가 → 완전 다른 섬 (진짜로 길 없음)
    return -1;
}

bool CNavigation::FindPath_NoGoalAdjust(int startCell, int goalCell, vector<int>& outPath)
{
    outPath.clear();

    priority_queue<NAV_NODE, vector<NAV_NODE>, CompareNode> open;
    vector<bool> closed(m_Cells.size(), false);

    vector<int> parent(m_Cells.size(), -1);
    vector<float> gCost(m_Cells.size(), FLT_MAX);

    NAV_NODE st;
    st.cellIndex = startCell;
    st.gCost = 0;
    st.hCost = 0;
    st.fCost = 0;
    st.parent = -1;

    open.push(st);
    gCost[startCell] = 0;

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
            reverse(outPath.begin(), outPath.end());
            return true;
        }

        CCell* cell = m_Cells[cur.cellIndex];

        for (int i = 0; i < 3; i++)
        {
            int n = cell->m_iNeighborIndices[i];
            if (n == -1)
                continue;

            float move = XMVectorGetX(XMVector3Length(
                cell->Get_Center() - m_Cells[n]->Get_Center()));

            float g = cur.gCost + move;

            if (g < gCost[n])
            {
                gCost[n] = g;
                parent[n] = cur.cellIndex;

                NAV_NODE next;
                next.cellIndex = n;
                next.gCost = g;
                next.hCost = 0;
                next.fCost = g;
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
