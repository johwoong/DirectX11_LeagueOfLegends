#include "VayneAttackState.h"
#include "VayneMoveState.h"
#include "VayneIdleState.h"
#include "Vayne.h"
#include "VayneReturnState.h"  
#include "GameInstance.h"
#include "Projectile.h"
#include "Champion.h"
#include "Minion.h"
#include "Building.h"
#include "VayneSkillStateQ.h"
#include "VayneSkillStateE.h"

USING(Client);
using namespace Vayne;

CAttackState::CAttackState(CVayne* pVayne)
{
    m_pOwner = pVayne;
}

CVayneState* CAttackState::AI_Behaviour(_float fTimeDelta)
{
    UNREFERENCED_PARAMETER(fTimeDelta);
    return nullptr;
}

CVayneState* CAttackState::Update(_float fTimeDelta)
{
    if (!m_pOwner)
        return nullptr;

    CActor* pTarget = m_pOwner->Check_FiledTarget();
    if (!pTarget)
        return new CMoveState(m_pOwner);

    float dist = m_pOwner->Target_Distance(pTarget);
    float range = m_pOwner->Get_AttackRangeWorld();

    // 사거리 벗어나면 추격
    if (dist > range * 1.05f)
        return new CMoveState(m_pOwner);

    // ======================
    // 1. 타겟 바라보기
    // ======================
    if (CTransform* pTr = m_pOwner->Get_Transform())
    {
        _vector myPos = pTr->Get_State(CTransform::STATE_TRANSLATION);
        _vector tarPos = pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

        _vector vLookDir = XMVectorSetY(myPos - tarPos, 0.f);

        if (XMVectorGetX(XMVector3LengthSq(vLookDir)) > 0.0001f)
        {
            vLookDir = XMVector3Normalize(vLookDir);

            // ? 기존 스케일 보존
            _vector vOldRight = pTr->Get_State(CTransform::STATE_RIGHT);
            float   fScale = XMVectorGetX(XMVector3Length(vOldRight));
            if (fScale < 0.0001f) fScale = 1.f;

            _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
            _vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vLookDir));
            _vector vLook = XMVector3Normalize(vLookDir);

            // ?? 스케일 다시 곱해준다
            vRight *= fScale;
            vUp *= fScale;
            vLook *= fScale;

            pTr->Set_State(CTransform::STATE_RIGHT, XMVectorSetW(vRight, 0.f));
            pTr->Set_State(CTransform::STATE_UP, XMVectorSetW(vUp, 0.f));
            pTr->Set_State(CTransform::STATE_LOOK, XMVectorSetW(vLook, 0.f));
        }
    }

    // ======================
    // 2. 공격 애니 1회
    // ======================
    CModel* pModel = m_pOwner->Get_Model();
    m_bIsAnimFinished = pModel->Play_Animation(
        fTimeDelta,
        false,      // 루프 X
        nullptr,
        0.1f
    );

    // ======================
    // 3. 투사체 이벤트
    // ======================
    for (auto& evt : pModel->Get_Events())
    {
        if (evt.isPlay && evt.eType == ANIMEVENT::EVENT_PROJECTILE && !m_bProjectileSpawned)
        {
            m_bProjectileSpawned = true;

            if (m_pOwner->Has_TeemoBlind())
                break;

            if (m_pOwner->m_bConsumeQThisAttack)
            {
                // Q 강화 Attack 사운드 2개 랜덤 1개 재생
                // rand()는 이미 너 코드에서 쓰고 있으니 그대로 사용
                const int pick = rand() % 2;
                if (pick == 0)
                    m_pOwner->Play_VayneSFX(TEXT("VayneQAttack1.wav"), SOUND_VAYNE, 0.75f);
                else
                    m_pOwner->Play_VayneSFX(TEXT("VayneQAttack2.wav"), SOUND_VAYNE, 0.75f);
            }
            else
            {
                // 일반 공격
                m_pOwner->Play_VayneSFX(TEXT("VayneAttack.wav"), SOUND_VAYNE, 0.70f);
            }


            CTransform* pTr = m_pOwner->Get_Transform();
            _vector vSpawn = pTr->Get_State(CTransform::STATE_TRANSLATION);
            vSpawn += XMVectorSet(0.f, 1.0f, 0.f, 0.f);
            vSpawn = XMVectorSetW(vSpawn, 1.f);

            // 타겟 방향
            _vector vTar = pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
            _vector vDir = XMVectorSetY(vTar - vSpawn, 0.f);
            vDir = XMVector3Normalize(vDir);

            CProjectile::ProjectileTag tag{};
            tag.eCollisionGroup = ENEMY;
            tag.vInitPositon = vSpawn;
            tag.vTargetDir = vDir;
            tag.pOwner = m_pOwner;
            tag.ePoolLevel = LEVEL_GAMEPLAY;
            tag.pPoolLayerTag = TEXT("Layer_Projectile_Vayne"); // 타입별로 다르게
            tag.bPrewarm = false;
            tag.pTarget = pTarget;   // ★★★ 핵심
            // =========================
            // 강화 평타
            // =========================
            if (m_pOwner->m_bConsumeQThisAttack)
            {
                tag.eProjType = 1;
                tag.iDamage = 40;
                tag.fVelocity = 32.f;
            }
            else
            {
                tag.eProjType = 0;
                tag.iDamage = 20;
                tag.fVelocity = 25.f;
            }

            CPoolManager* PM = CPoolManager::GetInstance();
            CGameInstance* GI = CGameInstance::GetInstance();

            if (!PM->Reuse_Pooling_Object(
                LEVEL_GAMEPLAY,
                TEXT("Layer_Projectile_Vayne"),
                &tag))
            {
                GI->Add_GameObject(
                    TEXT("Prototype_GameObject_VayneProjectile"),
                    LEVEL_GAMEPLAY,
                    TEXT("Layer_Projectile_Vayne"),
                    &tag);
            }
        }
    }

    // ======================
    // 4. 공격 끝 → 무조건 카이팅
    // ======================

 // ======================
