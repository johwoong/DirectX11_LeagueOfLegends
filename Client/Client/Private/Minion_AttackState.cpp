#include "Minion_AttackState.h"
#include "GameInstance.h"
#include "Minion_Melee.h"
#include "Minion_Range.h"
#include "Minion_ChaseState.h"
#include "Minion_MoveState.h"
#include "PoolManager.h"
#include "Projectile.h"
USING(Client);
using namespace Minion;

static float GetAttackAnimSpeedMul(CMinion* pOwner)
{
    if (!pOwner)
        return 1.f;

    const float BASE_ATTACK_SPEED = 0.83f; // 네가 쓰는 기본값

    float cur = pOwner->Get_Stats().m_fAttackSpeed;
    if (cur <= 0.f)
        return 1.f;

    float mul = cur / BASE_ATTACK_SPEED;   // AS 비율만큼 애니를 빠르게/느리게

    // 너무 극단적이지 않게 클램프
    if (mul < 0.3f) mul = 0.3f;
    if (mul > 3.0f) mul = 3.0f;

    return mul;
}

static void SpawnRangeProjectile(CMinion* pOwner)
{
    if (!pOwner)
        return;

    CActor* pTarget = pOwner->Get_Target();
    if (!pTarget || pTarget->Get_Dead())
        return;

    CGameInstance* pGI = CGameInstance::GetInstance();
    if (!pGI)
        return;
    pGI->AddRef();

 
    // ?? 발사 위치: 무기/손 뼈에서 가져오기
    _float4x4 Pivot = pOwner->Get_Model()->Get_PivotFloat4x4();
    CBone* pBone = pOwner->Get_Model()->Get_BonePtr("Buffbone_Glb_Weapon_1"); 
    if (!pBone)
    {
        Safe_Release(pGI);
        return;
    }

    _matrix BoneWorld =
        pBone->Get_CombinedTransformationMatrix() *
        XMLoadFloat4x4(&Pivot) *
        pOwner->Get_Transform()->Get_WorldMatrix();

    _float3 vStart{};
    XMStoreFloat3(&vStart, BoneWorld.r[3]);

    Safe_Release(pGI);
}

CAttackState::CAttackState(class CMinion* pMinion)
{
    m_pOwner = pMinion;
    m_bDamageApplied = false;   
}

CAttackState::~CAttackState()
{
    if (m_pAttackColider)
    {
        CCollision_Manager::GetInstance()->Collect_Collider(
            CColider::TYPE_SPHERE,
            m_pAttackColider);

        m_pAttackColider = nullptr;
    }
}


CMinionState* CAttackState::AI_Behaviour(_float fTimeDelta)
{
    return nullptr;
}


