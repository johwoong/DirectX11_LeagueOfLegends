#pragma once
#include "Component.h"

NS_BEGIN(Engine)
class CTransform;
class ENGINE_DLL CColider final : public CComponent
{
public:
	enum TYPE {TYPE_AABB, TYPE_OBB, TYPE_SPHERE, TYPE_END};
	enum BOUNDING {BOUNDING_ORIGINAL, BOUNDING_WORLD, BOUNDING_END};

public:
	// 콜라이더 초기 상태
	typedef struct tagColiderDesc
	{
		_float3 vScale;
		_float3 vRotation;
		_float3 vPosition;
	}COLLIDERDESC;

	typedef struct tagOBB
	{
		_float3 vCenter;
		_float3 vAliginAxis[3];
		_float3 vCenterAxis[3];
	}OBBDESC;

private:
	CColider(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CColider(const CColider& rhs);
	virtual ~CColider() = default;

public:
	virtual HRESULT Initialize_Prototype(TYPE eType);
	virtual HRESULT Initialize(void* pArg);

public:
	void Update(_fmatrix WorldMatrix);
	_float Get_SphereRadius(void) { return 1.f; }
	HRESULT Set_Colider_Desc(void* pArg);
	void Set_Sphere_Radius(float radius);
	void Set_Scale(_float3 vScale);
#ifdef _DEBUG
public:
	HRESULT Render();
#endif

public:
	_bool Collision(class CColider* pTargetColider);
	_bool Collision_AABB(class CColider* pTargetColider);
	_bool Collision_OBB(class CColider* pTargetColider);
	bool RayPick(CTransform* pTransform,_float3* pOutHitPos = nullptr, float* pOutDist = nullptr);
private:
	TYPE	m_eType = TYPE_END;
	COLLIDERDESC m_ColiderDesc;

	BoundingBox* m_pAABB[BOUNDING_END] = { nullptr };
	BoundingOrientedBox* m_pOBB[BOUNDING_END] = { nullptr };
	BoundingSphere* m_pSphere[BOUNDING_END] = { nullptr };

	BoundingBox* m_pOriAABB[BOUNDING_END] = { nullptr };
	BoundingOrientedBox* m_pOriOBB[BOUNDING_END] = { nullptr };
	BoundingSphere* m_pOriSphere[BOUNDING_END] = { nullptr };

	_bool m_isCollision = false;

#ifdef _DEBUG
private:
	ID3D11InputLayout*								m_pInputLayout = { nullptr };
	PrimitiveBatch<VertexPositionColor>*			m_pBatch = { nullptr };
	BasicEffect*									m_pEffect = { nullptr };
	ID3D11DepthStencilState*						m_pDepthStencilState = { nullptr };
#endif

private:
	_matrix Remove_Rotation(_fmatrix Matrix);
	_float3 Compute_Min();
	_float3 Compute_Max();

	OBBDESC Compute_OBBDesc();
public:
	static CColider* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, TYPE eType);
	virtual CComponent* Clone(void* pArg = nullptr) override;
	virtual void Free() override;


};
NS_END

