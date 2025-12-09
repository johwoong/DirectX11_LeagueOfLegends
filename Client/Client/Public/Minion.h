#pragma once
#include "Actor.h"
#include "GameInstance.h"

NS_BEGIN(Engine)
class CModel;
class CTexture;
class CNavigation;
class CColider;
NS_END

NS_BEGIN(Client)

// 미니언 스폰 설정
typedef struct tagMinionSpawnDesc
{
    TEAMTYPE    eTeam = TEAM_BLUE;
    LANE_TYPE   eLane = LANE_MID;
    MINION_TYPE eMinionType = MINION_MELEE;

    _int        iLaneIndex = 0;
    _int        iWaveIndex = 0;

    _float3     vSpawnPos = _float3(0.f, 0.f, 0.f);
    _float3     vTargetPos = _float3(0.f, 0.f, 0.f);

} MINION_SPAWNDESC;

class CMinion abstract : public CActor
{
public:
    typedef struct tagMinionStats
    {
        NONANIMDESC ModelDesc;

        _float  m_fMaxHp = 100.f;
        _float  m_fCurrentHp = 100.f;
        _float  m_fAttackPower = 10.f;
        _float  m_fWalkSpeed = 3.f;

        _float  m_fAttackRange = 2.0f;
        _float  m_fAttackSpeed = 1.0f;
        _float  m_fAggroRadius = 6.0f;

    } STATS;

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
    // 기본 Getter
    CModel* Get_Model() { return m_pModelCom; }
    CNavigation* Get_Navigation() { return m_pNavigationCom; }

    TEAMTYPE        Get_Team()      const { return m_eTeam; }
    LANE_TYPE       Get_Lane()      const { return m_eLane; }
    MINION_TYPE     Get_MinType()   const { return m_eMinionType; }

    _int            Get_LaneIndex() const { return m_iLaneIndex; }
    _int            Get_AttackIndex() const { return m_iAttackIndex; }

    const STATS& Get_Stats() const { return m_tStats; }
    const _float3& Get_TargetPos() const { return m_vTargetPos; }
    CActor* Get_Target() const { return m_pTarget; }

    // Setter
    void Set_AttackIndex(_int idx) { m_iAttackIndex = idx; }
    void Set_LaneInfo(TEAMTYPE team, LANE_TYPE lane, MINION_TYPE type, _int idx)
    {
        m_eTeam = team; m_eLane = lane; m_eMinionType = type; m_iLaneIndex = idx;
    }

    // Lane path control
    void  Set_LaneNodeIndex(_int idx) { m_iLaneNodeIndex = idx; }
    _int  Get_LaneNodeIndex() const { return m_iLaneNodeIndex; }

    _uint GetIdleAnimIndex() const { return m_iIdleAnimIndex; }
    _uint GetRunAnimIndex()  const { return m_iRunAnimIndex; }

    // Attack control
    _float Get_AttackTimer()    const { return m_fAttackTimer; }
    _float Get_AttackCoolTime() const { return m_fAttackCoolTime; }
    void   Reset_AttackTimer() { m_fAttackTimer = 0.f; }
    float Get_DesiredAttackDist(CActor* pTarget) const;

public:
    void Set_HpBar(class CMinion_HpBar* pBar) { m_pHpBar = pBar; }
    class CMinion_HpBar* Get_HpBar() const { return m_pHpBar; }

    _bool Is_Dying() const { return m_bDying; }
    _bool IsDeadOrDying() const
    {
        return m_bDead || m_bDying || m_tStats.m_fCurrentHp <= 0.f || m_eState == STATE_DIE;
    }

public: // Target check
    _bool Is_TargetInAttackRange();      
    _bool Is_TargetInAggroRange();     
    _bool Is_TargetStillExist(CActor * pTarget) const;

public: // AI
    virtual void AI_Behaviour(_float fTimeDelta) {}
    virtual void Find_Target(_float fTimeDelta);
    virtual CActor* Find_MinDistance_Target();
    virtual CActor* Check_FiledTarget();
    virtual _float Target_Distance(CActor* pTarget) const;
    void Compute_LaneOffset();

    virtual void Follow_Target(_float fTimeDelta);
    virtual void Attack_Target(_float fTimeDelta) {}
    virtual void On_Death() {}
    void Apply_Separation(_float fTimeDelta);
public: // Navigation logic
    virtual void Check_Navigation() = 0;
    virtual void Compute_CurrentIndex(LEVEL eLevel);
    void Compute_CurrentIndex();

public: // Pool reuse
    virtual void ReUse_Setting(void* pArg) override;

public: // Damage
    virtual _int Take_Damage(int iDamage, CActor* DamageCauser) override;
    void Mark_Dead();

public:
    virtual _bool Picking(_float3* PickingPoint);
    virtual void PickingTrue();
    virtual void Set_Picked();

public:
    void Create_HpBar();
    void Face_Target();
    _int Find_ForwardLaneNodeIndex();
protected:
    virtual HRESULT SetUp_ShaderResources();
    virtual HRESULT SetUp_ShaderID();
    virtual HRESULT Ready_Components(void* pArg = nullptr) = 0;

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
    virtual _bool Is_AnimationLoop(_uint eAnim) = 0;
    virtual _float Check_CullingRadius();
public:
        virtual void Ready_Animation_Event() PURE;

protected:
    // Base info
    STATS           m_tStats;
    MINION_STATE    m_eState = STATE_IDLE;

    TEAMTYPE        m_eTeam = TEAM_BLUE;
    LANE_TYPE       m_eLane = LANE_MID;
    MINION_TYPE     m_eMinionType = MINION_MELEE;

    // Target and attack
    _float          m_fAttackTimer = 0.f;
    _float          m_fAttackCoolTime = 0.f;
    _int            m_iAttackIndex = 0;
    _float3         m_vTargetPos = _float3(0, 0, 0);

    CActor* m_pTarget = nullptr;
    CActor* m_pOrginDamageCauser = nullptr;

    // Lane/Path
    _int            m_iLaneIndex = 0;
    _int            m_iWaveIndex = 0;
    _int            m_iLaneNodeIndex = -1;
    _float          m_fLaneOffset = 0.f;

    // Anim indexes
    _uint           m_iIdleAnimIndex = 0;
    _uint           m_iRunAnimIndex = 0;

    // State flags
    _bool           m_bMove = true;
    _bool           m_bDying = false;

    _vector m_vLastSeparation = {};
    _vector m_vMoveDir = XMVectorZero();
    _vector m_vAvoidDir = XMVectorZero();
    _vector m_vLastAvoidDir = XMVectorZero();

protected: // Components
    CModel* m_pModelCom = nullptr;
    CNavigation* m_pNavigationCom = nullptr;

protected:
    class CMinion_HpBar* m_pHpBar = nullptr;
    // Rewards
    _int            m_iGoldReward = 14;
    _int            m_iExpReward = 58;

public:
    virtual void Free() override;
};

NS_END


