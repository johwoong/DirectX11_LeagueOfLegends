#include "VayneSkillStateE.h"
#include "VayneMoveState.h"
#include "VayneProjectile.h"
#include "GameInstance.h"

USING(Client);
using namespace Vayne;

CVayneSkillStateE::CVayneSkillStateE(CVayne* pVayne)
{
    m_pOwner = pVayne;
}

void CVayneSkillStateE::Enter()
{
    m_pOwner->m_bSkillLock = true; // ?? 이동/회전/Run 차단
    m_bProjectileSpawned = false;
    m_bAnimFinished = false;

    CModel* pModel = m_pOwner->Get_Model();
    if (!pModel) return;
    m_pOwner->Play_VayneSFX(TEXT("VayneETalk.wav"), SOUND_VAYNE_TALK, 0.75f);
    m_pOwner->Play_VayneSFX(TEXT("VayneEReady.wav"), SOUND_VAYNE, 0.75f);
    // ?? E 진입 시 애니 강제 전환 (보간은 Play_Animation에서)
    pModel->Set_CurrentAnimIndex(CVayne::ANIM::E);
}

CVayneState* CVayneSkillStateE::AI_Behaviour(_float)
{
    // ? E는 시전 중 절대 끊기면 안 됨
    return nullptr;
}

CVayneState* CVayneSkillStateE::Update(_float fTimeDelta)
{
    CModel* pModel = m_pOwner->Get_Model();
    if (!pModel)
        return new CMoveState(m_pOwner);

    // =========================
    // 1. 애니 재생
    // =========================
    bool bAnimEnd = pModel->Play_Animation(
        fTimeDelta,
        false,
        nullptr,
        0.25f
    );

    if (bAnimEnd)
        m_bAnimFinished = true;

    // =========================
    // 2. 애니 이벤트 시점
    // =========================
    if (!m_bProjectileSpawned)
    {
        for (auto& evt : pModel->Get_Events())
        {
            if (!evt.isPlay || evt.eType != ANIMEVENT::EVENT_PROJECTILE)
                continue;

            CActor* pTarget = m_pOwner->Check_FiledTarget();
            if (!pTarget)
                break;

            // =========================
            // ??? 여기서 판정 확정 ???
            // =========================

            // 1) 데미지
            pTarget->Take_Damage(50, m_pOwner);

            // 2) W 스택
            pTarget->Apply_VayneSilverMark(m_pOwner);

            // 3) E 넉백 (베인 기준 방향)
            {
                _vector vMyPos =
                    m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
                _vector vTarPos =
                    pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

                _vector vDir = XMVectorSetY(vTarPos - vMyPos, 0.f);
                if (XMVectorGetX(XMVector3LengthSq(vDir)) > 0.0001f)
                {
                    vDir = XMVector3Normalize(vDir);
                    vTarPos += vDir * 3.5f;
                    pTarget->Get_Transform()->Set_State(
                        CTransform::STATE_TRANSLATION,
                        XMVectorSetW(vTarPos, 1.f));
                }
            }

            // =========================
            // 4) Projectile (연출용)
            // =========================
            _vector vSpawn = m_pOwner->Get_Transform()
                ->Get_State(CTransform::STATE_TRANSLATION);
            vSpawn += XMVectorSet(0.f, 1.f, 0.f, 0.f);
            vSpawn = XMVectorSetW(vSpawn, 1.f);

            _vector vDir =
                XMVectorSetY(
                    pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION) - vSpawn,
                    0.f);
            vDir = XMVector3Normalize(vDir);

            CProjectile::ProjectileTag tag{};
            tag.eCollisionGroup = ENEMY;
            tag.eProjType = CVayneProjectile::E_CONDEMN;
            tag.vInitPositon = vSpawn;
            tag.vTargetDir = vDir;
            tag.pOwner = m_pOwner;
            tag.fDeadTime = 0.4f;
            tag.ePoolLevel = LEVEL_GAMEPLAY;
            tag.pPoolLayerTag = TEXT("Layer_Projectile_Vayne"); // 타입별로 다르게
            tag.bPrewarm = false;

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

            m_bProjectileSpawned = true;
            break;
        }
    }

    // =========================
    // 3. 종료
    // =========================
    if (m_bAnimFinished && m_bProjectileSpawned)
        return new CMoveState(m_pOwner);

    return nullptr;
}

CVayneState* CVayneSkillStateE::LateUpdate(_float)
{
    return nullptr;
}

void CVayneSkillStateE::Exit()
{
    m_pOwner->m_bSkillLock = false;
}