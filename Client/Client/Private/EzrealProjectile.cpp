
#include "EzrealProjectile.h"
#include "Enemy.h"
#include "Minion.h"
#include "Building.h"
#include "ParticleSystem.h"
#include "Collision_Manager.h"
#include "GameInstance.h"

CEzrealProjectile::CEzrealProjectile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CProjectile(pDevice, pContext)
{
}

CEzrealProjectile::CEzrealProjectile(const CEzrealProjectile& rhs)
    : CProjectile(rhs)
{
}

HRESULT CEzrealProjectile::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CEzrealProjectile::Initialize(void* pArg)
{
    __super::Initialize(pArg);

    // ? 풀 프리워밍이면 여기서 끝 (FX 생성 금지)
    if (m_ProjectileDesc.bPrewarm)
    {
        m_iRHitCount = 0;
        ZeroMemory(m_RHitList, sizeof(m_RHitList));
        return S_OK;
    }

    // ===== 아래는 "실사용"일 때만 =====
    m_vStartPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    m_vStartPos = XMVectorSetW(m_vStartPos, 1.f);

    switch (m_ProjectileDesc.eProjType)
    {
    case Q: m_fMaxDistance = 650.f; break;
    case W: m_fMaxDistance = 700.f; break;
    case R:
        m_fMaxDistance = -1.f;              // ? 사거리 제한 없음
        m_ProjectileDesc.fDeadTime = 20.f;  // ? 충분히 길게
        break;
    default: m_fMaxDistance = 0.f; break;
    }

    m_iRHitCount = 0;
    ZeroMemory(m_RHitList, sizeof(m_RHitList));

    _matrix projWorld = m_pTransformCom->Get_WorldMatrix();

    const _tchar* pFlyDat = TEXT("Ezreal_Base.dat");
    switch (m_ProjectileDesc.eProjType)
    {
    case BASE: pFlyDat = TEXT("Ezreal_Base.dat"); break;
    case Q:    pFlyDat = TEXT("Ezreal_Q.dat");    break;
    case W:    pFlyDat = TEXT("Ezreal_W.dat");    break;
    case R:    pFlyDat = TEXT("Ezreal_R.dat");          break;
    }

    m_pEffects = CEffect::PlayEffectAtLocation(pFlyDat, projWorld);
    for (auto& fx : m_pEffects)
        if (fx) fx->Set_AttachObject(this);

    return S_OK;
}

int CEzrealProjectile::Update(_float fTimeDelta)
{
    _int iSuperTick = __super::Update(fTimeDelta);

    if (iSuperTick == OBJ_POOL)
        return OBJ_POOL;  

    // 이동
    switch (m_ProjectileDesc.eProjType)
    {
    case BASE:
    case Q:
    case W:
    case R:
        Update_Projectile(fTimeDelta);
        break;
    }

    if (m_pAABBCom)   m_pAABBCom->Update(m_pTransformCom->Get_WorldMatrix());
    if (m_pOBBCom)    m_pOBBCom->Update(m_pTransformCom->Get_WorldMatrix());
    if (m_pSphereCom) m_pSphereCom->Update(m_pTransformCom->Get_WorldMatrix());

    return OBJ_NOEVENT;
}

void CEzrealProjectile::Late_Update(_float fTimeDelta)
{
    if (m_bInPool) return;   // ? 추가
    __super::Late_Update(fTimeDelta);

    for (auto& iter : m_pEffects)
        if (iter && iter->Get_PreDead())
            iter = nullptr;

    if (m_ProjectileDesc.eProjType != R)
    {
        if (m_fTime >= m_ProjectileDesc.fDeadTime)
            m_bDead = true;
    }
    else
    {
        // R은 DeadTime만으로 종료
        if (m_fTime >= m_ProjectileDesc.fDeadTime)
            m_bDead = true;
    }
}

bool CEzrealProjectile::HasHitTarget(CActor* pActor) const
{
    for (int i = 0; i < m_iRHitCount; ++i)
    {
        if (m_RHitList[i] == pActor)
            return true;
    }
    return false;
}

void CEzrealProjectile::RegisterHitTarget(CActor* pActor)
{
    if (m_iRHitCount >= MAX_R_HIT)
        return;

    m_RHitList[m_iRHitCount++] = pActor;
}

bool CEzrealProjectile::Is_RedTarget(CActor* pActor) const
{
    if (!pActor) return false;

    // Enemy는 팀 판정 없이 무조건 적 취급
    if (dynamic_cast<CEnemy*>(pActor))
        return true;

    const TEAMTYPE RED = TEAM_RED; // 네 enum에 맞춰

    if (auto pMinion = dynamic_cast<CMinion*>(pActor))
        return pMinion->Get_Team() == RED;

    if (auto pBuilding = dynamic_cast<CBuilding*>(pActor))
        return pBuilding->Get_TeamType() == RED;

    return false;
}

