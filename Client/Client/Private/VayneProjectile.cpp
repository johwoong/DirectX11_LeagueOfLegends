#include "VayneProjectile.h"

#include "Player.h"
#include "Minion.h"
#include "Building.h"
#include "ParticleSystem.h"
#include "Collision_Manager.h"
#include "GameInstance.h"

USING(Client);

CVayneProjectile::CVayneProjectile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CProjectile(pDevice, pContext)
{
}

CVayneProjectile::CVayneProjectile(const CVayneProjectile& rhs)
    : CProjectile(rhs)
{
}

HRESULT CVayneProjectile::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CVayneProjectile::Initialize(void* pArg)
{
    __super::Initialize(pArg);

    m_bHitOnce = false;   // ? 필수

    if (m_ProjectileDesc.bPrewarm)
        return S_OK;

    // =========================
    // 시작 위치
    // =========================
    m_vStartPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    m_vStartPos = XMVectorSetW(m_vStartPos, 1.f);

    // =========================
    // 타입별 수치 세팅
    // =========================
    switch (m_ProjectileDesc.eProjType)
    {
    case E_CONDEMN:
        m_fMaxDistance = 5.0f;
        m_ProjectileDesc.fVelocity = 45.f;
        break;

    default:
        m_fMaxDistance = 8.f;
        break;
    }

    // =========================
    // ? 타입별 비행 이펙트 선택
    // =========================
    const _tchar* pFlyEffect = TEXT("Vayne_Base.dat");

    if (m_ProjectileDesc.eProjType == E_CONDEMN)
        pFlyEffect = TEXT("Vayne_E.dat");

    // =========================
    // 이펙트 생성 + Attach
    // =========================
    _matrix projWorld = m_pTransformCom->Get_WorldMatrix();
    m_pEffects = CEffect::PlayEffectAtLocation(pFlyEffect, projWorld);

    for (auto& fx : m_pEffects)
    {
        if (fx)
            fx->Set_AttachObject(this);
    }

    return S_OK;
}

int CVayneProjectile::Update(_float fTimeDelta)
{
    _int iSuperTick = __super::Update(fTimeDelta);
    if (iSuperTick == OBJ_POOL)
        return OBJ_POOL;

    Update_Projectile(fTimeDelta);

    Collision_Check();   // ?? 이 줄이 핵심

    if (m_pAABBCom)   m_pAABBCom->Update(m_pTransformCom->Get_WorldMatrix());
    if (m_pOBBCom)    m_pOBBCom->Update(m_pTransformCom->Get_WorldMatrix());
    if (m_pSphereCom) m_pSphereCom->Update(m_pTransformCom->Get_WorldMatrix());

    return OBJ_NOEVENT;
}

void CVayneProjectile::Late_Update(_float fTimeDelta)
{
    if (m_bInPool) return;
    __super::Late_Update(fTimeDelta);

    for (auto& fx : m_pEffects)
        if (fx && fx->Get_PreDead())
            fx = nullptr;

    if (m_fTime >= m_ProjectileDesc.fDeadTime)
    {
        m_bDead = true;
        Dead_Effect();
    }
}

void CVayneProjectile::Update_Projectile(_float fTimeDelta)
{
    // 1) 현재 위치
    _vector vPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);

    // 2) 방향 (XZ)
    _vector vDir = XMVectorSetY(m_ProjectileDesc.vTargetDir, 0.f);
    if (XMVectorGetX(XMVector3LengthSq(vDir)) < 1e-6f)
    {
        m_bDead = true;
        Dead_Effect();
        return;
    }
    vDir = XMVector3Normalize(vDir);

    // 3) 이동
    _vector vNext = vPos + vDir * (m_ProjectileDesc.fVelocity * fTimeDelta);
    m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vNext);

    // 4) 사거리 체크
    _vector vDelta = XMVectorSetY(vNext - m_vStartPos, 0.f);
    if (XMVectorGetX(XMVector3Length(vDelta)) >= m_fMaxDistance)
    {
        m_bDead = true;
        Dead_Effect();
        return;
    }

    // =====================================================
    // ? 핵심: Projectile 자체를 "타겟 방향으로 LookAt"
    // Look 축 = +Z
    // =====================================================
    _vector vLookTarget = vNext + vDir;
    m_pTransformCom->LookAt(vLookTarget);

    // =====================================================
    // ? FX는 Projectile World 그대로 따라감
    // (툴 회전값만 신뢰)
    // =====================================================
    _matrix projWorld = m_pTransformCom->Get_WorldMatrix();

    for (auto& fx : m_pEffects)
    {
        if (fx)
            fx->Set_WorldPosition(projWorld);
    }
}

