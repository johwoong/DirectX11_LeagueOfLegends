#include "LeeSinProjectile.h"
#include "LeeSin.h"
#include "GameInstance.h"
#include "Collision_Manager.h"
#include "Effect.h"

USING(Client)

CLeeSinProjectile::CLeeSinProjectile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CProjectile(pDevice, pContext)
{
}

CLeeSinProjectile::CLeeSinProjectile(const CLeeSinProjectile& rhs)
    : CProjectile(rhs)
{
}

HRESULT CLeeSinProjectile::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CLeeSinProjectile::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    // 풀 프리워밍이면 FX 생성 금지 (Ezreal과 동일)
    if (m_ProjectileDesc.bPrewarm)
    {
        m_bHit = false;
        return S_OK;
    }

    m_bHit = false;
    m_bDead = false;

    // ★ 사거리 세팅 안 하면 0이라 바로 죽을 수 있음
    // (너 프로젝트 단위에 맞춰 튜닝)
    m_fMaxDistance = 650.f;

    m_vStartPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    m_vStartPos = XMVectorSetW(m_vStartPos, 1.f);

    // 방향 정리
    _vector vDir = m_ProjectileDesc.vTargetDir;
    vDir = XMVectorSetY(vDir, 0.f);

    if (XMVectorGetX(XMVector3LengthSq(vDir)) < 1e-6f)
    {
        if (m_ProjectileDesc.pOwner)
            vDir = m_ProjectileDesc.pOwner->Get_Transform()->Get_State(CTransform::STATE_LOOK);
        vDir = XMVectorSetY(vDir, 0.f);
    }

    vDir = XMVector3Normalize(vDir);
    m_ProjectileDesc.vTargetDir = vDir;

    // 회전 정렬 (FX/메시 방향 맞추기)
    Align_ToDir(vDir);

    // ★ 날아가는 Q 이펙트 생성 + Attach
    Spawn_FlyEffect();

    return S_OK;
}

int CLeeSinProjectile::Update(_float fTimeDelta)
{
    const int iRet = __super::Update(fTimeDelta);
    if (iRet == OBJ_POOL)
        return OBJ_POOL;

    Update_Projectile(fTimeDelta);

    if (m_pOBBCom)
        m_pOBBCom->Update(m_pTransformCom->Get_WorldMatrix());

    return OBJ_NOEVENT;
}

void CLeeSinProjectile::Late_Update(_float fTimeDelta)
{
    if (m_bInPool) return; // Ezreal처럼

    __super::Late_Update(fTimeDelta);

    // FX가 PreDead 들어간 애들 정리
    for (auto& fx : m_pEffects)
        if (fx && fx->Get_PreDead())
            fx = nullptr;

    // DeadTime으로 죽고 싶으면 (원하면 유지)
    if (m_fTime >= m_ProjectileDesc.fDeadTime) { m_bDead = true; Dead_Effect(); }
}

void CLeeSinProjectile::Update_Projectile(_float dt)
{
    if (m_bDead || m_bHit)
        return;

    _vector vPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    _vector vDir = m_ProjectileDesc.vTargetDir;

    _vector vNext = vPos + vDir * (m_ProjectileDesc.fVelocity * dt);
    m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vNext);

    // 방향 정렬 유지
    Align_ToDir(vDir);

    // ★ FX follow (Ezreal과 동일)
    _matrix w = m_pTransformCom->Get_WorldMatrix();
    for (auto& fx : m_pEffects)
        if (fx) fx->Set_WorldPosition(w);

    // 사거리(XZ)
    _vector vDelta = XMVectorSetY(vNext - m_vStartPos, 0.f);
    if (m_fMaxDistance > 0.f &&
        XMVectorGetX(XMVector3Length(vDelta)) >= m_fMaxDistance)
    {
        m_bDead = true;
        Dead_Effect();
        return;
    }

    Collision_Check();
}

void CLeeSinProjectile::Collision_Check()
{
    if (m_bDead || m_bHit)
        return;

    CActor* pHit = nullptr;

    if (CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_ENEMY, m_pOBBCom, &pHit) ||
        CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_MINION, m_pOBBCom, &pHit) ||
        CCollision_Manager::GetInstance()->CollisionwithGroup(CCollision_Manager::COLLISION_BUILDING, m_pOBBCom, &pHit))
    {
        if (pHit)
        {
            pHit->Take_Damage(m_ProjectileDesc.iDamage, m_ProjectileDesc.pOwner);

            // 표식 (dat 이름은 네가 가진 걸로 바꿔)
            pHit->Apply_Mark(
                CActor::MARK_TYPE::LEE_SIN_Q,
                m_ProjectileDesc.pOwner,
                m_fMarkDuration,
                TEXT("LeeSinQ_Mark.dat"),
                _float3(0.f, 1.1f, 0.f));

            if (auto pLee = dynamic_cast<CLeeSin*>(m_ProjectileDesc.pOwner))
                pLee->Set_QMarkTarget(pHit);

            // ★ 히트 이펙트
            Spawn_HitEffect(pHit);
        }

        m_bHit = true;
        m_bDead = true;

        // ★ 날아가는 FX는 여기서 정리
        Dead_Effect();
    }
}

