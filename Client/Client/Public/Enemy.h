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
class CMinion;
class CBuilding;
class CEnemy abstract : public CActor
{
public:
	typedef struct ChampionStat
	{
		wstring Name; // 챔피언 이름
		int iLevel; // 현재 레벨

		// 기본 능력치
		float Hp;        // 현재 HP
		float MaxHp;     // 최대 HP  ← 추가
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

	struct AGGRO_DESC
	{
		float fAttackRange;   // 실제 공격 사거리 (보통 Stats.AttackRange)
		float fChaseRange;    // 추적 허용 최대 거리
		float fGiveUpRange;   // 이 이상이면 타겟 완전 포기
	};

	// ?? Enemy 공통 라인 타입
	enum LINE_TYPE
	{
		LINE_TOP,
		LINE_MID,
		LINE_BOTTOM,
	};

	struct AISKILLINFO
	{
		float fCooldown = 0.f;   // 쿨타임
		float fTimer = 0.f;   // 경과 시간
		float fRange = 0.f;   // 사용 가능 사거리
		float fUseProb = 0.f;   // 사용 확률 (0.0 ~ 1.0)
	};

	enum AISKILLTYPE
	{
		SKILL_NONE = 0,
		SKILL_Q,
		SKILL_W,
		SKILL_E,
		SKILL_R,
		SKILL_END
	};


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
	virtual void Set_Picked();


public:
	ENEMY_TYPE Get_EnemyType(void) { return m_eEnemyType; }
	STATS Get_Stats() { return m_tStats; }
	CNavigation* Get_Navigation(void) { return m_pNavigationCom; }
	void Set_HpBar(class CEnemy_HpBar* pBar) { m_pHpBar = pBar; }
	class CEnemy_HpBar* Get_HpBar() { return m_pHpBar; }
	// ?? 타겟 Get/Set
	CActor* Get_Target() const { return m_pTarget; }
	void    Set_Target(CActor* pT) { m_pTarget = pT; }
	float Get_HpPercent() const
	{
		if (m_tStats.MaxHp <= 0.f)
			return 0.f;
		return m_tStats.Hp / m_tStats.MaxHp;
	}
	const AGGRO_DESC& GetAggroDesc() const { return m_tAggro; }
	// ?? XZ 거리 계산 (Minion이랑 비슷)
	_float  Target_Distance(CActor* pTarget) const;

	// ?? 공격 사거리 (필요하면 월드 스케일 반영해서 튜닝)
	_float  Get_AttackRangeWorld() const { return m_tStats.AttackRange; }
	const AISKILLINFO& GetSkill(AISKILLTYPE eType) const
	{
		static AISKILLINFO dummy{};
		if (eType <= SKILL_NONE || eType >= SKILL_END)
			return dummy;
		return m_tSkill[eType];
	}

	AISKILLINFO& GetSkillRef(AISKILLTYPE eType)
	{
		static AISKILLINFO dummy{};
		if (eType <= SKILL_NONE || eType >= SKILL_END)
			return dummy;
		return m_tSkill[eType];
	}
public:
	virtual void AI_Behaviour(_float fTimeDelta) {};
	virtual void Find_Target();
	virtual CActor* Find_MinDistance_Target();
	virtual CActor* Check_FiledTarget();
	void Follow_Lane(_float fTimeDelta);

public:
	void         Tick_SkillTimers(float fDeltaTime);
	bool         CanUseSkill(AISKILLTYPE eType, float fDist) const;
	void         ConsumeSkill(AISKILLTYPE eType);
	AISKILLTYPE  DecideRandomSkill(float fDistToTarget);

	AISKILLTYPE  Get_NextSkill() const { return m_eNextSkill; }
	void         Clear_NextSkill() { m_eNextSkill = SKILL_NONE; }

public:
	virtual void Compute_CurrentIndex();
	static _float3 Get_EnemyLaneDest(LINE_TYPE eLine);
	static const vector<_float3>& Get_EnemyLanePath(LINE_TYPE eLine);
	void RefreshLaneNodeByPosition();
	bool Has_AllyWaveNearby(float fRadius = 15.f) const;
	CMinion* Get_FrontAllyMinion(float fMaxDistance = 200.f) const;
	bool Has_AllyMinionUnderEnemyTower(CBuilding* pTower, float fRadius = 8.f) const;
	bool Has_AllyMinionNearBuilding(CBuilding* pBuilding, float fRadius);
	bool    HasReachedFirstTower() const { return m_bReachedFirstTower; }
	void    SetReachedFirstTower(bool b) { m_bReachedFirstTower = b; }
	_float3 Get_FirstTowerWaitPos() const;

protected:
	virtual HRESULT SetUp_ShaderResources();
	virtual HRESULT SetUp_ShaderID();
public: // Damage
	virtual _int Take_Damage(int iDamage, CActor* DamageCauser) override;
	void Mark_Dead();

protected:
	virtual HRESULT Ready_Components(void* pArg = nullptr)	PURE;
	virtual _bool Is_AnimationLoop(_uint eAnimId)			PURE;
	virtual void Check_Navigation()							PURE;
	virtual void Ready_Animation_Event() PURE;
protected:
	STATS m_tStats;
	ENEMY_TYPE	m_eEnemyType = ENEMY_END;
	LINE_TYPE m_eLineType = {};
	_float m_fTimeDeltaAcc = 0.f;
	CEnemy_HpBar* m_pHpBar = nullptr; // Hp Bar
protected: /* For.Components */
	CModel* m_pModelCom = nullptr;
	CNavigation* m_pNavigationCom = nullptr;
	CActor* m_pTarget = nullptr;


protected:
	// 어그로 관련
	float m_fAggroBonus = {};
	float m_fAggroReleaseMul = {};
	_float3 m_vLaneTarget = { 0.f, 0.f, 0.f };
	_int    m_iLaneNodeIndex = 0;

	// ★ 스킬 AI 공통
	AISKILLINFO m_tSkill[SKILL_END];      // [SKILL_Q..SKILL_R]
	AISKILLTYPE m_eNextSkill = SKILL_NONE;
	bool m_bWasChasing = false;   // 직전에 타겟을 쫓고 있었는지 여부
	bool    m_bReachedFirstTower = false;   // ← 추가: 미드 1차 포탑(8번째 노드)까지 갔는지
	AGGRO_DESC m_tAggro;
public:
	virtual void Free() override;

};
NS_END

