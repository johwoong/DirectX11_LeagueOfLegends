#include "EzrealAttackState.h"

#include "GameInstance.h"
#include "Champion.h"
#include "Ezreal.h"
#include "Enemy.h"
#include "PickingManager.h"

#include "PlayerIdleState.h"
#include "PlayerRunState.h"
#include "Projectile.h"

USING(Client)
USING(Player)



CEzrealAttackState::CEzrealAttackState(CPlayer* pPlayer, STATE_ID eStateType, _float /*fTime*/)
{
    m_pOwner = pPlayer;
    m_eState = eStateType;
    m_ePlayerID = m_pOwner->Get_PlayerID();

}

CPlayerState* CEzrealAttackState::HandleInput()
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
                // 이미 사거리 안 → 현 위치에서 계속 공격
                m_bIsAnimationFinished = false;
                m_bProjectileSpawned = false;
                // 필요하면 여기서 평타 애니 다시 세팅
                return nullptr; // AttackState 유지
            }
            else
            {
                // 사거리 밖 → "공격 포지션"으로 이동 설정 후 RunState로
                Set_AttackMoveToTarget(m_pOwner, enemy);
                return new CRunState(m_pOwner);
            }
        }
        else
        {
            // 2) Enemy 아닌 곳 찍으면 → 타겟 해제 + 땅으로 이동
            m_pOwner->Set_AttackTarget(nullptr);
            m_pOwner->Set_TargetPosition(clickPos);
            m_pOwner->Get_Transform()->Set_Destination(
                XMLoadFloat3(&clickPos),
                m_pOwner->Get_Navigation());

            return new CRunState(m_pOwner);
        }
    }

    return nullptr;
}

CPlayerState* CEzrealAttackState::Update(_float fTimeDelta)
{
    CActor* target = m_pOwner->Get_AttackTarget();

    if (!target || target->Get_Dead())
        return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);

    _vector myPos = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
    _vector tarPos = target->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

    float dist = GetDistanceXZ(myPos, tarPos);
    float range = dynamic_cast<CChampion*>(m_pOwner)->Get_WorldAttackRange();  

    if (dist > range)
    {
        CActor* pTarget = m_pOwner->Get_AttackTarget();
        if (pTarget)
        {
            // 공격용 포지션으로 이동 설정
            Set_AttackMoveToTarget(m_pOwner, pTarget);
            return new CRunState(m_pOwner);
        }

        // 타겟이 없으면 그냥 Idle
        return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
    }

    _vector lookDir = tarPos - myPos;  
    lookDir = XMVectorSetY(lookDir, 0.f);
    lookDir = XMVector3Normalize(lookDir);

    m_pOwner->Get_Transform()->SmoothLookAt(-lookDir, fTimeDelta, 8.f);
    m_bIsAnimationFinished = m_pOwner->Get_Model()->Play_Animation(
        fTimeDelta,
        m_pOwner->Is_AnimationLoop(m_pOwner->Get_Model()->Get_CurrentAnimIndex())
    );

    vector<ANIMEVENT> events = m_pOwner->Get_Model()->Get_Events();

    for (auto& evt : events)
    {
        if (evt.isPlay && m_bIsAnimationFinished == false)
        {
            if (evt.eType == ANIMEVENT::EVENT_PROJECTILE)
            {
                if (!m_bProjectileSpawned)
                {
                    CGameInstance* pGI = CGameInstance::GetInstance();

                    CBone* pHand = m_pOwner->Get_Model()->Get_BonePtr("L_Hand");

                    _float4x4 pivot = m_pOwner->Get_Model()->Get_PivotFloat4x4();
                    _matrix pivotMat = XMLoadFloat4x4(&pivot);

                    _matrix HandMatrix =
                        pHand->Get_CombinedTransformationMatrix() *
                        pivotMat *                                            
                        m_pOwner->Get_Transform()->Get_WorldMatrix();

                    HandMatrix.r[0] = XMVector4Normalize(HandMatrix.r[0]);
                    HandMatrix.r[1] = XMVector4Normalize(HandMatrix.r[1]);
                    HandMatrix.r[2] = XMVector4Normalize(HandMatrix.r[2]);

                    _vector vHandVec = HandMatrix.r[3];
                    CProjectile::PROJECTILEDESC desc{};
                    desc.pOwner = m_pOwner;
                    desc.pTarget = m_pOwner->Get_AttackTarget();
                    desc.fSpeed = 4.f;
                    desc.vStartPos = vHandVec;

                    pGI->Add_GameObject(
                        TEXT("Prototype_GameObject_Projectile"),
                        LEVEL_GAMEPLAY,
                        L"Layer_Projectile",
                        &desc);

                    m_bProjectileSpawned = true;
                }
            }
        }
    }

   

    if (m_bIsAnimationFinished)
        return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);

    return nullptr;
}

CPlayerState* CEzrealAttackState::Late_Update(_float /*fTimeDelta*/)
{
    return nullptr;
}

void CEzrealAttackState::Enter()
{
    __super::Enter();
    m_eState = STATE_ID::STATE_ATTACK1;
    m_bProjectileSpawned = false;

    const int AttackAnims[] =
    {
        CEzreal::ANIM::ATTACK1,
        CEzreal::ANIM::ATTACK2,
        CEzreal::ANIM::ATTACK3,
        CEzreal::ANIM::ATTACK4,
        CEzreal::ANIM::CRIT,
    };

    int idx = rand() % 5;
    m_pOwner->Get_Model()->Set_CurrentAnimIndex(idx);
}

void CEzrealAttackState::Exit()
{
    __super::Exit();
}
