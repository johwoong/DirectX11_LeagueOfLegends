#include "TeemoAttackState.h"

#include "GameInstance.h"
#include "Champion.h"
#include "Enemy.h"
#include "PickingManager.h"
#include "PoolManager.h"

#include "TeemoProjectile.h"    // ★ TeemoProjectile 타입 사용
#include "PlayerIdleState.h"
#include "PlayerRunState.h"

USING(Client)
USING(Player)

CTemooAttackState::CTemooAttackState(CPlayer* pPlayer, STATE_ID eStateType, _float /*fTime*/)
{
    m_pOwner = pPlayer;
    m_eState = eStateType;
    m_ePlayerID = m_pOwner->Get_PlayerID();
}

CPlayerState* CTemooAttackState::HandleInput()
{
    CGameInstance* GI = CGameInstance::GetInstance();
    CPickingManager* PM = CPickingManager::GetInstance();

    if (GI->Mouse_Down(DIMK_RBUTTON))
    {
        _float3 clickPos = PM->Get_PickingPos();
        CGameObject* pickedObj = PM->Get_PickedObj();

        // 1) Enemy 찍었는지 확인
        if (CEnemy* enemy = dynamic_cast<CEnemy*>(pickedObj))
        {
            m_pOwner->Set_AttackTarget(enemy);

            _vector myPos = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
            _vector tarPos = enemy->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

            float dist = GetDistanceXZ(myPos, tarPos);
            float range = dynamic_cast<CChampion*>(m_pOwner)->Get_WorldAttackRange();

            if (dist <= range)
            {
                // ★ 같은 상태 유지 + 공격 사이클만 다시 시작(ATTACK1/CRIT 랜덤)
                Begin_AttackCycle();
                return nullptr;
            }
            else
            {
                Set_AttackMoveToTarget(m_pOwner, enemy);
                return new CRunState(m_pOwner);
            }
        }
        else
        {
            // Enemy 아닌 곳 찍으면 → 타겟 해제 + 이동
            m_pOwner->Set_AttackTarget(nullptr);
            m_pOwner->Set_TargetPosition(clickPos);
            m_pOwner->Get_Transform()->Set_Destination(XMLoadFloat3(&clickPos), m_pOwner->Get_Navigation());
            return new CRunState(m_pOwner);
        }
    }

    return nullptr;
}

CPlayerState* CTemooAttackState::Update(_float fTimeDelta)
{
    // 보간값(요청: 자연스럽게)
    constexpr float kInterp_Attack = 0.08f;
    constexpr float kInterp_ToIdle = 0.12f;

    // 공격 과속 방지(요청: 자연스럽게) : 기본 1.0
    // 원하면 1.05f 정도만
    constexpr float kAttackSpeedMul = 1.0f;

    CActor* target = m_pOwner->Get_AttackTarget();

    // TOIDLE 중에는 타겟 없어도 마무리만 하고 Idle로
    if (m_ePhase != PHASE::TOIDLE)
    {
        if (!target || target->Get_Dead())
            return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
    }

    // =========================
    // 1) ATTACK
    // =========================
    if (m_ePhase == PHASE::ATTACK)
    {
        _vector myPos = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
        _vector tarPos = target->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

        float dist = GetDistanceXZ(myPos, tarPos);
        float range = dynamic_cast<CChampion*>(m_pOwner)->Get_WorldAttackRange();

        // 사거리 밖이면 추격
        if (dist > range)
        {
            Set_AttackMoveToTarget(m_pOwner, target);
            return new CRunState(m_pOwner);
        }

        // 타겟 바라보기
        _vector lookDir = tarPos - myPos;
        lookDir = XMVectorSetY(lookDir, 0.f);
        lookDir = XMVector3Normalize(lookDir);
        m_pOwner->Get_Transform()->SmoothLookAt(-lookDir, fTimeDelta, 8.f);

        _float animDT = fTimeDelta * kAttackSpeedMul;

        // 공격 애니: 루프 X
        m_bIsAnimationFinished = m_pOwner->Get_Model()->Play_Animation(
            animDT,
            false,
            nullptr,
            kInterp_Attack
        );

        // ===== 애니 이벤트로 투사체 스폰 (ATTACK1/CRIT 둘 다 너가 이벤트 넣어둠) =====
        const vector<ANIMEVENT> events = m_pOwner->Get_Model()->Get_Events();
        for (const auto& evt : events)
        {
            if (!evt.isPlay) continue;
            if (evt.eType != ANIMEVENT::EVENT_PROJECTILE) continue;
            if (m_bProjectileSpawned) break;

            Spawn_TemooProjectile(target);
            m_bProjectileSpawned = true;
            break;
        }

        // 공격 애니 끝났으면
        if (m_bIsAnimationFinished)
        {
            // CRIT이면 바로 Idle (요청사항)
            if (m_iAttackAnim == CTemoo::ANIM::CRIT)
                return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);

            // ATTACK1이면 ToIdle로 자연스럽게
            m_ePhase = PHASE::TOIDLE;
            m_bIsAnimationFinished = false;
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CTemoo::ANIM::ATTACK_TOIDLE);
            return nullptr;
        }

        return nullptr;
    }

    // =========================
    // 2) TOIDLE
    // =========================
    if (m_ePhase == PHASE::TOIDLE)
    {
        const bool bEnd = m_pOwner->Get_Model()->Play_Animation(
            fTimeDelta,
            false,
            nullptr,
            kInterp_ToIdle
        );

        if (bEnd)
            return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);

        return nullptr;
    }

    return nullptr;
}

