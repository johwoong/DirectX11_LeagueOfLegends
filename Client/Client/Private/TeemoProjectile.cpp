#include "TeemoProjectile.h"

#include "Enemy.h"
#include "Minion.h"
#include "Building.h"
#include "ParticleSystem.h"
#include "Collision_Manager.h"
#include "GameInstance.h"
#include "Effect.h"              // ★ FX 필요

USING(Client)

CTemooProjectile::CTemooProjectile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CProjectile(pDevice, pContext)
{
}

CTemooProjectile::CTemooProjectile(const CTemooProjectile& rhs)
    : CProjectile(rhs)
{
}

HRESULT CTemooProjectile::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CTemooProjectile::Initialize(void* pArg)
{
    __super::Initialize(pArg);

    // ? 풀 프리워밍이면 FX 만들지 말고 끝
    if (m_ProjectileDesc.bPrewarm)
        return S_OK;

    // 시작점
    m_vStartPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    m_vStartPos = XMVectorSetW(m_vStartPos, 1.f);

    // 사거리 제한(기본 0=무제한)
    m_fMaxDistance = 0.f;

    // ? 타입별 날아가는 FX
    const _tchar* pFlyDat = nullptr;
    if (m_ProjectileDesc.eProjType == Q_BLIND)  pFlyDat = TEXT("Temoo_Q.dat");
    else                                       pFlyDat = TEXT("Temoo_Base.dat");

    _matrix projWorld = m_pTransformCom->Get_WorldMatrix();
    m_pEffects = CEffect::PlayEffectAtLocation(pFlyDat, projWorld);
    for (auto& fx : m_pEffects)
        if (fx) fx->Set_AttachObject(this);

    return S_OK;
}

int CTemooProjectile::Update(_float fTimeDelta)
{
    _int iSuperTick = __super::Update(fTimeDelta);
    if (iSuperTick == OBJ_POOL)
        return OBJ_POOL;

    // ? 이동/회전/FX follow
    Update_Projectile(fTimeDelta);

    // 콜라이더 갱신(OBB만 써도 되는데, 있으면 같이 갱신)
    if (m_pAABBCom)   m_pAABBCom->Update(m_pTransformCom->Get_WorldMatrix());
    if (m_pOBBCom)    m_pOBBCom->Update(m_pTransformCom->Get_WorldMatrix());
    if (m_pSphereCom) m_pSphereCom->Update(m_pTransformCom->Get_WorldMatrix());

    return OBJ_NOEVENT;
}

void CTemooProjectile::Late_Update(_float fTimeDelta)
{
    if (m_bInPool) return;
    __super::Late_Update(fTimeDelta);

    for (auto& iter : m_pEffects)
        if (iter && iter->Get_PreDead())
            iter = nullptr;

    // ? 수명 종료
    if (m_fTime >= m_ProjectileDesc.fDeadTime)
        m_bDead = true;
}

bool CTemooProjectile::Is_RedTarget(CActor* pActor) const
{
    if (!pActor) return false;

    // Enemy는 팀 판정 없이 무조건 적
    if (dynamic_cast<CEnemy*>(pActor))
        return true;

    const TEAMTYPE RED = TEAM_RED;

    if (auto pMinion = dynamic_cast<CMinion*>(pActor))
        return pMinion->Get_Team() == RED;

    if (auto pBuilding = dynamic_cast<CBuilding*>(pActor))
        return pBuilding->Get_TeamType() == RED;

    return false;
}

