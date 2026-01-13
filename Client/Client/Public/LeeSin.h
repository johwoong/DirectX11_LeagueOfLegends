#pragma once
#include "Champion.h"

NS_BEGIN(Client)
class CLeeSin final : public CChampion
{
public:
    enum ANIM {
        ATTACK_L_PUNCH, LAUGH, ATTACK_KICK, ATTACK_L_FINISH, ATTACK_R_KICK, ATTACK_R_PUCNH,
        CHANNEL, CHANNEL_WINDUP, CRIT, DANCE_IN, DANCE_LOOP, DEATH, IDLE1, IDLE_BASE, IDLE_BASE1,
        JOKE, RECALL, RESPAWN, RUN1, RUN_TOIDLE, RUN_RUNB, RUN_HOMEGUARD, RUN_B, RUN_TORUN,
        Q, Q_LONG, Q_LONGTRANS, Q_MID, Q_MIDTRANS, Q_SHORT, Q_SHORTTRANS, W, E, E_B, R_A, R_B, R_C, TAUNT, WINDDOWN, E_MOVE, ANIM_END
    };

private:
    CLeeSin(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CLeeSin(const CLeeSin& rhs);
    virtual ~CLeeSin() = default;

public:
    virtual HRESULT Initialize_Prototype();
    virtual HRESULT Initialize(void* pArg);
    virtual int Update(_float fTimeDelta) override;
    virtual void Late_Update(_float fTimeDelta) override;
public:
    virtual _bool Is_AnimationLoop(_uint eAnimId) override;
    virtual void Ready_Animation_Event() override;

    
private:
    virtual void SetSkillInfo() override;

    // =========================
    // W 실드(방호) 시스템
    // =========================
public:
    void  Apply_WShield(float fShieldHP, float fDuration);
    // W1: 자기 자신에게 실드 + 실드FX + W2 오픈 (애니 안 건드림)
    void Cast_W_Self(float shieldHP, float shieldDur, float recastWindow,
        const _tchar* pShieldFxDat);
private:
    void  Update_WShield(_float dt);
private:
    void Play_WShieldEffect(const _tchar* pDat);
    void Clear_WShieldEffect();

private:
    float m_fWShieldHP = 0.f;
    float m_fWShieldAcc = 0.f;
    float m_fWShieldDuration = 0.f;

private:
    vector<CActor*> m_vecEMarkTargets;

private:
    // 실드 고려한 데미지 처리
    virtual _int Take_Damage(int iDamage, CActor* DamageCauser) override;

    // =========================
    // W 재사용(2타)
    // =========================
private:
    bool  m_bWRecastReady = false;
    float m_fWRecastAcc = 0.f;
    float m_fWRecastWindow = 3.f;

private:
    std::vector<CEffect*> m_vecWShieldFx;
public:
    bool  Can_WRecast() const { return m_bWRecastReady; }
    void  Start_WRecast(float window = 3.f);
    void  Consume_WRecast(bool bUsed = false);
    // E2는 "재사용 타이머 ON" && "표식 대상 1명 이상"일 때만
    bool Can_ERecast() const { return m_bERecastReady && !m_vecEMarkTargets.empty(); }

    void Add_EMarkTarget(CActor* p)
    {
        if (!p) return;
        auto it = std::find(m_vecEMarkTargets.begin(), m_vecEMarkTargets.end(), p);
        if (it == m_vecEMarkTargets.end())
            m_vecEMarkTargets.push_back(p);
    }

    void Clear_EMarkTargets()
    {
        for (auto* t : m_vecEMarkTargets)
        {
            if (!t) continue;

            // 내 표식만 제거
            if (t->Has_Mark(CActor::MARK_TYPE::LEE_SIN_E) &&
                t->Get_MarkCaster(CActor::MARK_TYPE::LEE_SIN_E) == (CActor*)this)
            {
                t->Remove_Mark(CActor::MARK_TYPE::LEE_SIN_E);
            }
        }

        m_vecEMarkTargets.clear();
    }

    void Cleanup_EMarkTargets()
    {
        auto& v = m_vecEMarkTargets;

        v.erase(std::remove_if(v.begin(), v.end(),
            [this](CActor* t)
            {
                if (!t) return true;

                // 죽었으면(내 표식이면 지우고) 리스트에서 제거
                if (t->Get_Dead())
                {
                    if (t->Has_Mark(CActor::MARK_TYPE::LEE_SIN_E) &&
                        t->Get_MarkCaster(CActor::MARK_TYPE::LEE_SIN_E) == (CActor*)this)
                    {
                        t->Remove_Mark(CActor::MARK_TYPE::LEE_SIN_E);
                    }
                    return true;
                }

                // 살아있어도 "내 표식"이 아니면 리스트에서 제거
                if (!t->Has_Mark(CActor::MARK_TYPE::LEE_SIN_E)) return true;
                if (t->Get_MarkCaster(CActor::MARK_TYPE::LEE_SIN_E) != (CActor*)this) return true;

                return false;
            }), v.end());
    }

    const std::vector<CActor*>& Get_EMarkTargets() const { return m_vecEMarkTargets; }

    // W2 평타 강화 버프(네가 평타 히트에서 Consume 호출)
public:
    void  Apply_WEmpower(int stacks = 2, float duration = 3.f);
    bool  Consume_WEmpowerStack();

private:
    bool  m_bWEmpower = false;
    int   m_iWEmpowerStacks = 0;
    float m_fWEmpowerAcc = 0.f;
    float m_fWEmpowerDuration = 0.f;

    // =========================
    // E 재사용(2타)
    // =========================
private:
    bool  m_bERecastReady = false;
    float m_fERecastAcc = 0.f;
    float m_fERecastWindow = 3.f;



public:
    void  Start_ERecast(float window = 3.f);
    void  Consume_ERecast();

private:
    void  Update_RecastTimers(float dt);
    void  Update_SkillIcons();
    void Play_WDownEffect(const _tchar* pDat);
    void Clear_WDownEffect();
    void Update_WDownEffect();
private:
    _matrix Get_BoneWorldMatrix(const char* pBoneName) const;

    void Play_WEmpowerEffect(const _tchar* pDat);
    void Clear_WEmpowerEffect();
    void Update_WEmpowerEffect();

public:
    virtual _bool Picking(_float3* PickingPoint);
    virtual void PickingTrue();
    virtual void Set_Picked();

public:
    void   Set_QMarkTarget(CActor* pTarget);
    void   Clear_QMarkTarget();
    bool   Can_QRecast() const;
    CActor* Get_QMarkTarget() const { return m_pQMarkTarget; }

    private:
        struct WEFFECT_ATTACH
        {
            std::string Bone;   // "L_Hand" / "R_Hand"
            CEffect* pFx = nullptr;
        };

    vector<WEFFECT_ATTACH> m_vecWEmpowerFx;
    vector<CEffect*> m_vecWDownFx;
private:
    virtual HRESULT Ready_Components(void* pArg) override;

private:
    CActor* m_pQMarkTarget = nullptr;

public:
    static CLeeSin* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr);
    virtual void Free() override;
};
NS_END