void CTemooAttackState::Spawn_TemooProjectile(CActor* pTarget)
{
    if (!pTarget) return;

    // === EzrealAttackState랑 동일한 손뼈 기반 스폰 ===
    CBone* pHand = m_pOwner->Get_Model()->Get_BonePtr("L_Hand");
    _vector vSpawn = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
    vSpawn = XMVectorSetW(vSpawn, 1.f);

    if (pHand)
    {
        _float4x4 pivot = m_pOwner->Get_Model()->Get_PivotFloat4x4();
        _matrix pivotMat = XMLoadFloat4x4(&pivot);

        _matrix HandMatrix =
            pHand->Get_CombinedTransformationMatrix() *
            pivotMat *
            m_pOwner->Get_Transform()->Get_WorldMatrix();

        HandMatrix.r[0] = XMVector4Normalize(HandMatrix.r[0]);
        HandMatrix.r[1] = XMVector4Normalize(HandMatrix.r[1]);
        HandMatrix.r[2] = XMVector4Normalize(HandMatrix.r[2]);

        vSpawn = HandMatrix.r[3];
        vSpawn = XMVectorSetW(vSpawn, 1.f);
    }

    CProjectile::PROJECTILEDESC ProjDesc;
    ProjDesc.eCollisionGroup = PLAYER;
    ProjDesc.fDeadTime = 10.f;
    ProjDesc.fVelocity = 6.f;
    ProjDesc.iDamage = 20; // 너 게임 수치에 맞게
    ProjDesc.eProjType = CTemooProjectile::BASE;
    ProjDesc.vInitPositon = vSpawn;

    ProjDesc.pOwner = m_pOwner;
    ProjDesc.pTarget = pTarget;
    ProjDesc.vTargetPosition = pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
    ProjDesc.vTargetDir = XMVector3Normalize(ProjDesc.vTargetPosition - ProjDesc.vInitPositon);

    ProjDesc.ePoolLevel = LEVEL_GAMEPLAY;
    ProjDesc.pPoolLayerTag = TEXT("Layer_TemooProjectile");

    CPoolManager* PM = CPoolManager::GetInstance();

    // 풀 재사용 우선
    if (!PM->Reuse_Pooling_Object(LEVEL_GAMEPLAY, TEXT("Layer_TemooProjectile"), &ProjDesc))
    {
        // ★ Teemo Projectile 프로토타입으로 생성
        if (FAILED(CGameInstance::GetInstance()->Add_GameObject(
            TEXT("Prototype_GameObject_TemooProjectile"),
            LEVEL_GAMEPLAY,
            TEXT("Layer_TemooProjectile"),
            &ProjDesc)))
        {
            return;
        }
    }
}

void CTemooAttackState::Begin_AttackCycle()
{
    m_ePhase = PHASE::ATTACK;
    m_bIsAnimationFinished = false;
    m_bProjectileSpawned = false;

    // 필요하면 확률 가중치도 가능(지금은 50:50)
    const _uint AttackAnims[] =
    {
        CTemoo::ANIM::ATTACK1,
        CTemoo::ANIM::CRIT
    };

    int idx = rand() % 2;
    m_iAttackAnim = AttackAnims[idx];

    m_pOwner->Get_Model()->Set_CurrentAnimIndex(m_iAttackAnim);
}

CPlayerState* CTemooAttackState::Late_Update(_float /*fTimeDelta*/)
{
    return nullptr;
}

void CTemooAttackState::Enter()
{
    __super::Enter();
    m_eState = STATE_ID::STATE_ATTACK1;

    Begin_AttackCycle();
}

void CTemooAttackState::Exit()
{
    __super::Exit();
}
