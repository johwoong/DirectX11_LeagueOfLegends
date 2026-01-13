#include "TurretProjectile.h"
#include "GameInstance.h"
#include "Collision_Manager.h"
#include "Player.h"

USING(Client)

CTurretProjectile::CTurretProjectile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CProjectile(pDevice, pContext)
{
}

CTurretProjectile::CTurretProjectile(const CTurretProjectile& rhs)
    : CProjectile(rhs)
{
}

HRESULT CTurretProjectile::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CTurretProjectile::Initialize(void* pArg)
{
    __super::Initialize(pArg);

    TEAMTYPE eTeam = TEAM_RED; // 기본값

    // 1) 포탑/빌딩이면 여기서 팀 결정
    if (auto pB = dynamic_cast<CBuilding*>(m_ProjectileDesc.pOwner))
        eTeam = (TEAMTYPE)pB->Get_TeamType();

    // 2) 혹시 미니언이 쏘는 경우도 있으면
    else if (auto pM = dynamic_cast<CMinion*>(m_ProjectileDesc.pOwner))
        eTeam = pM->Get_Team();

    const _tchar* szProjectileFx =
        (eTeam == TEAM_BLUE) ? TEXT("Turret_Blue.dat") : TEXT("Turret_Red.dat");

    _matrix world = m_pTransformCom->Get_WorldMatrix();

    m_pEffects = CEffect::PlayEffectAtLocation(szProjectileFx, world);
    for (auto& fx : m_pEffects)
        if (fx)
            fx->Set_AttachObject(this);

    return S_OK;
}

int CTurretProjectile::Update(_float fTimeDelta)
{
    const int iRet = __super::Update(fTimeDelta);

    if (iRet == OBJ_POOL)
        return OBJ_POOL; 

    Update_Projectile(fTimeDelta);

    if (m_pOBBCom)
        m_pOBBCom->Update(m_pTransformCom->Get_WorldMatrix());

    return OBJ_NOEVENT;
}

void CTurretProjectile::Late_Update(_float fTimeDelta)
{
    __super::Late_Update(fTimeDelta);

    for (auto& fx : m_pEffects)
        if (fx && fx->Get_PreDead())
            fx = nullptr;
}

void CTurretProjectile::Update_Projectile(_float fTimeDelta)
{
    CActor* pTarget = m_ProjectileDesc.pTarget;
    if (!pTarget || pTarget->Get_Dead())
        return;

    _vector vPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    _vector vTargetPos = pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

    // ? 1) 이동은 Y 포함 (충돌되게)
    _vector vDelta = vTargetPos - vPos;
    _float  distSq = XMVectorGetX(XMVector3LengthSq(vDelta));
    if (distSq < 1e-6f)
        return;

    _vector vMoveDir = XMVector3Normalize(vDelta);

    _float fStep = m_ProjectileDesc.fVelocity * fTimeDelta;
    _vector vNext;

    // 너무 가까우면 스냅(관통/미스 줄이기)
    if (distSq <= fStep * fStep)
        vNext = XMVectorSetW(vTargetPos, 1.f);
    else
        vNext = XMVectorSetW(vPos + vMoveDir * fStep, 1.f);

    m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vNext);

    // ? 2) 회전은 수평(XZ)만: 이펙트가 위아래로 꺾여서 안보이는 문제 방지
    _vector vLookDir = XMVectorSetY(vMoveDir, 0.f);
    if (XMVectorGetX(XMVector3LengthSq(vLookDir)) < 1e-6f)
        vLookDir = XMVectorSet(0.f, 0.f, 1.f, 0.f);
    vLookDir = XMVector3Normalize(vLookDir);

    _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    _vector vRight = XMVector3Cross(vUp, vLookDir);
    if (XMVectorGetX(XMVector3LengthSq(vRight)) < 1e-6f)
        vRight = XMVectorSet(1.f, 0.f, 0.f, 0.f);

    vRight = XMVector3Normalize(vRight);
    vUp = XMVector3Normalize(XMVector3Cross(vLookDir, vRight));

    m_pTransformCom->Set_State(CTransform::STATE_RIGHT, vRight);
    m_pTransformCom->Set_State(CTransform::STATE_UP, vUp);
    m_pTransformCom->Set_State(CTransform::STATE_LOOK, vLookDir);

    // FX follow (그대로 유지)
    _matrix ownerWorld = m_pTransformCom->Get_WorldMatrix();
    for (auto& fx : m_pEffects)
        if (fx)
            fx->Set_WorldPosition(ownerWorld);
}

void CTurretProjectile::Collision_Check()
{
    if (m_bDead)
        return;

    CActor* pTarget = m_ProjectileDesc.pTarget;
    if (!pTarget || pTarget->Get_Dead())
        return;

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

void CTurretProjectile::Spawn_HitEffect(CActor* pHitObj)
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

void CTurretProjectile::Dead_Effect()
{
    // Projectile 이펙트 종료
    for (auto& fx : m_pEffects)
    {
        if (!fx) continue;

        if (auto ps = dynamic_cast<CParticleSystem*>(fx))
            ps->Set_Stop(true);

        fx->Set_Dead(true);
    }
    m_pEffects.clear();
}

HRESULT CTurretProjectile::Ready_Components(void* pArg)
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

void CTurretProjectile::ReUse_Setting(void* pArg)
{
    // 1) 부모 재사용 처리 (ProjectileDesc 갱신, 위치 초기화 등)
    __super::ReUse_Setting(pArg);

    // 2) 혹시 남아있는 이펙트 정리 (안전)
    for (auto& fx : m_pEffects)
    {
        if (!fx) continue;
        fx->Set_Dead(true);
    }
    m_pEffects.clear();

    // 3) 팀 결정 (Initialize와 동일 로직)
    TEAMTYPE eTeam = TEAM_RED;

    if (auto pB = dynamic_cast<CBuilding*>(m_ProjectileDesc.pOwner))
        eTeam = (TEAMTYPE)pB->Get_TeamType();
    else if (auto pM = dynamic_cast<CMinion*>(m_ProjectileDesc.pOwner))
        eTeam = pM->Get_Team();

    const _tchar* szProjectileFx =
        (eTeam == TEAM_BLUE) ? TEXT("Turret_Blue.dat") : TEXT("Turret_Red.dat");

    // 4) 현재 위치 기준으로 이펙트 다시 생성
    _matrix world = m_pTransformCom->Get_WorldMatrix();

    m_pEffects = CEffect::PlayEffectAtLocation(szProjectileFx, world);

    for (auto& fx : m_pEffects)
        if (fx)
            fx->Set_AttachObject(this);
}

CTurretProjectile* CTurretProjectile::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CTurretProjectile* pInstance = new CTurretProjectile(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create CTurretProjectile");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CTurretProjectile::Clone(void* pArg)
{
    CTurretProjectile* pInstance = new CTurretProjectile(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Clone CMinionProjectile");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CTurretProjectile::Free()
{
    __super::Free();
}
