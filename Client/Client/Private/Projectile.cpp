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

    // 1) 위치 설정
    m_pTransformCom->Set_State(
        CTransform::STATE_TRANSLATION,
        m_Desc.vStartPos
    );

    // 2) === 직선형 스킬이면 발사 방향으로 회전 세팅 ===
    if (m_Desc.eType == PROJ_LINEAR)
    {
        _vector vDir = XMLoadFloat3(&m_Desc.vDir);
        vDir = XMVector3Normalize(vDir);
        _vector vLook = vDir;         

        _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
        _vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vLook));
        vUp = XMVector3Cross(vLook, vRight);

        m_pTransformCom->Set_State(CTransform::STATE_RIGHT, vRight);
        m_pTransformCom->Set_State(CTransform::STATE_UP, vUp);
        m_pTransformCom->Set_State(CTransform::STATE_LOOK, vLook);
    }

    CCollision_Manager::GetInstance()->Add_CollisionGroup(CCollision_Manager::COLLISION_PROJECT, this);
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

    CColider::COLLIDERDESC cDesc{};

    // ====== 1) 평타(PROJ_TARGET) → Sphere ======
    if (m_Desc.eType == PROJ_TARGET)
    {
        // 기본 크기
        cDesc.vScale = _float3(0.004f, 0.004f, 0.004f);

        // 커스텀 콜라이더 쓰면 desc 값 덮어쓰기
        if (m_Desc.bUseCustomCollider)
        {
            if (m_Desc.vColliderScale.x != 0.f ||
                m_Desc.vColliderScale.y != 0.f ||
                m_Desc.vColliderScale.z != 0.f)
            {
                cDesc.vScale = m_Desc.vColliderScale;
            }
        }

        // ★ 회전 / 위치 오프셋 적용
        cDesc.vRotation = m_Desc.vColliderRotation;
        cDesc.vPosition = m_Desc.vColliderOffset;

        Add_Components(TEXT("Com_SPHERE"), LEVEL_STATIC,
            TEXT("Prototype_Component_Colider_Sphere"),
            (CComponent**)&m_pSphereCom, &cDesc);
    }

    // ====== 2) 직선 스킬(PROJ_LINEAR) → OBB ======
    if (m_Desc.eType == PROJ_LINEAR)
    {
        ZeroMemory(&cDesc, sizeof(CColider::COLLIDERDESC));

        // 크기
        if (m_Desc.bUseCustomCollider &&
            (m_Desc.vColliderScale.x != 0.f ||
                m_Desc.vColliderScale.y != 0.f ||
                m_Desc.vColliderScale.z != 0.f))
        {
            cDesc.vScale = m_Desc.vColliderScale;
        }
        else
        {
            cDesc.vScale = _float3(1.f, 1.f, 1.f);
        }

        // ★ 회전 / 위치 오프셋 적용
        cDesc.vRotation = m_Desc.vColliderRotation;  // 로컬 회전
        cDesc.vPosition = m_Desc.vColliderOffset;    // 로컬 위치

        if (FAILED(__super::Add_Components(
            TEXT("Com_OBB"), LEVEL_STATIC,
            TEXT("Prototype_Component_Colider_OBB"),
            (CComponent**)&m_pOBBCom, &cDesc)))
            return E_FAIL;
    }

    return S_OK;
}