bool CVayneProjectile::Is_BlueTarget(CActor* pActor) const
{
    if (!pActor) return false;

    // ? 플레이어
    if (dynamic_cast<CPlayer*>(pActor))
        return true;

    const TEAMTYPE BLUE = TEAM_BLUE;

    if (auto pMinion = dynamic_cast<CMinion*>(pActor))
        return pMinion->Get_Team() == BLUE;

    if (auto pBuilding = dynamic_cast<CBuilding*>(pActor))
        return pBuilding->Get_TeamType() == BLUE;

    return false;
}

void CVayneProjectile::Collision_Check()
{
        // 이미 처리된 투사체는 다시 처리 X
    if (m_bDead || m_bHitOnce)
        return;

    CActor* pHit = nullptr;

    if (!(CCollision_Manager::GetInstance()->CollisionwithGroup(
        CCollision_Manager::COLLISION_PLAYER, m_pOBBCom, &pHit) ||
        CCollision_Manager::GetInstance()->CollisionwithGroup(
            CCollision_Manager::COLLISION_MINION, m_pOBBCom, &pHit)))
        return;

    // ? 여기서 바로 1회 확정
    m_bHitOnce = true;

    // 블루팀 아니면 연출만 하고 종료
    if (!Is_BlueTarget(pHit))
    {
        Spawn_Effect(pHit);
        m_bDead = true;
        Dead_Effect();
        return;
    }

    // =================================================
    // ? 공통: 데미지 + W 스택 (평타 / Q / E 전부)
    // =================================================
    pHit->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);
    pHit->Apply_VayneSilverMark(m_ProjectileDesc.pOwner);

    // =================================================
    // ? E 전용 처리
    // =================================================
    if (m_ProjectileDesc.eProjType == E_CONDEMN)
    {
        CTransform* pTr = pHit->Get_Transform();
        if (pTr)
        {
            _vector vDir = XMVectorSetY(m_ProjectileDesc.vTargetDir, 0.f);

            if (XMVectorGetX(XMVector3LengthSq(vDir)) > 0.0001f)
            {
                vDir = XMVector3Normalize(vDir);

                _vector hitPos =
                    pTr->Get_State(CTransform::STATE_TRANSLATION);
                hitPos += vDir * 3.5f;

                pTr->Set_State(
                    CTransform::STATE_TRANSLATION,
                    XMVectorSetW(hitPos, 1.f));
            }
        }
    }

    // =================================================
    // ? 히트 FX
    // =================================================
    Spawn_Effect(pHit);

    // =================================================
    // ? 종료
    // =================================================
    m_bDead = true;
    Dead_Effect();
}

void CVayneProjectile::Dead_Effect()
{
    for (auto& fx : m_pEffects)
    {
        if (!fx) continue;

        if (auto ps = dynamic_cast<CParticleSystem*>(fx))
            ps->Set_Stop(true);

        fx->Set_Dead(true);
    }
    m_pEffects.clear();
}

void CVayneProjectile::Spawn_Effect(CActor* pHitObj)
{
    _vector vPos = pHitObj->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
    vPos += XMVectorSet(0.f, 0.9f, 0.f, 0.f);
    vPos = XMVectorSetW(vPos, 1.f);

    _matrix mWorld = XMMatrixTranslationFromVector(vPos);
    CEffect::PlayEffectAtLocation(TEXT("Vayne_Base_Hit.dat"), mWorld);
}

HRESULT CVayneProjectile::Ready_Components(void* pArg)
{
    if (FAILED(__super::Ready_Components(pArg)))
        return E_FAIL;

    CColider::COLLIDERDESC Desc{};
    Desc.vScale = _float3(0.6f, 0.6f, 1.2f);

    return __super::Add_Components(
        TEXT("Com_OBB"),
        LEVEL_STATIC,
        TEXT("Prototype_Component_Colider_OBB"),
        (CComponent**)&m_pOBBCom,
        &Desc);
}

void CVayneProjectile::ReUse_Setting(void* pArg)
{
    __super::ReUse_Setting(pArg);

    m_bHitOnce = false;   // ? 필수

    m_vStartPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    m_vStartPos = XMVectorSetW(m_vStartPos, 1.f);

    if (m_ProjectileDesc.eProjType == E_CONDEMN)
        m_fMaxDistance = 5.f;
    else
        m_fMaxDistance = 8.f;

    _matrix projWorld = m_pTransformCom->Get_WorldMatrix();
    m_pEffects = CEffect::PlayEffectAtLocation(TEXT("Vayne_Base.dat"), projWorld);

    for (auto& fx : m_pEffects)
        if (fx) fx->Set_AttachObject(this);
}

CVayneProjectile* CVayneProjectile::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CVayneProjectile* pInstance = new CVayneProjectile(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create: CVayneProjectile");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CVayneProjectile::Clone(void* pArg)
{
    CVayneProjectile* pInstance = new CVayneProjectile(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Clone: CVayneProjectile");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CVayneProjectile::Free()
{
    __super::Free();
}
