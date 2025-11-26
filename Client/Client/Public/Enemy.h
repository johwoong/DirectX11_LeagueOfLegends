#pragma once
#include "Actor.h"
#include "GameInstance.h"
#include "Cell.h"


NS_BEGIN(Engine)
class CModel;
class CTexture;
class CNavigation;
NS_END

NS_BEGIN(Client)
class CEnemy abstract : public CActor
{
public:
	typedef struct ChampionStat
	{
		wstring Name; // 챔피언 이름
		int iLevel; // 현재 레벨

		// 기본 능력치
		float Hp; // 체력
		float HpPerLevel; // 레벨당 체력 증가량

		float Mp; // 마나
		float MpPerLevel; // 레벨당 마나 증가량

		float MoveSpeed; // 이동속도
		float AttackRange; // 공격 사거리

		// ---------- 공격 ------------
		float AttackDamage; // 기본공격력
		float AttackDamagePerLevel; // 레벨당 공격력 증가량

		float AttackSpeed; // 기본 공격속도
		float AttackSpeedPerLevel;

		float Critical;	// 치명타 확률
		float CritDamage; // 치명타 피해량

		// -----------방어 ------------
		float Armor; // 방어력
		float ArmorPerLevel;

		// ------- 회복 관련 ---------
		float HPRegen; // 초당 체력 재생
		float HpRegenPerLevel; // 초당 체력 재생 증가량

		float MPRegen; // 초당 마나 재생
		float MpRegenPerLevel; // 초당 마나 재생 증가량
	}STATS;


protected:
	CEnemy(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CEnemy(const CEnemy& rhs);
	virtual ~CEnemy() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;
	virtual int Update(_float fTimeDelta) override;
	virtual void Late_Update(_float fTimeDelta) override;
	virtual HRESULT Render() override;
	virtual HRESULT Render_ShadowDepth() override;
	virtual HRESULT Render_EdgeDetection() override;


public:
	virtual _bool Picking(_float3* PickingPoint);
	virtual void PickingTrue();


public:
	ENEMY_TYPE Get_EnemyType(void) { return m_eEnemyType; }
	STATS Get_Stats() { return m_tStats; }
	CNavigation* Get_Navigation(void) { return m_pNavigationCom; }

public:
	virtual void AI_Behaviour(_float fTimeDelta) {};
	virtual void Find_Target();
	virtual CActor* Find_MinDistance_Target();
	virtual CActor* Check_FiledTarget();

public:
	virtual void Compute_CurrentIndex();

protected:
	virtual HRESULT SetUp_ShaderResources();
	virtual HRESULT SetUp_ShaderID();



protected:
	virtual HRESULT Ready_Components(void* pArg = nullptr)	PURE;
	virtual _bool Is_AnimationLoop(_uint eAnimId)			PURE;
	virtual void Check_Navigation()							PURE;

protected:
	STATS m_tStats;
	ENEMY_TYPE	m_eEnemyType = ENEMY_END;
	_float m_fTimeDeltaAcc = 0.f;
protected: /* For.Components */
	CModel* m_pModelCom = nullptr;
	CNavigation* m_pNavigationCom = nullptr;
	CActor* m_pTarget = nullptr;


public:
	virtual void Free() override;

};
NS_END