void CProjectile::Update_Collider_Size_ByType()
{
    // 콜라이더 없으면 패스
    if (!m_pSphereCom && !m_pOBBCom)
        return;

    // ====== 평타 전용 커스텀 ======
    if (m_Desc.eType == PROJ_TARGET && m_pSphereCom)
    {
        if (!m_Desc.bUseCustomCollider)
            return;

        if (m_Desc.fColliderRadius > 0.f)
            m_pSphereCom->Set_Sphere_Radius(m_Desc.fColliderRadius);

        if (m_Desc.vColliderScale.x != 0.f ||
            m_Desc.vColliderScale.y != 0.f ||
            m_Desc.vColliderScale.z != 0.f)
        {
            m_pSphereCom->Set_Scale(m_Desc.vColliderScale);
        }
    }

    // PROJ_LINEAR는 Ready_Components에서 이미 크기 적용해버렸으니까 여기선 안 만져도 됨.
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

    CActor* pTargetActor = dynamic_cast<CActor*>(m_Desc.pTarget);
    if (!pTargetActor || pTargetActor->Get_Dead())
    {
        m_bDead = true;
        return;
    }

    _vector vMy = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    _vector vTar = pTargetActor->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

    // 수평만 따라가고 싶으면 Y를 고정
    vTar = XMVectorSetY(vTar, XMVectorGetY(vMy));

    _vector vToTar = vTar - vMy;
    float   fDist = XMVectorGetX(XMVector3Length(vToTar));
    if (fDist < 0.0001f)
        return;

    _vector vDir = XMVector3Normalize(vToTar);
    float   fStep = m_Desc.fSpeed * dt;

    // 한 프레임에 타겟을 넘어가려 하면 정확히 타겟 위치까지 이동
    if (fStep >= fDist)
    {
        m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vTar);
    }
    else
    {
        m_pTransformCom->Move_PosDir(dt, vDir, nullptr);
    }

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
    CActor* pOwnerActor = dynamic_cast<CActor*>(m_Desc.pOwner);
    if (!pOwnerActor)
        return;

    // ================== 1) 타워/미니언 평타용 추적 투사체 ==================
    if (m_Desc.eType == PROJ_TARGET)
    {
        CActor* pTargetActor = dynamic_cast<CActor*>(m_Desc.pTarget);
        if (!pTargetActor || pTargetActor->Get_Dead())
        {
            m_bDead = true;
            return;
        }

        // 1) 위치 가져오기
        _vector vMy = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
        _vector vTar = pTargetActor->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

        // Y 높이 차이 때문에 못 맞는 경우 방지 → XZ 평면 기준 거리만 사용
        vMy = XMVectorSetY(vMy, 0.f);
        vTar = XMVectorSetY(vTar, 0.f);

        float fDist = XMVectorGetX(XMVector3Length(vTar - vMy));

        // 2) 히트 거리(튜닝용 상수) ? 유닛 스케일에 맞게 살짝 넉넉하게
        const float fHitRadius = 0.5f; // 필요하면 0.4~0.7 사이로 조정

        if (fDist <= fHitRadius)
        {
            pTargetActor->Take_Damage(static_cast<int>(m_Desc.fDamage), pOwnerActor);
            m_bDead = true;
        }

        // PROJ_TARGET 은 여기서 끝
        return;
    }

    if (m_Desc.eType == PROJ_LINEAR)
    {
        CActor* pTargetActor = nullptr;
        bool bHit = false;

        switch (m_Desc.eSkillTag)
        {
        case SKILL_EZREAL_Q:
        {
            // 1) Enemy 먼저 체크
            if (CCollision_Manager::GetInstance()->CollisionwithGroup(
                CCollision_Manager::COLLISION_ENEMY,
                m_pOBBCom,
                &pTargetActor))
            {
                bHit = true;
            }

            // 2) 아직 안 맞았으면 Minion 그룹도 체크
            if (!bHit && CCollision_Manager::GetInstance()->CollisionwithGroup(
                CCollision_Manager::COLLISION_MINION,
                m_pOBBCom,
                &pTargetActor))
            {
                if (dynamic_cast<CMinion*>(pTargetActor)->Get_Team() == TEAM_BLUE)
                    return;

                bHit = true;
            }

            if (bHit && pTargetActor)
            {
                // 혹시라도 자기 자신이면 무시
                if (pTargetActor == pOwnerActor)
                    return;

                pTargetActor->Take_Damage(m_Desc.fDamage, m_Desc.pOwner);
                m_bDead = true;
            }

            break;
        }
        case SKILL_EZREAL_W:
            // 1) Enemy 먼저 체크
            if (CCollision_Manager::GetInstance()->CollisionwithGroup(
                CCollision_Manager::COLLISION_ENEMY,
                m_pOBBCom,
                &pTargetActor))
            {
                bHit = true;
            }
            if (bHit && pTargetActor)
            {
                // 혹시라도 자기 자신이면 무시
                if (pTargetActor == pOwnerActor)
                    return;
                m_bDead = true;
            }
            break;

        case SKILL_EZREAL_R:
        {
            CCollision_Manager* pCM = CCollision_Manager::GetInstance();

            std::vector<CActor*> vHits;
            std::vector<CActor*> vTemp;

            // Enemy
            if (pCM->CollisionwithGroup_All(
                CCollision_Manager::COLLISION_ENEMY,
                m_pOBBCom,
                &vTemp))
            {
                vHits.insert(vHits.end(), vTemp.begin(), vTemp.end());
            }

            // Minion
            vTemp.clear();
            if (pCM->CollisionwithGroup_All(
                CCollision_Manager::COLLISION_MINION,
                m_pOBBCom,
                &vTemp))
            {
                vHits.insert(vHits.end(), vTemp.begin(), vTemp.end());
            }

            // 맞은 애들 전부 처리
            for (CActor* pActor : vHits)
            {
                if (!pActor) continue;
                if (pActor == pOwnerActor) continue; // 자기 자신 제외

                // 이미 맞은 애면 또 때리지 않기
                if (std::find(m_HitActors.begin(), m_HitActors.end(), pActor) != m_HitActors.end())
                    continue;

                // 아군 미니언이면 제외 (너 팀 기준 맞게 수정)
                if (auto pMinion = dynamic_cast<CMinion*>(pActor))
                {
                    if (pMinion->Get_Team() == TEAM_BLUE)
                        continue;
                }

                // 실제 데미지
                pActor->Take_Damage(m_Desc.fDamage, m_Desc.pOwner);

                // 맞았다고 기록
                m_HitActors.push_back(pActor);
            }

            break;
        }
        default:
            break;
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
