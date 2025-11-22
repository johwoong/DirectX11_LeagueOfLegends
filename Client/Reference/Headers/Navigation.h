#pragma once
#include "Component.h"

NS_BEGIN(Engine)

class ENGINE_DLL CNavigation final : public CComponent
{
public:
	typedef struct tagNavigationDesc
	{
		_int	iCurrentCellIndex = -1; 
	}NAVDESC;

	struct NAV_NODE
	{
		int cellIndex;
		float gCost;  // 시작점부터 현재 Cell까지 비용
		float hCost;  // 휴리스틱 (목표까지 직선 거리)
		float fCost;  // g + h
		int parent;
	};

	struct CompareNode {
		bool operator()(const NAV_NODE& a, const NAV_NODE& b) const {
			return a.fCost > b.fCost;
		}
	};

private:
	CNavigation(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CNavigation(const CNavigation& rhs);
	virtual ~CNavigation() = default;

public:
	virtual HRESULT Initialize_Prototype(const _tchar* pNavigationData);
	virtual HRESULT Initialize(void* pArg);

public:
	_bool isMove(_fvector vPosition);
	_float Compute_Height(_vector vPosition, _float foffset);
	void Compute_CurrentIndex_byXZ(_vector vPosition);

public: // Getter Setter
	_int	Get_CurrentCellIndex() { return m_NaviDesc.iCurrentCellIndex; }
	_uint	Get_CurrentCellType();
	_vector Get_CurrentCellCenter();
	_vector Get_LastNormal() { return XMLoadFloat3(&m_vLastNormal); }
	

public:
	bool FindPath(_vector startPos, _vector endPos, vector<int>& outCellPath);
	bool BuildFunnelPath(const vector<int>& cellPath, _vector startPos, _vector endPos, vector<_vector>& outPoints);
public:
	// 거의 일직선인 경유지는 제거해서 지그재그 줄이는 함수
	void SimplifyLinearPoints(vector<_vector>& points,
		float cosThreshold = cosf(XMConvertToRadians(10.f)));
public:
	// 경로 스무딩용
	bool IsPointOnNavMesh(_vector pos);
	bool IsSegmentOnNavMesh(_vector a, _vector b);
	void SmoothPathPoints(vector<_vector>& points);
private:
	int FindCellIndexFromPos(_vector pos);
	bool GetPortal(int cellA, int cellB, _float3& outLeft, _float3& outRight);
	float TriangleArea2(_vector a, _vector b, _vector c);
	int FindNearestReachableCell(_vector endPos, int startCell);
	bool FindPath_NoGoalAdjust(int startCell, int goalCell, vector<int>& outPath);

#ifdef _DEBUG
public:
	HRESULT Render();
#endif // _DEBUG

private:
	NAVDESC						m_NaviDesc;
	vector<class CCell*>		m_Cells;
	_float3						m_vLastNormal = _float3(0.f, 0.f, 0.f);\


	// ★ A*가 마지막에 사용한 목표 셀, 그리고 목표 조정 여부
	int                         m_iLastGoalCell = -1;
	bool                        m_bAdjustedGoal = false;

#ifdef _DEBUG
private:
	class CShader* m_pShader = nullptr;
#endif // _DEBUG

private:
	friend class CTransform;


private:
	HRESULT SetUp_Neighbor();

public:
	static CNavigation* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _tchar* pNavigationData);
	virtual CComponent* Clone(void* pArg = nullptr) override;
	virtual void Free() override;
};
NS_END