void CEzrealProjectile::Collision_Check()
{
    if (m_bDead)
        return;

    CActor* pHit = nullptr;

    // =========================
    // 1) BASE 타겟 추적
    // =========================
    if (m_ProjectileDesc.pTarget != nullptr)
    {
        if (m_ProjectileDesc.pTarget->Get_Dead())
        {
            m_bDead = true;
            Dead_Effect();
            return;
        }

        // ? Q는 건물 절대 맞으면 안 됨
        if (m_ProjectileDesc.eProjType == Q && dynamic_cast<CBuilding*>(m_ProjectileDesc.pTarget))
            return;

        // ? 레드 타겟만(Enemy는 무조건 true)
        if (!Is_RedTarget(m_ProjectileDesc.pTarget))
            return;

        CColider* pMyCol = m_pOBBCom ? (CColider*)m_pOBBCom : Get_Collider();
        CColider* pTargetCol = m_ProjectileDesc.pTarget->Get_OBBCollider();
        if (!pTargetCol) pTargetCol = m_ProjectileDesc.pTarget->Get_Collider();

        if (pMyCol && pTargetCol && pMyCol->Collision(pTargetCol))
        {
            // ? Q는 건물 데미지 금지(안전장치)
            if (!(m_ProjectileDesc.eProjType == Q && dynamic_cast<CBuilding*>(m_ProjectileDesc.pTarget)))
            {
                if (auto pEnemy = dynamic_cast<CEnemy*>(m_ProjectileDesc.pTarget))
                    pEnemy->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);
                else if (auto pMinion = dynamic_cast<CMinion*>(m_ProjectileDesc.pTarget))
                    pMinion->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);
                else if (auto pBuilding = dynamic_cast<CBuilding*>(m_ProjectileDesc.pTarget))
                    pBuilding->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);
            }

            Spawn_Effect(m_ProjectileDesc.pTarget);
            m_bDead = true;
            Dead_Effect();
        }
        return;
    }

    // =========================
    // 2) 직선탄(Q/W/R)
    // =========================

    // ? Q는 BUILDING 그룹을 아예 검사하지 않는다
    if (m_ProjectileDesc.eProjType == Q)
    {
        if (CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_ENEMY, m_pOBBCom, &pHit) ||
            CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_MINION, m_pOBBCom, &pHit))
        {
            // Enemy는 무조건 true / Minion은 RED만 true
            if (!Is_RedTarget(pHit))
                return;

            if (auto pEnemy = dynamic_cast<CEnemy*>(pHit))
                pEnemy->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);
            else if (auto pMinion = dynamic_cast<CMinion*>(pHit))
                pMinion->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);

            Spawn_Effect(pHit);
            m_bDead = true;
            Dead_Effect();
        }
        return;
    }

    if (m_ProjectileDesc.eProjType == W)
    {
        CActor* pHit = nullptr;

        if (CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_ENEMY, m_pOBBCom, &pHit) ||
            CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_BUILDING, m_pOBBCom, &pHit))
        {
            if (!Is_RedTarget(pHit))
                return;

            // 몸통 위치 느낌 기본값
            _float3 offsetEzW = { 0.f, 1.f, 0.f };

            pHit->Apply_Mark(
                MARK_TYPE::EZREAL_W,
                m_ProjectileDesc.pOwner,
                3.f,
                TEXT("Ezreal_WMark.dat"),
                offsetEzW
            );

            // W 투사체는 여기서 죽음
            Spawn_Effect(pHit);
            m_bDead = true;
            Dead_Effect();
        }
        return;
    }

    // =========================