void CTemooProjectile::Collision_Check()
{
    if (m_bDead)
        return;

    // =========================
    // Teemo Poison (기본공격용)
    // =========================
    const float kPoisonDuration = 3.f;
    const float kPoisonTick = 1.f;
    const int   kPoisonDmgPerTick = 10;

    // =========================
    // Teemo Blind (Q용)
    // =========================
    const float kBlindDuration = 2.5f;

    auto ApplyOnHit = [&](CActor* pHit)
        {
            if (!pHit) return;

            // ? Q면 "Enemy에게만" 실명
            if (m_ProjectileDesc.eProjType == Q_BLIND)
            {
                if (auto pEnemy = dynamic_cast<CEnemy*>(pHit))
                    pEnemy->Apply_TeemoBlind(m_ProjectileDesc.pOwner, kBlindDuration);

                // Q는 여기서 독 안 바름(원하면 같이 바꿔도 됨)
                pHit->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);
                Spawn_Effect(pHit);
                m_bDead = true;
                Dead_Effect();
                return;
            }

            // ? BASE면 기존처럼 독(Enemy/Minion)
            if (auto pEnemy = dynamic_cast<CEnemy*>(pHit))
            {
                pEnemy->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);
                pEnemy->Apply_TeemoPoison(m_ProjectileDesc.pOwner, kPoisonDuration, kPoisonDmgPerTick, kPoisonTick);
            }
            else if (auto pMinion = dynamic_cast<CMinion*>(pHit))
            {
                pMinion->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);
                pMinion->Apply_TeemoPoison(m_ProjectileDesc.pOwner, kPoisonDuration, kPoisonDmgPerTick, kPoisonTick);
            }
            else if (auto pBuilding = dynamic_cast<CBuilding*>(pHit))
            {
                pBuilding->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);
            }

            Spawn_Effect(pHit);
            m_bDead = true;
            Dead_Effect();
        };

    // =========================
    // 1) 타겟 지정(추적형)
    // =========================
    if (m_ProjectileDesc.pTarget != nullptr)
    {
        if (m_ProjectileDesc.pTarget->Get_Dead())
        {
            m_bDead = true;
            Dead_Effect();
            return;
        }

        // 팀 필터(너가 쓰던 방식 유지)
        if (!Is_RedTarget(m_ProjectileDesc.pTarget))
            return;

        CColider* pMyCol = m_pOBBCom ? (CColider*)m_pOBBCom : Get_Collider();
        CColider* pTargetCol = m_ProjectileDesc.pTarget->Get_OBBCollider();
        if (!pTargetCol) pTargetCol = m_ProjectileDesc.pTarget->Get_Collider();

        if (pMyCol && pTargetCol && pMyCol->Collision(pTargetCol))
        {
            ApplyOnHit(m_ProjectileDesc.pTarget);
        }
        return;
    }

    // =========================
    // 2) 타겟 없을 때 그룹 충돌
    // =========================
    CActor* pHit = nullptr;

    if (CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_ENEMY, m_pOBBCom, &pHit) ||
        CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_MINION, m_pOBBCom, &pHit) ||
        CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_BUILDING, m_pOBBCom, &pHit))
    {
        if (!Is_RedTarget(pHit))
            return;

        ApplyOnHit(pHit);
    }
}

void CTemooProjectile::Dead_Effect()
{
    if (m_pEffects.empty())
        return;

    for (auto& fx : m_pEffects)
    {
        if (!fx) continue;

        if (auto ps = dynamic_cast<CParticleSystem*>(fx))
            ps->Set_Stop(true);

        fx->Set_Dead(true);
    }

    m_pEffects.clear();
}

void CTemooProjectile::Spawn_Effect(CActor* pHitObj)
{
    if (!pHitObj)
        return;

    _vector vPos = pHitObj->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
    vPos += XMVectorSet(0.f, 0.9f, 0.f, 0.f);
    vPos = XMVectorSetW(vPos, 1.f);

    _float4 camPos4 = CGameInstance::GetInstance()->Get_CamPosition();
    _vector vCamPos = XMLoadFloat4(&camPos4);

    _vector vToCam = vCamPos - vPos;
    vToCam = XMVectorSetY(vToCam, 0.f);
    if (XMVectorGetX(XMVector3LengthSq(vToCam)) < 1e-6f)
        vToCam = XMVectorSet(0.f, 0.f, 1.f, 0.f);

    _vector vLook = XMVector3Normalize(vToCam);

    const float fPushToCamera = 0.35f;
    vPos += vLook * fPushToCamera;
    vPos = XMVectorSetW(vPos, 1.f);

    _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    _vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vLook));
    vUp = XMVector3Normalize(XMVector3Cross(vLook, vRight));

    _matrix mWorld = XMMatrixIdentity();
    mWorld.r[0] = vRight;
    mWorld.r[1] = vUp;
    mWorld.r[2] = vLook;
    mWorld.r[3] = vPos;

    const _tchar* pHitDat = nullptr;
    if (m_ProjectileDesc.eProjType == Q_BLIND) pHitDat = TEXT("Temoo_Q_Hit.dat");
    else                                      pHitDat = TEXT("Temoo_Base_Hit.dat");

    CEffect::PlayEffectAtLocation(pHitDat, mWorld);
}