void CLeeSinProjectile::Spawn_FlyEffect()
{
    // 기존 FX 남아있으면 정리
    Dead_Effect();

    _matrix projWorld = m_pTransformCom->Get_WorldMatrix();

    // ★ 너가 가진 dat 이름으로 바꿔
    const _tchar* pFlyDat = TEXT("LeeSin_Q.dat");

    m_pEffects = CEffect::PlayEffectAtLocation(pFlyDat, projWorld);
    for (auto& fx : m_pEffects)
        if (fx) fx->Set_AttachObject(this);
}

void CLeeSinProjectile::Spawn_HitEffect(CActor* pHit)
{
    if (!pHit) return;

    // =========================
    // (A) 히트 이펙트 (기존: 카메라쪽 밀고 빌보드)
    // =========================
    _vector vPos = pHit->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
    vPos += XMVectorSet(0.f, 0.9f, 0.f, 0.f);
    vPos = XMVectorSetW(vPos, 1.f);

    _float4 camPos4 = CGameInstance::GetInstance()->Get_CamPosition();
    _vector vCamPos = XMLoadFloat4(&camPos4);

    _vector vToCam = vCamPos - vPos;
    vToCam = XMVectorSetY(vToCam, 0.f);
    if (XMVectorGetX(XMVector3LengthSq(vToCam)) < 1e-6f)
        vToCam = XMVectorSet(0.f, 0.f, 1.f, 0.f);

    _vector vLook = XMVector3Normalize(vToCam);

    const float fPushToCamera = 0.25f;
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

    const _tchar* pHitDat = TEXT("LeeSin_Q_Hit.dat");
    CEffect::PlayEffectAtLocation(pHitDat, mWorld);


}

void CLeeSinProjectile::Dead_Effect()
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

void CLeeSinProjectile::ReUse_Setting(void* pArg)
{
    __super::ReUse_Setting(pArg);

    // 풀에서 재사용될 때도 FX 다시 붙여야 함 (Ezreal과 동일)
    m_bHit = false;
    m_bDead = false;

    // ★ 사거리 반드시 다시 세팅
    m_fMaxDistance = 650.f;

    m_vStartPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    m_vStartPos = XMVectorSetW(m_vStartPos, 1.f);

    _vector vDir = m_ProjectileDesc.vTargetDir;
    vDir = XMVectorSetY(vDir, 0.f);
    if (XMVectorGetX(XMVector3LengthSq(vDir)) < 1e-6f)
    {
        if (m_ProjectileDesc.pOwner)
            vDir = m_ProjectileDesc.pOwner->Get_Transform()->Get_State(CTransform::STATE_LOOK);
        vDir = XMVectorSetY(vDir, 0.f);
    }
    vDir = XMVector3Normalize(vDir);
    m_ProjectileDesc.vTargetDir = vDir;

    Align_ToDir(vDir);

    // 프리워밍이면 FX 만들지 않음
    if (!m_ProjectileDesc.bPrewarm)
        Spawn_FlyEffect();
}

void CLeeSinProjectile::Align_ToDir(_vector vDir)
{
    if (!m_pTransformCom)
        return;

    vDir = XMVectorSetY(vDir, 0.f);
    if (XMVectorGetX(XMVector3LengthSq(vDir)) < 1e-6f)
        return;

    vDir = XMVector3Normalize(vDir);

    // 기존 스케일 유지
    _vector vOldRight = m_pTransformCom->Get_State(CTransform::STATE_RIGHT);
    float fScale = XMVectorGetX(XMVector3Length(vOldRight));
    if (fScale < 1e-6f) fScale = 1.f;

    _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    _vector vRight = XMVector3Cross(vUp, vDir);
    if (XMVectorGetX(XMVector3LengthSq(vRight)) < 1e-6f)
        vRight = XMVectorSet(1.f, 0.f, 0.f, 0.f);

    vRight = XMVector3Normalize(vRight);
    vUp = XMVector3Normalize(XMVector3Cross(vDir, vRight));

    vRight *= fScale;
    vUp *= fScale;
    vDir *= fScale;

    vRight = XMVectorSetW(vRight, 0.f);
    vUp = XMVectorSetW(vUp, 0.f);
    vDir = XMVectorSetW(vDir, 0.f);

    m_pTransformCom->Set_State(CTransform::STATE_RIGHT, vRight);
    m_pTransformCom->Set_State(CTransform::STATE_UP, vUp);
    m_pTransformCom->Set_State(CTransform::STATE_LOOK, vDir);
}

HRESULT CLeeSinProjectile::Ready_Components(void* pArg)
{
    if (FAILED(__super::Ready_Components(pArg)))
        return E_FAIL;

    // 너 엔진 콜라이더 세팅 방식대로 유지
    CColider::COLLIDERDESC desc{};
    desc.vScale = _float3(0.35f, 0.35f, 0.35f);
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

CLeeSinProjectile* CLeeSinProjectile::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CLeeSinProjectile* pInstance = new CLeeSinProjectile(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create LeeSinProjectile");
        Safe_Release(pInstance);
    }
    return pInstance;
}

CGameObject* CLeeSinProjectile::Clone(void* pArg)
{
    CLeeSinProjectile* pInstance = new CLeeSinProjectile(*this);
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Clone LeeSinProjectile");
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CLeeSinProjectile::Free()
{
    __super::Free();
}
