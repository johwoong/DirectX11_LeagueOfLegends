#pragma once
#include "Actor.h"
#include "GameInstance.h"

NS_BEGIN(Engine)
class CModel;
class CTexture;
class CNavigation;
NS_END

NS_BEGIN(Client)


typedef struct tagMinionSpawnDesc
{
    TEAMTYPE    eTeam = TEAM_BLUE;      // 블루 / 레드
    LANE_TYPE   eLane = LANE_MID;       // TOP / MID / BOT
    MINION_TYPE eMinionType = MINION_MELEE;   // 근접 / 캐스터 / 대포
    _int        iLaneIndex = 0;         // 웨이브 안에서 몇 번째인지
    _int        iWaveIndex = 0;         // 몇 번째 웨이브인지

    _float3     vSpawnPos = _float3(0.f, 0.f, 0.f); // 실제 스폰 좌표
    _float3     vTargetPos = _float3(0.f, 0.f, 0.f); // ★ 목표 지점 (lane 끝, 넥서스 근처 등)
} MINION_SPAWNDESC;

class CMinion abstract: public CActor 
{
public:
    typedef struct tagMininonStats
    {
        NONANIMDESC ModelDesc;
        _float  m_fMaxHp = 100.f;
        _float  m_fCurrentHp = 100.f;
        _float  m_fAttackPower = 10.f;
        _float  m_fWalkSpeed = 3.f;

        // 롤 느낌 추가
        _float  m_fAttackRange = 2.0f;   // 밀리언 짧게, 캐스터 길게
        _float  m_fAttackSpeed = 1.0f;   // 초당 1타 (AS)
        _float  m_fAggroRadius = 6.0f;   // 시야/어그로 범위
    }STATS;