// 4. 공격 끝 → 스킬 판정
// ======================
    if (m_bIsAnimFinished)
    {
        m_bIsAnimFinished = false;
        m_bProjectileSpawned = false;

        float dist = m_pOwner->Target_Distance(pTarget);

        // =========================
        // ?? 1?? E 먼저 (Condemn은 강한 스킬)
        // =========================
        if (m_pOwner->CanUseSkill(CEnemy::SKILL_E, dist))
        {
            float r = (rand() % 1000) / 1000.f;
            if (r <= m_pOwner->GetSkill(CEnemy::SKILL_E).fUseProb)
            {
                m_pOwner->ConsumeSkill(CEnemy::SKILL_E);
                return new CVayneSkillStateE(m_pOwner);
            }
        }

        // =========================
        // 2?? Q
        // =========================
        if (m_pOwner->CanUseSkill(CEnemy::SKILL_Q, dist))
        {
            float r = (rand() % 1000) / 1000.f;
            if (r <= m_pOwner->GetSkill(CEnemy::SKILL_Q).fUseProb)
            {
                m_pOwner->ConsumeSkill(CEnemy::SKILL_Q);
                return new CVayneSkillStateQ(m_pOwner);
            }
        }

        // =========================
        // 3?? 이동
        // =========================
        return new CMoveState(m_pOwner);
    }
    return nullptr;
}

CVayneState* CAttackState::LateUpdate(_float fTimeDelta)
{
    return nullptr;
}

void CAttackState::Enter()
{
    m_bProjectileSpawned = false;
    m_bIsAnimFinished = false;

    CModel* pModel = m_pOwner->Get_Model();
    if (!pModel)
        return;

    // =========================
    // Q 강화 여부 확정 (여기서!)
    // =========================
    if (m_pOwner->m_bQEmpowered)
    {
        m_pOwner->m_bConsumeQThisAttack = true;
        pModel->Set_CurrentAnimIndex(CVayne::ANIM::Q_ATTACK);
        return;
    }

    // =========================
    // 일반 공격
    // =========================
    m_pOwner->m_bConsumeQThisAttack = false;

    const int AttackAnims[] =
    {
        CVayne::ANIM::ATTACK1,
        CVayne::ANIM::ATTACK2,
        CVayne::ANIM::CRIT,
    };

    int r = rand() % 10;
    int animIdx =
        (r < 4) ? AttackAnims[0] :
        (r < 8) ? AttackAnims[1] :
        AttackAnims[2];

    pModel->Set_CurrentAnimIndex(animIdx);
}

void CAttackState::Exit()
{
    if (m_pOwner->m_bConsumeQThisAttack)
    {
        m_pOwner->m_bConsumeQThisAttack = false;
        m_pOwner->m_bQEmpowered = false;
        m_pOwner->m_fQEmpoweredTime = 0.f;
    }
}