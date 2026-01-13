#include "MinionProjectile.h"
#include "GameInstance.h"
#include "Collision_Manager.h"
#include "ParticleSystem.h"
#include "Player.h"

USING(Client)

CMinionProjectile::CMinionProjectile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CProjectile(pDevice, pContext)
{
}

CMinionProjectile::CMinionProjectile(const CMinionProjectile& rhs)
    : CProjectile(rhs)
{
}

HRESULT CMinionProjectile::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CMinionProjectile::Initialize(void* pArg)
{
    __super::Initialize(pArg);

    CMinion* pOwnerMinion = dynamic_cast<CMinion*>(m_ProjectileDesc.pOwner);

    const _tchar* szProjectileFx =
        (pOwnerMinion && pOwnerMinion->Get_Team() == TEAM_BLUE)
        ? TEXT("Minion_Blue_Projectile.dat")
        : TEXT("Minion_Red_Projectile.dat");


    _matrix world = m_pTransformCom->Get_WorldMatrix();

    // ?? Projectile 본체 이펙트
    m_pEffects = CEffect::PlayEffectAtLocation(szProjectileFx, world);
    for (auto& fx : m_pEffects)
        if (fx)
            fx->Set_AttachObject(this);


    return S_OK;
}

int CMinionProjectile::Update(_float fTimeDelta)
{
    const int iRet = __super::Update(fTimeDelta);

    if (iRet == OBJ_POOL)
        return OBJ_POOL;   // ★ 여기서 끝내야 함

    Update_Projectile(fTimeDelta);

    if (m_pOBBCom)
        m_pOBBCom->Update(m_pTransformCom->Get_WorldMatrix());

    return OBJ_NOEVENT;
}

void CMinionProjectile::Late_Update(_float fTimeDelta)
{
    __super::Late_Update(fTimeDelta);

    for (auto& fx : m_pEffects)
        if (fx && fx->Get_PreDead())
            fx = nullptr;
}

void CMinionProjectile::Update_Projectile(_float fTimeDelta)
{
    CActor* pTarget = m_ProjectileDesc.pTarget;
    if (!pTarget || pTarget->Get_Dead())
    {
        m_bDead = true;
        Dead_Effect();
        return;
    }

    _vector vPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    _vector vTargetPos =
        pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

    _vector vDir = vTargetPos - vPos;
    vDir = XMVectorSetY(vDir, 0.f);

    if (XMVectorGetX(XMVector3LengthSq(vDir)) < 0.0001f)
        return;

    vDir = XMVector3Normalize(vDir);

    _vector vNext =
        vPos + vDir * (m_ProjectileDesc.fVelocity * fTimeDelta);

    m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vNext);

    // 회전
    _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    _vector vRight = XMVector3Cross(vUp, vDir);
    if (XMVectorGetX(XMVector3LengthSq(vRight)) < 1e-6f)
        vRight = XMVectorSet(1.f, 0.f, 0.f, 0.f);

    vRight = XMVector3Normalize(vRight);
    vUp = XMVector3Normalize(XMVector3Cross(vDir, vRight));

    m_pTransformCom->Set_State(CTransform::STATE_RIGHT, vRight);
    m_pTransformCom->Set_State(CTransform::STATE_UP, vUp);
    m_pTransformCom->Set_State(CTransform::STATE_LOOK, vDir);

    // FX follow
    _matrix ownerWorld = m_pTransformCom->Get_WorldMatrix();
    for (auto& fx : m_pEffects)
        if (fx)
            fx->Set_WorldPosition(ownerWorld);
}

void CMinionProjectile::Collision_Check()
{
     if (m_bDead)
        return;

    CActor* pTarget = m_ProjectileDesc.pTarget;
    if (!pTarget || pTarget->Get_Dead())
    {
        m_bDead = true;
        Dead_Effect();
        return;
    }

    CColider* pMyCol = m_pOBBCom;
    CColider* pTargetCol = pTarget->Get_Collider();
    if (!pTargetCol)
        return;

    if (pMyCol && pTargetCol && pMyCol->Collision(pTargetCol))
    {
        pTarget->Take_Damage(
            m_ProjectileDesc.iDamage,
            m_ProjectileDesc.pOwner);

        Spawn_HitEffect(pTarget);
        m_bDead = true;
        Dead_Effect();
    }
}

void CMinionProjectile::Spawn_HitEffect(CActor* pHitObj)
{
    if (!pHitObj)
        return;

    _vector vPos =
        pHitObj->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

    vPos += XMVectorSet(0.f, 0.9f, 0.f, 0.f);
    vPos = XMVectorSetW(vPos, 1.f);

    _matrix world = XMMatrixIdentity();
    world.r[3] = vPos;

    CEffect::PlayEffectAtLocation(
        TEXT("Minion_Range_Hit.dat"),
        world);
}

void CMinionProjectile::Dead_Effect()
{
    // Projectile 이펙트 종료
    for (auto& fx : m_pEffects)
    {
        if (!fx) continue;

        if (auto ps = dynamic_cast<CParticleSystem*>(fx))
            ps->Set_Stop(true);
    }
}

HRESULT CMinionProjectile::Ready_Components(void* pArg)
{
    if (FAILED(__super::Ready_Components(pArg)))
        return E_FAIL;

    CColider::COLLIDERDESC desc{};
    desc.vScale = _float3(0.4f, 0.4f, 0.4f);
    desc.vRotation = _float3(0.f, 0.f, 0.f);
    desc.vPosition = _float3(0.f, 0.f, 0.f);

    if (FAILED(__super::Add_Components(
        TEXT("Com_OBB"),
        LEVEL_STATIC,
        TEXT("Prototype_Component_Colider_OBB"),
        (CComponent**)&m_pOBBCom,
        &desc)))
        return E_FAIL;

    return S_OK;
}

void CMinionProjectile::ReUse_Setting(void* pArg)
{
    __super::ReUse_Setting(pArg);

    // ?? 기존 이펙트 제거
    for (auto& fx : m_pEffects)
        if (fx)
            fx->Set_Dead(true);

    m_pEffects.clear();

    // ?? 새 이펙트 생성
    CMinion* pOwnerMinion = dynamic_cast<CMinion*>(m_ProjectileDesc.pOwner);

    const _tchar* szProjectileFx =
        (pOwnerMinion && pOwnerMinion->Get_Team() == TEAM_BLUE)
        ? TEXT("Minion_Blue_Projectile.dat")
        : TEXT("Minion_Red_Projectile.dat");

    _matrix world = m_pTransformCom->Get_WorldMatrix();

    m_pEffects = CEffect::PlayEffectAtLocation(szProjectileFx, world);

    for (auto& fx : m_pEffects)
        if (fx)
            fx->Set_AttachObject(this);
}

CMinionProjectile* CMinionProjectile::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CMinionProjectile* pInstance = new CMinionProjectile(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create CMinionProjectile");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CMinionProjectile::Clone(void* pArg)
{
    CMinionProjectile* pInstance = new CMinionProjectile(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Clone CMinionProjectile");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CMinionProjectile::Free()
{
    __super::Free();
}
