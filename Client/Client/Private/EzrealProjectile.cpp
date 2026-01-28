
#include "EzrealProjectile.h"
#include "Enemy.h"
#include "Minion.h"
#include "Building.h"
#include "ParticleSystem.h"
#include "Collision_Manager.h"
#include "GameInstance.h"

namespace
{
    // =========================
    // FX 월드행렬 생성:
    // - "앞방향이 +X(STATE_RIGHT)"인 메쉬 기준
    // - 발사 방향(vDir)을 +X축으로 맞춰준다.
    // =========================
    inline _matrix MakeFxWorld_DirXForward(_vector vPos, _vector vDir)
    {
        vDir = XMVectorSetY(vDir, 0.f);
        if (XMVectorGetX(XMVector3LengthSq(vDir)) < 1e-6f)
            vDir = XMVectorSet(1.f, 0.f, 0.f, 0.f);

        vDir = XMVector3Normalize(vDir);

        _vector vRight = vDir; // +X = 발사방향 (메쉬 "앞"이 +X라고 가정)
        _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);

        _vector vLook = XMVector3Cross(vUp, vRight);
        if (XMVectorGetX(XMVector3LengthSq(vLook)) < 1e-6f)
            vLook = XMVectorSet(0.f, 0.f, 1.f, 0.f);

        vLook = XMVector3Normalize(vLook);
        vUp = XMVector3Normalize(XMVector3Cross(vRight, vLook));

        _matrix m = XMMatrixIdentity();
        m.r[0] = vRight;                  // RIGHT
        m.r[1] = vUp;                     // UP
        m.r[2] = vLook;                   // LOOK
        m.r[3] = XMVectorSetW(vPos, 1.f); // POS
        return m;
    }

    // 만약 네 FX(Plane/Slash 등)가 "앞방향이 +Z(STATE_LOOK)"이면,
    // 위 함수 대신 아래처럼 쓰면 된다(스왑 버전).
    /*
    inline _matrix MakeFxWorld_DirZForward(_vector vPos, _vector vDir)
    {
        vDir = XMVectorSetY(vDir, 0.f);
        if (XMVectorGetX(XMVector3LengthSq(vDir)) < 1e-6f)
            vDir = XMVectorSet(0.f, 0.f, 1.f, 0.f);

        vDir = XMVector3Normalize(vDir);

        _vector vLook = vDir; // +Z = 발사방향
        _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);

        _vector vRight = XMVector3Cross(vUp, vLook);
        if (XMVectorGetX(XMVector3LengthSq(vRight)) < 1e-6f)
            vRight = XMVectorSet(1.f, 0.f, 0.f, 0.f);

        vRight = XMVector3Normalize(vRight);
        vUp = XMVector3Normalize(XMVector3Cross(vLook, vRight));

        _matrix m = XMMatrixIdentity();
        m.r[0] = vRight;
        m.r[1] = vUp;
        m.r[2] = vLook;
        m.r[3] = XMVectorSetW(vPos, 1.f);
        return m;
    }
    */
}

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
    ApplyInitialDirection();

    // 풀 프리워밍이면 여기서 끝 (FX 생성 금지)
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
        m_fMaxDistance = -1.f;              // 사거리 제한 없음
        m_ProjectileDesc.fDeadTime = 20.f;  // 충분히 길게
        break;
    default: m_fMaxDistance = 0.f; break;
    }

    m_iRHitCount = 0;
    ZeroMemory(m_RHitList, sizeof(m_RHitList));

    // ===============================
    // FX 생성: "발사 방향"으로 회전 맞춘 월드행렬을 넘긴다
    // ===============================
    _vector vPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    _matrix fxWorld = MakeFxWorld_DirXForward(vPos, m_ProjectileDesc.vTargetDir);
    // 만약 +Z forward 메쉬면 위 라인 대신:
    // _matrix fxWorld = MakeFxWorld_DirZForward(vPos, m_ProjectileDesc.vTargetDir);

    const _tchar* pFlyDat = TEXT("Ezreal_Base.dat");
    switch (m_ProjectileDesc.eProjType)
    {
    case BASE: pFlyDat = TEXT("Ezreal_Base.dat"); break;
    case Q:    pFlyDat = TEXT("Ezreal_Q.dat");    break;
    case W:    pFlyDat = TEXT("Ezreal_W.dat");    break;
    case R:    pFlyDat = TEXT("Ezreal_R.dat");    break;
    }

    m_pEffects = CEffect::PlayEffectAtLocation(pFlyDat, fxWorld);
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
    if (m_bInPool) return;
    __super::Late_Update(fTimeDelta);

    for (auto& iter : m_pEffects)
        if (iter && iter->Get_PreDead())
            iter = nullptr;

    // R은 DeadTime만으로 종료
    if (m_fTime >= m_ProjectileDesc.fDeadTime)
        m_bDead = true;
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

    const TEAMTYPE RED = TEAM_RED;

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

        // Q는 건물 절대 맞으면 안 됨
        if (m_ProjectileDesc.eProjType == Q && dynamic_cast<CBuilding*>(m_ProjectileDesc.pTarget))
            return;

        // 레드 타겟만(Enemy는 무조건 true)
        if (!Is_RedTarget(m_ProjectileDesc.pTarget))
            return;

        CColider* pMyCol = m_pOBBCom ? (CColider*)m_pOBBCom : Get_Collider();
        CColider* pTargetCol = m_ProjectileDesc.pTarget->Get_OBBCollider();
        if (!pTargetCol) pTargetCol = m_ProjectileDesc.pTarget->Get_Collider();

        if (pMyCol && pTargetCol && pMyCol->Collision(pTargetCol))
        {
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

    // Q는 BUILDING 그룹을 아예 검사하지 않는다
    if (m_ProjectileDesc.eProjType == Q)
    {
        if (CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_ENEMY, m_pOBBCom, &pHit) ||
            CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_MINION, m_pOBBCom, &pHit))
        {
            if (!Is_RedTarget(pHit))
                return;
            m_pGameInstance->PlaySounds(const_cast<TCHAR*>(L"EzrealQHitSound.wav"), SOUND_EFFECT, 0.5f);
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
        CActor* pHitW = nullptr;

        if (CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_ENEMY, m_pOBBCom, &pHitW) ||
            CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_BUILDING, m_pOBBCom, &pHitW))
        {
            if (!Is_RedTarget(pHitW))
                return;

            _float3 offsetEzW = { 0.f, 1.f, 0.f };

            pHitW->Apply_Mark(
                MARK_TYPE::EZREAL_W,
                m_ProjectileDesc.pOwner,
                3.f,
                TEXT("Ezreal_WMark.dat"),
                offsetEzW
            );

            Spawn_Effect(pHitW);
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
        CActor* pHitR = nullptr;

        if (CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_ENEMY, m_pOBBCom, &pHitR) ||
            CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_MINION, m_pOBBCom, &pHitR))
        {
            if (!Is_RedTarget(pHitR))
                return;

            if (HasHitTarget(pHitR))
                return;

            RegisterHitTarget(pHitR);

            if (auto pEnemy = dynamic_cast<CEnemy*>(pHitR))
                pEnemy->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);
            else if (auto pMinion = dynamic_cast<CMinion*>(pHitR))
                pMinion->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);

            Spawn_Effect(pHitR);
        }

        return;
    }

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
    case Q:    pHitDat = TEXT("Ezreal_Q_Hit.dat"); break;
    case R:    pHitDat = TEXT("Ezreal_R_Hit.dat"); break;
    case BASE: pHitDat = TEXT("Ezreal_Base_Hit");  break;
    default:   pHitDat = TEXT("Ezreal_Base_HitD.dat"); break;
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

    // 투사체 자체는 기존 로직 유지(너가 쓰던 Look기준)
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

    // ===============================
    // FX follow: "쏘는 방향" 기준 회전 포함해서 넘긴다
    // (툴 rotation은 이 기준(local)으로 먹는다)
    // ===============================
    _matrix fxOwner = MakeFxWorld_DirXForward(vNext, vDir);
    // +Z forward 메쉬면:
    // _matrix fxOwner = MakeFxWorld_DirZForward(vNext, vDir);

    for (auto& fx : m_pEffects)
        if (fx) fx->Set_WorldPosition(fxOwner);
}