HRESULT CTemooProjectile::Ready_Components(void* pArg)
{
    if (FAILED(__super::Ready_Components(pArg)))
        return E_FAIL;

    CColider::COLLIDERDESC ColliderDesc{};
    ZeroMemory(&ColliderDesc, sizeof(ColliderDesc));

    ColliderDesc.vScale = _float3(1.f, 1.f, 1.f);
    ColliderDesc.vRotation = _float3(0.f, 0.f, 0.f);
    ColliderDesc.vPosition = _float3(0.f, 0.f, 0.f);

    if (FAILED(__super::Add_Components(TEXT("Com_OBB"), LEVEL_STATIC,
        TEXT("Prototype_Component_Colider_OBB"), (CComponent**)&m_pOBBCom, &ColliderDesc)))
        return E_FAIL;

    return S_OK;
}

void CTemooProjectile::Update_Projectile(_float fTimeDelta)
{
    _vector vMyPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    _vector vDir = XMVectorZero();

    // BASE는 타겟 추적(있고, 살아있으면)
    if (m_ProjectileDesc.eProjType == BASE && m_ProjectileDesc.pTarget && !m_ProjectileDesc.pTarget->Get_Dead())
    {
        if (!Is_RedTarget(m_ProjectileDesc.pTarget))
            vDir = m_ProjectileDesc.vTargetDir; // 레드 아니면 마지막 방향 유지
        else
        {
            _vector vTarPos = m_ProjectileDesc.pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
            vDir = vTarPos - vMyPos;
        }
    }
    else
    {
        vDir = m_ProjectileDesc.vTargetDir;
    }

    vDir = XMVectorSetY(vDir, 0.f);
    if (XMVectorGetX(XMVector3LengthSq(vDir)) < 1e-6f)
    {
        m_bDead = true;
        return;
    }
    vDir = XMVector3Normalize(vDir);

    // 이동
    _vector vNext = vMyPos + vDir * (m_ProjectileDesc.fVelocity * fTimeDelta);
    m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vNext);

    // (선택) 직선 사거리 체크 쓰고 싶으면 여기 켜면 됨
    if (m_fMaxDistance > 0.f)
    {
        _vector vDelta = vNext - m_vStartPos;
        vDelta = XMVectorSetY(vDelta, 0.f);
        float dist = XMVectorGetX(XMVector3Length(vDelta));
        if (dist >= m_fMaxDistance)
        {
            m_bDead = true;
            return;
        }
    }

    // 회전(앞방향 = vDir)
    _vector vLook = vDir;
    _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    _vector vRight = XMVector3Cross(vUp, vLook);
    if (XMVectorGetX(XMVector3LengthSq(vRight)) < 1e-6f)
        vRight = XMVectorSet(1.f, 0.f, 0.f, 0.f);

    vRight = XMVector3Normalize(vRight);
    vUp = XMVector3Normalize(XMVector3Cross(vLook, vRight));
    vLook = XMVector3Normalize(vLook);

    m_pTransformCom->Set_State(CTransform::STATE_RIGHT, vRight);
    m_pTransformCom->Set_State(CTransform::STATE_UP, vUp);
    m_pTransformCom->Set_State(CTransform::STATE_LOOK, vLook);

    // FX follow
    _matrix ownerWorld = m_pTransformCom->Get_WorldMatrix();
    for (auto& fx : m_pEffects)
        if (fx) fx->Set_WorldPosition(ownerWorld);
}

void CTemooProjectile::ReUse_Setting(void* pArg)
{
    __super::ReUse_Setting(pArg);

    Dead_Effect();

    m_vStartPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    m_vStartPos = XMVectorSetW(m_vStartPos, 1.f);

    const _tchar* pFlyDat = nullptr;
    if (m_ProjectileDesc.eProjType == Q_BLIND)  pFlyDat = TEXT("Temoo_Q.dat");
    else                                       pFlyDat = TEXT("Temoo_Base.dat");

    _matrix projWorld = m_pTransformCom->Get_WorldMatrix();
    m_pEffects = CEffect::PlayEffectAtLocation(pFlyDat, projWorld);
    for (auto& fx : m_pEffects)
        if (fx) fx->Set_AttachObject(this);
}

CTemooProjectile* CTemooProjectile::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CTemooProjectile* pInstance = new CTemooProjectile(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create: CTemooProjectile");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CTemooProjectile::Clone(void* pArg)
{
    CTemooProjectile* pInstance = new CTemooProjectile(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Clone: CTemooProjectile");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CTemooProjectile::Free()
{
    __super::Free();
}