// R : 관통 궁극기
// =========================
    if (m_ProjectileDesc.eProjType == R)
    {
        CActor* pHit = nullptr;

        if (m_ProjectileDesc.eProjType == R)
        {
            CActor* pHit = nullptr;

            if (CCollision_Manager::GetInstance()->CollisionwithGroup(
                CCollision_Manager::COLLISION_ENEMY, m_pOBBCom, &pHit) ||
                CCollision_Manager::GetInstance()->CollisionwithGroup(
                    CCollision_Manager::COLLISION_MINION, m_pOBBCom, &pHit))
            {
                if (!Is_RedTarget(pHit))
                    return;

                if (HasHitTarget(pHit))
                    return;

                RegisterHitTarget(pHit);

                if (auto pEnemy = dynamic_cast<CEnemy*>(pHit))
                    pEnemy->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);
                else if (auto pMinion = dynamic_cast<CMinion*>(pHit))
                    pMinion->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);

                Spawn_Effect(pHit);
            }

            return;
        }

        return;
    }

    // ? Q가 아닌 경우(W/R/BASE 직선 등)는 건물도 포함 가능
    if (CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_ENEMY, m_pOBBCom, &pHit) ||
        CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_MINION, m_pOBBCom, &pHit) ||
        CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_BUILDING, m_pOBBCom, &pHit))
    {
        if (!Is_RedTarget(pHit))
            return;

        if (auto pEnemy = dynamic_cast<CEnemy*>(pHit))
            pEnemy->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);
        else if (auto pMinion = dynamic_cast<CMinion*>(pHit))
            pMinion->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);
        else if (auto pBuilding = dynamic_cast<CBuilding*>(pHit))
            pBuilding->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);

        Spawn_Effect(pHit);
        m_bDead = true;
        Dead_Effect();
    }

   


}

void CEzrealProjectile::Dead_Effect()
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

void CEzrealProjectile::Spawn_Effect(CActor* pHitObj)
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
    switch (m_ProjectileDesc.eProjType)
    {
    case Q:  pHitDat = TEXT("Ezreal_Q_Hit.dat"); break;
    case R:  pHitDat = TEXT("Ezreal_R_Hit.dat"); break;
    case BASE: pHitDat = TEXT("Ezreal_Base_Hit"); break;
    default: pHitDat = TEXT("Ezreal_Base_HitD.dat"); break;
    }

    CEffect::PlayEffectAtLocation(pHitDat, mWorld);
}

HRESULT CEzrealProjectile::Ready_Components(void* pArg)
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

void CEzrealProjectile::Update_Projectile(_float fTimeDelta)
{
    _vector vMyPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);

    _vector vDir = XMVectorZero();

    // BASE만 추적, 나머지는 직선
    if (m_ProjectileDesc.eProjType == BASE && m_ProjectileDesc.pTarget && !m_ProjectileDesc.pTarget->Get_Dead())
    {
        // ? 단, 타겟이 레드가 아니면 추적하지 말고 그냥 직선으로 가게 하거나(원하면)
        // 여기서는 "타겟이 레드가 아니면 직선으로" 처리
        if (!Is_RedTarget(m_ProjectileDesc.pTarget))
            vDir = m_ProjectileDesc.vTargetDir;
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

    // 사거리 체크
    if (m_ProjectileDesc.eProjType != R && m_fMaxDistance > 0.f)
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

    // 회전
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

void CEzrealProjectile::ReUse_Setting(void* pArg)
{
    __super::ReUse_Setting(pArg);

    // 시작점/사거리
    m_vStartPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    m_vStartPos = XMVectorSetW(m_vStartPos, 1.f);

    switch (m_ProjectileDesc.eProjType)
    {
    case Q: m_fMaxDistance = 650.f; break;
    case W: m_fMaxDistance = 700.f; break;
    case R:
        m_fMaxDistance = -1.f;
        m_ProjectileDesc.fDeadTime = 20.f;
        break;
    default: m_fMaxDistance = 0.f; break;
    }

    // ? R 히트리스트 반드시 초기화 (재사용하면 안 맞는 버그/터지는 버그 방지)
    m_iRHitCount = 0;
    ZeroMemory(m_RHitList, sizeof(m_RHitList));

    // ? 날아가는 FX 재생 (풀에선 안 만들었고, 여기서 만든다)
    _matrix projWorld = m_pTransformCom->Get_WorldMatrix();

    const _tchar* pFlyDat = TEXT("Ezreal_Base.dat");
    switch (m_ProjectileDesc.eProjType)
    {
    case BASE: pFlyDat = TEXT("Ezreal_Base.dat"); break;
    case Q:    pFlyDat = TEXT("Ezreal_Q.dat");    break;
    case W:    pFlyDat = TEXT("Ezreal_W.dat");    break;
    case R:    pFlyDat = TEXT("Ezreal_R.dat");          break;
    }

    m_pEffects = CEffect::PlayEffectAtLocation(pFlyDat, projWorld);
    for (auto& fx : m_pEffects)
        if (fx) fx->Set_AttachObject(this);
}

CEzrealProjectile* CEzrealProjectile::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CEzrealProjectile* pInstance = new CEzrealProjectile(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create: CEzrealProjectile");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CEzrealProjectile::Clone(void* pArg)
{
    CEzrealProjectile* pInstance = new CEzrealProjectile(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Clone: CEzrealProjectile");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CEzrealProjectile::Free()
{
    __super::Free();
}
