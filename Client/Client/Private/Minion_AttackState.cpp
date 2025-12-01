#include "Minion_AttackState.h"
#include "GameInstance.h"
#include "Minion_Melee.h"
#include "Minion_Range.h"
#include "Minion_ChaseState.h"
#include "Minion_MoveState.h"
USING(Client);
using namespace Minion;

CAttackState::CAttackState(class CMinion* pMinion)
{
    m_pOwner = pMinion;
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
        m_pOwner->Find_Target();
        pTarget = m_pOwner->Get_Target();

        if (!pTarget)
            return new CMoveState(m_pOwner);
    }

    if (!m_pOwner->Is_TargetInAttackRange())
        return new CChaseState(m_pOwner);

    m_bIsAnimationFinished =
        m_pOwner->Get_Model()->Play_Animation(
            fTimeDelta,
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
            if (ANIMEVENT::EVENTTYPE::EVENT_COLLIDER == pEvent.eType)
            {
                if (nullptr == m_pAttackColider)
                {
                    m_pAttackColider = Get_Collider(CColider::TYPE_SPHERE, _float3(0.01f, 0.01f, 0.01f), _float3(0.f, 0.f, 0.f), _float3(0.f, 0.f, 0.f));

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
        }
        else
        {
            if (nullptr != m_pAttackColider)
            {
                CCollision_Manager::GetInstance()->Collect_Collider(CColider::TYPE_SPHERE, m_pAttackColider);
                m_pAttackColider = nullptr;
            }
        }
        if (nullptr != m_pAttackColider)
            m_pAttackColider->Update(m_ColliderMatrix);
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
    Safe_Release(m_pAttackColider);
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