    enum MINION_STATE
    {
        STATE_IDLE,
        STATE_MOVE,   
        STATE_ATTACK,    
        STATE_CHASE,     
        STATE_DIE,
        STATE_END
    };

public:
    CModel* Get_Model() { return m_pModelCom; }
    CNavigation* Get_Navigation() { return m_pNavigationCom; }  // MoveState에서 쓰려고
protected:
    CMinion(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CMinion(const CMinion& rhs);
    virtual ~CMinion() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual int    Update(_float fTimeDelta) override;
    virtual void   Late_Update(_float fTimeDelta) override;
    virtual HRESULT Render() override;
    virtual HRESULT Render_ShadowDepth() override;
    virtual HRESULT Render_EdgeDetection() override;

public:
    // 간단한 Getter/Setter들
    TEAMTYPE    Get_Team()   const { return m_eTeam; }
    LANE_TYPE   Get_Lane()   const { return m_eLane; }

    _int  Get_LaneIndex() const { return m_iLaneIndex; }   // ★ 추가
    MINION_TYPE Get_MinType()const { return m_eMinionType; }
    const STATS& Get_Stats() const { return m_tStats; }
public:
    _int  Get_AttackIndex() const { return m_iAttackIndex; }
    void  Set_AttackIndex(_int iIdx) { m_iAttackIndex = iIdx; }
public:
    const _float3& Get_TargetPos() const { return m_vTargetPos; }
    CActor* Get_Target() const { return m_pTarget; }
    void        Set_LaneInfo(TEAMTYPE eTeam, LANE_TYPE eLane, MINION_TYPE eType, _int iLaneIndex)
    {
        m_eTeam = eTeam;
        m_eLane = eLane;
        m_eMinionType = eType;
        m_iLaneIndex = iLaneIndex;
    }
    _float Check_CullingRadius();
public:
    void  Set_LaneNodeIndex(_int idx) { m_iLaneNodeIndex = idx; }
    _int  Get_LaneNodeIndex() const { return m_iLaneNodeIndex; }
    _uint GetIdleAnimIndex() const { return m_iIdleAnimIndex; }
    _uint GetRunAnimIndex()  const { return m_iRunAnimIndex; }
    // ★ AttackState 에서 쓰기 위한 함수들
    _float Get_AttackTimer()     const { return m_fAttackTimer; }
    _float Get_AttackCoolTime()  const { return m_fAttackCoolTime; }
    void   Reset_AttackTimer() { m_fAttackTimer = 0.f; }


    // 공격 / 어그로 범위 체크
    _bool    Is_TargetInAttackRange() const;
    _bool    Is_TargetInAggroRange() const;

public:
    virtual void   AI_Behaviour(_float fTimeDelta) {};
public:
    virtual void   Find_Target();                      // 어그로 타겟 찾기
    virtual CActor* Find_MinDistance_Target();         // 가장 가까운 적
    virtual CActor* Check_FiledTarget();               // 현재 라인/필드 기준 타겟
    virtual _float  Target_Distance(CActor* pTarget) const;   // ★ const 로 
    void Compute_LaneOffset();

    virtual void   Follow_Target(_float fTimeDelta);
    // ChaseState 에서 사용할 이동
public:
    virtual void ReUse_Setting(void* pArg) override;  // ★ 추가
    virtual void   Attack_Target(_float fTimeDelta) {}; // 기본 공격 처리(가상)
    virtual void   On_Death() {};

    virtual _int Take_Damage(int fDamage, CActor* DamageCauser);
    void           Compute_CurrentIndex(LEVEL eLevel);
    void Mark_Dead() { m_bDead = true; } // ★ 추가
protected:
    virtual HRESULT SetUp_ShaderResources();
    virtual HRESULT SetUp_ShaderID();
    virtual void    Compute_CurrentIndex();
protected:
    virtual HRESULT Ready_Components(void* pArg = nullptr) = 0;
public:
    virtual _bool Is_AnimationLoop(_uint eAnimId) = 0;
    virtual void    Check_Navigation() = 0;

protected:
    // ====== 기본 스탯 / 상태 ======
    STATS           m_tStats;
    MINION_STATE    m_eState = STATE_IDLE;
    MINION_TYPE     m_eMinionType = MINION_MELEE;
    TEAMTYPE        m_eTeam = TEAM_BLUE;
    LANE_TYPE       m_eLane = LANE_MID;

    _float          m_fAttackRadius = 0.5f;  // 충돌용 반지름 (HitSphere)
    _float          m_fAnimSpeed = 1.f;
    _bool           m_bMove = true;  // 스턴/루트 등 걸렸을 때 false
    _int  m_iAttackIndex = 0;   // 공격 콤보 인덱스
protected:
    _float3 m_vTargetPos = _float3(0.f, 0.f, 0.f);

    // ====== 공격 쿨타임 / 타겟팅 ======
    _float          m_fAttackCoolTime = 0.f; // 1 / AttackSpeed
    _float          m_fAttackTimer = 0.f; // 경과 시간

    // 우선 목표 (미니언 > 타워 > 챔프 이런 식)
    CActor* m_pTarget = nullptr;
    CActor* m_pOrginDamageCauser = nullptr;   // 마지막으로 때린 놈(어그로 변경시 사용)


    _int            m_iWaveIndex = 0;   // n번째 웨이브
    _int            m_iLaneIndex = 0;   // 웨이브 안에서 위치 인덱스 (중앙/왼/오 등)

protected:
    _int m_iLaneNodeIndex = -1;   // 현재 따라가는 웨이포인트 인덱스
    _float m_fLaneOffset = 0.f;     // 0 = 중앙, + = 오른쪽, - = 왼쪽
protected:
    _uint m_iIdleAnimIndex = 0;   // Idle 애니 인덱스
    _uint m_iRunAnimIndex = 0;   // Run 애니 인덱스

protected: /* For.Components */
    CModel* m_pModelCom = nullptr;
    CNavigation* m_pNavigationCom = nullptr;

    // 보상
    _int            m_iGoldReward = 14;   // 롤 근접 미니언 기준
    _int            m_iExpReward = 58;


public:
    virtual void Free() override;
};
NS_END