void CEzrealProjectile::ReUse_Setting(void* pArg)
{
    __super::ReUse_Setting(pArg);

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

    m_iRHitCount = 0;
    ZeroMemory(m_RHitList, sizeof(m_RHitList));

    ApplyInitialDirection();

    // ===============================
    // FX 재생도 "발사 방향" 회전 포함해서 생성
    // ===============================
    _vector vPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    _matrix fxWorld = MakeFxWorld_DirXForward(vPos, m_ProjectileDesc.vTargetDir);
    // +Z forward 메쉬면:
    // _matrix fxWorld = MakeFxWorld_DirZForward(vPos, m_ProjectileDesc.vTargetDir);

    const _tchar* pFlyDat = TEXT("Ezreal_Base.dat");
    switch (m_ProjectileDesc.eProjType)
    {
    case BASE: pFlyDat = TEXT("Ezreal_Base.dat"); break;
    case Q:    pFlyDat = TEXT("Ezreal_Q.dat");    break;
    case W:    pFlyDat = TEXT("Ezreal_W.dat");    break;
    case R:    pFlyDat = TEXT("Ezreal_R.dat");    break;
    }

    m_pEffects = CEffect::PlayEffectAtLocation(pFlyDat, fxWorld);
    for (auto& fx : m_pEffects)
        if (fx) fx->Set_AttachObject(this);
}

void CEzrealProjectile::ApplyInitialDirection()
{
    _vector vLook = m_ProjectileDesc.vTargetDir;
    vLook = XMVectorSetY(vLook, 0.f);

    if (XMVectorGetX(XMVector3LengthSq(vLook)) < 1e-6f)
        vLook = XMVectorSet(0.f, 0.f, 1.f, 0.f);

    vLook = XMVector3Normalize(vLook);

    _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    _vector vRight = XMVector3Cross(vUp, vLook);

    if (XMVectorGetX(XMVector3LengthSq(vRight)) < 1e-6f)
        vRight = XMVectorSet(1.f, 0.f, 0.f, 0.f);

    vRight = XMVector3Normalize(vRight);
    vUp = XMVector3Normalize(XMVector3Cross(vLook, vRight));

    m_pTransformCom->Set_State(CTransform::STATE_RIGHT, vRight);
    m_pTransformCom->Set_State(CTransform::STATE_UP, vUp);
    m_pTransformCom->Set_State(CTransform::STATE_LOOK, vLook);
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


