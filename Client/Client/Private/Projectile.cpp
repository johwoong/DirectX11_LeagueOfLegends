#include "Projectile.h"
#include "Collision_Manager.h"
#include "Minion.h"
#include "Champion.h"

CProjectile::CProjectile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CActor(pDevice, pContext)
{
}

CProjectile::CProjectile(const CProjectile& rhs)
    : CActor(rhs)
{
}

HRESULT CProjectile::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CProjectile::Initialize(void* pArg)
{
    memcpy(&m_Desc, pArg, sizeof(PROJECTILEDESC));

    __super::Initialize(pArg);
    Ready_Components(pArg);

    Update_Collider_Size_ByType();

    m_pTransformCom->Set_State(
        CTransform::STATE_TRANSLATION,
        m_Desc.vStartPos
    );

    return S_OK;
}

HRESULT CProjectile::Ready_Components(void* pArg)
{
    // Transform
    CTransform::TRANSFORMDESC tDesc{};
    tDesc.fSpeedPerSec = m_Desc.fSpeed;
    tDesc.fRotationPerSec = XMConvertToRadians(360.f);

    Add_Components(TEXT("Com_Transform"), LEVEL_STATIC,
        TEXT("Prototype_Component_Transform"),
        (CComponent**)&m_pTransformCom, &tDesc);

    // Renderer
    Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC,
        TEXT("Prototype_Component_Renderer"),
        (CComponent**)&m_pRendererCom);

    // Collider
    CColider::COLLIDERDESC cDesc{};
    cDesc.vScale = _float3(1.f, 1.f, 1.f);

    Add_Components(TEXT("Com_SPHERE"), LEVEL_STATIC,
        TEXT("Prototype_Component_Colider_Sphere"),
        (CComponent**)&m_pSphereCom, &cDesc);

    return S_OK;
}

void CProjectile::Update_Collider_Size_ByType()
{
    switch (m_Desc.eType)
    {
    case PROJ_TARGET:  // 평타용
        m_pSphereCom->Set_Scale(_float3(0.003f, 0.003f, 0.003f));
        break;

    case PROJ_LINEAR:  // 이즈리얼 Q
        m_pSphereCom->Set_Sphere_Radius(1.f);
        m_pSphereCom->Set_Scale(_float3(0.01f, 0.01f, 0.4f));
        break;

    case PROJ_AREA:    // 궁극기 범위기qq
        m_pSphereCom->Set_Scale(_float3(1.f, 1.f, 1.f));
        break;
    }
}

int CProjectile::Update(_float fDeltaTime)
{
    if (m_bDead)
        return OBJ_DEAD;

    __super::Update(fDeltaTime);
    m_fAccTime += fDeltaTime;

    if (m_fAccTime >= m_Desc.fLifeTime)
        return OBJ_DEAD;

    switch (m_Desc.eType)
    {
    case PROJ_TARGET: Move_Target(fDeltaTime); break;
    case PROJ_LINEAR: Move_Linear(fDeltaTime); break;
    }

    Check_Hit();
    Update_Collider();

    return OBJ_NOEVENT;
}

void CProjectile::Move_Target(_float dt)
{
    if (m_Desc.pTarget == nullptr)
    {
        m_bDead = true;
        return;
    }

    _vector vMy = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    _vector vTar = ((CActor*)m_Desc.pTarget)->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

    vTar = XMVectorSetY(vTar, XMVectorGetY(vMy));

    _vector vDir = XMVector3Normalize(vTar - vMy);
    m_pTransformCom->Move_PosDir(dt, vDir, nullptr);
    m_pTransformCom->LookDir(vDir);
}

void CProjectile::Move_Linear(_float dt)
{
    _vector vDir = XMLoadFloat3(&m_Desc.vDir);
    m_pTransformCom->Move_PosDir(dt, vDir, nullptr);

    m_fTraveled += m_Desc.fSpeed * dt;
    if (m_fTraveled >= m_Desc.fMaxDistance)
        m_bDead = true;
}

void CProjectile::Check_Hit()
{
    if (!m_pSphereCom) return; CActor* pOwnerActor = dynamic_cast<CActor*>(m_Desc.pOwner);
    if (m_Desc.eType == PROJ_TARGET) 
    {
        CActor* pTargetActor = dynamic_cast<CActor*>(m_Desc.pTarget);
        if (!pTargetActor) return;
        if (pTargetActor->Get_Dead())
        { m_bDead = true; return; }
        CColider* pTargetCol = pTargetActor->Get_Collider();
        if (!pTargetCol) return;
        if (m_pSphereCom->Collision(pTargetCol))
        { 
            pTargetActor->Take_Damage(static_cast<int>(m_Desc.fDamage), pOwnerActor);
            m_bDead = true;
        }
        return;
    } // ================== 2) 직선 스킬샷 (Q / R) ==================
    if (m_Desc.eType == PROJ_LINEAR) 
    {
    
        auto& group = CCollision_Manager::GetInstance()->Get_CollisionGroup( CCollision_Manager::COLLISION_MINION);
        CChampion* pOwnerChamp = dynamic_cast<CChampion*>(pOwnerActor); 
        for (auto obj : group) 
        { 
            CActor* pTarget = dynamic_cast<CActor*>(obj);
            if (!pTarget || pTarget->Get_Dead())
                continue;
            if (CMinion* pMin = dynamic_cast<CMinion*>(pTarget)) 
            {
                if (pMin->Get_Team() == TEAM_BLUE) continue;
            }
            CColider* pTargetCol = pTarget->Get_Collider();
            if (!pTargetCol)
                continue;
            if (m_pSphereCom->Collision(pTargetCol))
            {
                pTarget->Take_Damage(static_cast<int>(m_Desc.fDamage), pOwnerActor);
            }
        }

    }

}

void CProjectile::Late_Update(_float dt)
{
    __super::Late_Update(dt);
#ifdef _DEBUG
    m_pRendererCom->Add_Debug(m_pSphereCom);
#endif
}

HRESULT CProjectile::Render()
{
    return S_OK;
}

CProjectile* CProjectile::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CProjectile* p = new CProjectile(pDevice, pContext);
    if (FAILED(p->Initialize_Prototype()))
    {
        Safe_Release(p);
        return nullptr;
    }
    return p;
}

CGameObject* CProjectile::Clone(void* pArg)
{
    CProjectile* p = new CProjectile(*this);
    if (FAILED(p->Initialize(pArg)))
    {
        Safe_Release(p);
        return nullptr;
    }
    return p;
}

void CProjectile::Free()
{
    __super::Free();
}