CMinionState* CAttackState::Update(_float fTimeDelta)
{
    if (!m_pOwner)
        return nullptr;

    m_fAttackElapsed += fTimeDelta;

    CActor* pTarget = m_pOwner->Check_FiledTarget();
    if (!pTarget)
    {
        m_pOwner->Find_Target(fTimeDelta);
        pTarget = m_pOwner->Get_Target();

        if (!pTarget)
            return new CMoveState(m_pOwner);
    }

    if (!m_pOwner->Is_TargetInAttackRange())
        return new CChaseState(m_pOwner);

    // 타겟 바라보기
    m_pOwner->Face_Target();

    float fAnimSpeedMul = GetAttackAnimSpeedMul(m_pOwner);

    m_bIsAnimationFinished =
        m_pOwner->Get_Model()->Play_Animation(
            fTimeDelta * fAnimSpeedMul,
            false,
            nullptr,
            0.1f);

    if (m_bIsAnimationFinished &&
        m_pOwner->Get_AttackTimer() >= m_pOwner->Get_AttackCoolTime())
    {
        m_pOwner->Attack_Target(fTimeDelta); 
        m_pOwner->Reset_AttackTimer();

        _int iIdx = m_pOwner->Get_AttackIndex();
        m_pOwner->Set_AttackIndex(iIdx + 1);

        return new CAttackState(m_pOwner);
    }

    vector<ANIMEVENT> pEvents = m_pOwner->Get_Model()->Get_Events();

    for (auto& pEvent : pEvents)
    {
        if (pEvent.isPlay)
        {
            if (pEvent.eType == ANIMEVENT::EVENT_COLLIDER &&
                m_pOwner->Get_MinType() == MINION_MELEE)
            {
                if (nullptr == m_pAttackColider)
                {
                    m_pAttackColider = Get_Collider(
                        CColider::TYPE_SPHERE,
                        _float3(0.01f, 0.01f, 0.01f),
                        _float3(0.f, 0.f, 0.f),
                        _float3(0.f, 0.f, 0.f));

                    _float4x4 Pivot = m_pOwner->Get_Model()->Get_PivotFloat4x4();

                    m_ColliderMatrix =
                        m_pOwner->Get_Model()->Get_BonePtr("Staff")->Get_CombinedTransformationMatrix() *
                        XMLoadFloat4x4(&Pivot) *
                        m_pOwner->Get_Transform()->Get_WorldMatrix();

                    m_ColliderMatrix.r[0] = XMVector4Normalize(m_ColliderMatrix.r[0]);
                    m_ColliderMatrix.r[1] = XMVector4Normalize(m_ColliderMatrix.r[1]);
                    m_ColliderMatrix.r[2] = XMVector4Normalize(m_ColliderMatrix.r[2]);
                }
            }
            else if (pEvent.eType == ANIMEVENT::EVENT_PROJECTILE &&
                m_pOwner->Get_MinType() == MINION_CASTER)
            {
                if (!m_bDamageApplied)
                {
                    CGameInstance* pGI = CGameInstance::GetInstance();

                    CBone* pBone =
                        m_pOwner->Get_Model()->Get_BonePtr("Buffbone_Glb_Weapon_1");

                    if (!pBone)
                        break;

                    _float4x4 pivot = m_pOwner->Get_Model()->Get_PivotFloat4x4();
                    _matrix boneWorld =
                        pBone->Get_CombinedTransformationMatrix() *
                        XMLoadFloat4x4(&pivot) *
                        m_pOwner->Get_Transform()->Get_WorldMatrix();

                    _vector vStart = boneWorld.r[3];
                    vStart = XMVectorSetW(vStart, 1.f);

                    _vector vTargetPos =
                        m_pOwner->Get_Target()->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

                    _vector vDir = vTargetPos - vStart;
                    vDir = XMVectorSetY(vDir, 0.f);
                    vDir = XMVector3Normalize(vDir);

                    CProjectile::PROJECTILEDESC Desc{};
                    Desc.eCollisionGroup = MINION;
                    Desc.vInitPositon = vStart;
                    Desc.vTargetDir = vDir;
                    Desc.fVelocity = 14.f;
                    Desc.fDeadTime = 1.f;
                    Desc.iDamage = (int)m_pOwner->Get_Stats().m_fAttackPower;
                    Desc.pOwner = m_pOwner;
                    Desc.pTarget = m_pOwner->Get_Target();
                    Desc.ePoolLevel = LEVEL_GAMEPLAY;
                    Desc.pPoolLayerTag = TEXT("Layer_Projectile_Minion"); // 타입별로 다르게
                    Desc.bPrewarm = false;

                    CPoolManager* PM = CPoolManager::GetInstance();
                    CGameInstance* GI = CGameInstance::GetInstance();

                    if (!PM->Reuse_Pooling_Object(
                        LEVEL_GAMEPLAY,
                        TEXT("Layer_Projectile_Minion"),
                        &Desc))
                    {
                        if (FAILED(CGameInstance::GetInstance()->Add_GameObject(TEXT("Prototype_GameObject_MinionProjectile"), LEVEL_GAMEPLAY, TEXT("Layer_Projectile_Minion"), &Desc)))
                            return nullptr;
                    }

                    m_bDamageApplied = true;
                    m_pOwner->Reset_AttackTimer();
                }
            }
        }
        else
        {
            // 이벤트 끝나면 근접 콜라이더 회수
            if (nullptr != m_pAttackColider)
            {
                CCollision_Manager::GetInstance()->Collect_Collider(
                    CColider::TYPE_SPHERE,
                    m_pAttackColider);
                m_pAttackColider = nullptr;
            }
        }

        if (nullptr != m_pAttackColider)
            m_pAttackColider->Update(m_ColliderMatrix);
    }

    if (m_pAttackColider && !m_bDamageApplied)
    {
        CActor* pCurTarget = m_pOwner->Get_Target();
        if (pCurTarget && !pCurTarget->Get_Dead())
        {
            CColider* pTargetCol = pCurTarget->Get_Collider(); // 몸통 콜라이더
            if (pTargetCol && m_pAttackColider->Collision(pTargetCol))
            {
                // 데미지 적용 (미니언 공격력 사용)
                pCurTarget->Take_Damage(
                    (int)m_pOwner->Get_Stats().m_fAttackPower,
                    m_pOwner);

                m_bDamageApplied = true;       // 한 번만 때리게
                m_pOwner->Reset_AttackTimer(); // 쿨타임 리셋
            }
        }
    }

    return nullptr;
}

