#pragma once
#include "Actor.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CModel;
NS_END

NS_BEGIN(Client)
class CBuilding_HpBar;   // ★ HpBar forward 선언 추가
class CBuilding abstract : public CActor
{
public:
	enum TEAMTYPE { BLUE, RED };
public:
	typedef struct BuildgTag
	{
		NONANIMDESC ModelDesc;
		TEAMTYPE		eTeamType;
	}BUILDINGDESC;

	typedef struct BuildInfo
	{
		_float  m_fMaxHp = 100.f;
		_float  m_fCurrentHp = 100.f;
		_float  m_fAttackPower = 10.f;
		_float  m_fAttackRange = 2.0f;
		_float  m_fAttackSpeed = 1.0f;
	}STAT;

	struct HPBARDESC
	{
		float fWidth = 10.f;  // 기본 폭
		float fHeight = 0.6f;  // 기본 높이
		float fOffsetY = 5.f;   // 월드 Y 오프셋
	};

protected:
	CBuilding(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CBuilding(const CBuilding& rhs);
	virtual ~CBuilding() = default;

public:
	STAT GetInfo() { return m_Stat; }
	TEAMTYPE Get_TeamType() const { return m_BuildDesc.eTeamType; }
	virtual HPBARDESC Get_HpBarDesc() const
	{
		HPBARDESC desc;
		// 여기 값이 "디폴트" (터렛/억제기가 이걸 그대로 써도 됨)
		desc.fWidth = 10.f;
		desc.fHeight = 0.6f;
		desc.fOffsetY = 5.f;
		return desc;
	}

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual int Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);
	virtual HRESULT Render();
	virtual HRESULT Render_ShadowDepth() override;
	virtual HRESULT Render_EdgeDetection() override;


public:
	virtual _bool Picking(_float3* PickingPoint);
	virtual void PickingTrue();
	virtual void Set_Picked();

public: // Damage
	virtual _int Take_Damage(int iDamage, CActor* DamageCauser) override;
	void Mark_Dead();

protected:
	CModel* m_pModelCom = nullptr;
	BUILDINGDESC	m_BuildDesc;
	STAT m_Stat;
protected:
	CBuilding_HpBar* m_pHpBar = nullptr;
protected:
	virtual HRESULT Ready_Components(void* pArg) PURE;
	virtual HRESULT SetUp_ShaderResources();
public:
	virtual void  Free() override;
};
NS_END

