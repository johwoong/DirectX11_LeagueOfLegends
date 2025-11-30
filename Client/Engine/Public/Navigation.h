#pragma once
#include "Component.h"

NS_BEGIN(Engine)
class CCell;
class CShader;
class ENGINE_DLL CNavigation final : public CComponent
{
public:
    struct NAVDESC
    {
        _int iCurrentCellIndex = -1;
    };

    struct NAV_NODE
    {
        int   cellIndex = -1;
        float gCost = 0.f; // 시작점부터 현재 Cell까지 비용
        float hCost = 0.f; // 휴리스틱 (목표까지 직선 거리)
        float fCost = 0.f; // g + h
        int   parent = -1;
    };

    struct CompareNode
    {
        bool operator()(const NAV_NODE& a, const NAV_NODE& b) const
        {
            return a.fCost > b.fCost;
        }
    };

private:
    CNavigation(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CNavigation(const CNavigation& rhs);
    virtual ~CNavigation() = default;

public:
    // CComponent
    virtual HRESULT Initialize_Prototype(const _tchar* pNavigationData);
    virtual HRESULT Initialize(void* pArg) override;

public:
    // 기본 네비게이션 기능
    _bool  isMove(_fvector vPosition);
    _float Compute_Height(_vector vPosition, _float fOffset);
    void   Compute_CurrentIndex_byXZ(_vector vPosition);

public: // Getter / Setter
    _int   Get_CurrentCellIndex() const { return m_NaviDesc.iCurrentCellIndex; }
    _uint  Get_CurrentCellType();
    _vector Get_CurrentCellCenter();
    _vector Get_LastNormal() const { return XMLoadFloat3(&m_vLastNormal); }

public:
    // A* + Funnel
    bool FindPath(_vector startPos, _vector endPos, std::vector<int>& outCellPath);
    bool BuildFunnelPath(const std::vector<int>& cellPath,
        _vector startPos,
        _vector endPos,
        std::vector<_vector>& outPoints);

    // 지그재그 줄이기
    void SimplifyLinearPoints(std::vector<_vector>& points,
        float cosThreshold = cosf(XMConvertToRadians(10.f)));

    // 경로 스무딩
    bool IsPointOnNavMesh(_vector pos);
    bool IsSegmentOnNavMesh(_vector a, _vector b);
    void SmoothPathPoints(std::vector<_vector>& points);


private:
    // 내부 헬퍼
    int   FindCellIndexFromPos(_vector pos);
    bool  GetPortal(int cellA, int cellB, _float3& outLeft, _float3& outRight);
    float TriangleArea2(_vector a, _vector b, _vector c);
    int   FindNearestReachableCell(_vector endPos, int startCell);
    bool  FindPath_NoGoalAdjust(int startCell, int goalCell, std::vector<int>& outPath);
    HRESULT SetUp_Neighbor();

#ifdef _DEBUG
public:
    HRESULT Render();
#endif

private:
    NAVDESC              m_NaviDesc{};
    std::vector<CCell*>  m_Cells;
    _float3              m_vLastNormal = _float3(0.f, 0.f, 0.f);

    // A*가 마지막에 사용한 목표 셀, 그리고 목표 조정 여부
    int   m_iLastGoalCell = -1;
    bool  m_bAdjustedGoal = false;

#ifdef _DEBUG
private:
    CShader* m_pShader = nullptr;
#endif

private:
    friend class CTransform;

public:
    static CNavigation* Create(ID3D11Device* pDevice,
        ID3D11DeviceContext* pContext,
        const _tchar* pNavigationData);
    virtual CComponent* Clone(void* pArg = nullptr) override;
    virtual void         Free() override;
};
NS_END