CMinionState* CAttackState::Late_Update(_float fTimeDelta)
{
#ifdef _DEBUG
    m_pOwner->Get_Renderer()->Add_Debug(m_pAttackColider);
#endif
    return nullptr;
}

void CAttackState::Enter()
{
    m_eState = STATE_ATTACK;
    m_fAttackElapsed = 0.f;

    if (!m_pOwner || !m_pOwner->Get_Model())
        return;

    MINION_TYPE eType = m_pOwner->Get_MinType();

    _uint iAnim = 0;
    _int  iIdx = m_pOwner->Get_AttackIndex();

    if (eType == MINION_MELEE)
    {
        switch (iIdx % 4)
        {
        case 0: iAnim = CMinion_Melee::ANIM::ATTACK;   break;
        case 1: iAnim = CMinion_Melee::ANIM::ATTACK2;   break;
        case 2: iAnim = CMinion_Melee::ANIM::ATTACK3;   break;
        case 3: iAnim = CMinion_Melee::ANIM::ATTACK3_B; break;
        }
    }
    else // MINION_CASTER
    {
        switch (iIdx % 2)
        {
        case 0: iAnim = CMinion_Range::ANIM::ATTACK1; break;
        case 1: iAnim = CMinion_Range::ANIM::ATTACK2; break;
        }
    }

    m_pOwner->Get_Model()->Set_CurrentAnimIndex(iAnim);
}

void CAttackState::Exit()
{
    if (m_pAttackColider)
    {
        CCollision_Manager::GetInstance()->Collect_Collider(
            CColider::TYPE_SPHERE,
            m_pAttackColider);

        m_pAttackColider = nullptr;
    }

    m_bDamageApplied = false;   // 다음 공격을 위해 플래그 초기화
}

CColider* CAttackState::Get_Collider(CColider::TYPE eType, _float3 vScale, _float3 vRotation, _float3 vPosition)
{
    CCollision_Manager* pCollisionManager = CCollision_Manager::GetInstance();

    CColider::COLLIDERDESC ColliderDesc;

    ColliderDesc.vScale = vScale;
    ColliderDesc.vRotation = vRotation;
    ColliderDesc.vPosition = vPosition;

    switch (eType)
    {
    case Engine::CColider::TYPE_AABB:
        return pCollisionManager->Reuse_Collider(eType, LEVEL_STATIC, TEXT("Prototype_Component_Colider_AABB"), &ColliderDesc);
        break;
    case Engine::CColider::TYPE_OBB:
        return pCollisionManager->Reuse_Collider(eType, LEVEL_STATIC, TEXT("Prototype_Component_Colider_OBB"), &ColliderDesc);
        break;
    case Engine::CColider::TYPE_SPHERE:
        return pCollisionManager->Reuse_Collider(eType, LEVEL_STATIC, TEXT("Prototype_Component_Colider_Sphere"), &ColliderDesc);
        break;
    }

    return nullptr;
}
