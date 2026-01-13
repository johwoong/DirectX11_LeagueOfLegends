#include "LeeSinSkillStateR.h"

#include "LeeSin.h"
#include "Enemy.h"              // ★ CEnemy 캐스팅용
#include "PickingManager.h"     // ★ 마우스 Hover 타겟팅
#include "PlayerIdleState.h"
#include "Collision_Manager.h"
#include "Effect.h"
#include "CColider.h"

using namespace Player;

CLeeSinSkillStateR::CLeeSinSkillStateR(CPlayer* pPlayer)
{
    m_pOwner = pPlayer;
    if (m_pOwner)
        m_ePlayerID = m_pOwner->Get_PlayerID();
}

CPlayerState* CLeeSinSkillStateR::HandleInput() { return nullptr; }

void CLeeSinSkillStateR::Enter()
{
    __super::Enter();

    m_pLeeSin = dynamic_cast<CLeeSin*>(m_pOwner);
    if (!m_pLeeSin)
        return;

    m_fAcc = 0.f;
    m_bDidHit = false;
    m_bAnimFinished = false;
    m_bColliderEventPlaying = false;

    m_bKnockActive = false;
    m_fKnockAcc = 0.f;
    m_vKnockDir = XMVectorSet(0.f, 0.f, 1.f, 0.f);

    Destroy_RCollider();

    // =========================================================
    // ★ R 타겟은 "적 챔피언(CEnemy)"만
    // 1) 기존 AttackTarget 우선
    // 2) 아니면 마우스 Hover(피킹된 오브젝트)에서 적 챔 찾기
    // =========================================================
    m_pTarget = nullptr;

    // 1) AttackTarget
    if (CActor* pAT = m_pLeeSin->Get_AttackTarget())
    {
        if (auto pEnemyChamp = dynamic_cast<CEnemy*>(pAT))
            m_pTarget = pEnemyChamp;
    }

    // 2) 마우스 Hover 타겟 (적 챔피언일 때만)
    if (!m_pTarget)
    {
        CPickingManager* PM = CPickingManager::GetInstance();
        if (PM)
        {
            if (CGameObject* picked = PM->Get_PickedObj())
            {
                if (auto pEnemyChamp = dynamic_cast<CEnemy*>(picked))
                {
                    m_pTarget = pEnemyChamp;
                    m_pLeeSin->Set_AttackTarget(m_pTarget); // 타겟팅 상태 유지
                }
            }
        }
    }

    // 타겟 없거나 죽었으면 취소
    if (!m_pTarget || m_pTarget->Get_Dead())
    {
        m_bAnimFinished = true;
        return;
    }

    // =========================================================
    // (선택) 리신이 타겟을 바라보게(차는 방향 정렬)
    // 너 프로젝트에서 LOOK이 뒤집혀 있어서,
    // Enemy 코드 패턴대로 "myPos - targetPos"로 LOOK 세팅
    // (Update_RCollider에서 -LOOK을 쓰니까 정방향이 맞아짐)
    // =========================================================
    {
        CTransform* pTr = m_pLeeSin->Get_Transform();
        _vector myPos = pTr->Get_State(CTransform::STATE_TRANSLATION);
        _vector tPos = m_pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

        _vector vLook = XMVectorSetY(myPos - tPos, 0.f);
        if (XMVectorGetX(XMVector3LengthSq(vLook)) > 0.0001f)
        {
            vLook = XMVector3Normalize(vLook);

            _vector vOldRight = pTr->Get_State(CTransform::STATE_RIGHT);
            float fScale = XMVectorGetX(XMVector3Length(vOldRight));
            if (fScale < 0.0001f) fScale = 1.f;

            _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
            _vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vLook));

            vRight *= fScale;
            vLook *= fScale;
            vUp = XMVector3Normalize(XMVector3Cross(vLook, vRight)) * fScale;

            vRight = XMVectorSetW(vRight, 0.f);
            vUp = XMVectorSetW(vUp, 0.f);
            vLook = XMVectorSetW(vLook, 0.f);

            pTr->Set_State(CTransform::STATE_RIGHT, vRight);
            pTr->Set_State(CTransform::STATE_UP, vUp);
            pTr->Set_State(CTransform::STATE_LOOK, vLook);
        }
    }

    // =========================================================
    // ★ R_A / R_B / R_C 랜덤 선택
    // =========================================================
    _uint animList[3] = { CLeeSin::ANIM::R_A, CLeeSin::ANIM::R_B, CLeeSin::ANIM::R_C };
    int r = rand() % 3;
    m_pLeeSin->Get_Model()->Set_CurrentAnimIndex(animList[r]);
}

void CLeeSinSkillStateR::Exit()
{
    Destroy_RCollider();
    __super::Exit();
}

void CLeeSinSkillStateR::Create_RCollider()
{
    if (m_pRCollider) return;

    CColider::COLLIDERDESC desc{};
    desc.vScale = m_vRBoxScale;
    desc.vRotation = _float3(0.f, 0.f, 0.f);
    desc.vPosition = _float3(0.f, 0.f, 0.f);

    m_pRCollider =
        CCollision_Manager::GetInstance()->Reuse_Collider(
            CColider::TYPE_AABB,
            LEVEL_STATIC,
            TEXT("Prototype_Component_Colider_AABB"),
            &desc);
}

void CLeeSinSkillStateR::Update_RCollider()
{
    if (!m_pRCollider || !m_pLeeSin) return;

    CTransform* pTr = m_pLeeSin->Get_Transform();
    _vector vPos = pTr->Get_State(CTransform::STATE_TRANSLATION);
    _vector vLook = pTr->Get_State(CTransform::STATE_LOOK);

    // ★ 너 코드 유지: -Look 보정
    vLook = -vLook;
    vLook = XMVectorSetY(vLook, 0.f);

    if (XMVectorGetX(XMVector3LengthSq(vLook)) < 0.0001f)
        vLook = XMVectorSet(0.f, 0.f, 1.f, 0.f);
    else
        vLook = XMVector3Normalize(vLook);

    _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    _vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vLook));
    vUp = XMVector3Normalize(XMVector3Cross(vLook, vRight));

    _vector vCenter = vPos + vLook * m_fROffset + XMVectorSet(0.f, m_fRYOffset, 0.f, 0.f);
    vCenter = XMVectorSetW(vCenter, 1.f);

    _matrix matWorld = XMMatrixIdentity();
    matWorld.r[0] = vRight;
    matWorld.r[1] = vUp;
    matWorld.r[2] = vLook;
    matWorld.r[3] = vCenter;

    m_pRCollider->Update(matWorld);
}

void CLeeSinSkillStateR::Destroy_RCollider()
{
    if (!m_pRCollider) return;

    CCollision_Manager::GetInstance()->Collect_Collider(
        CColider::TYPE_AABB,
        m_pRCollider);

    m_pRCollider = nullptr;
}

void CLeeSinSkillStateR::Do_DragonRage()
{
    if (!m_pLeeSin || !m_pTarget || !m_pRCollider) return;
    if (m_pTarget->Get_Dead()) return;

    // ★ 여기서도 한 번 더 보장: 적 챔피언만
    CEnemy* pEnemyChamp = dynamic_cast<CEnemy*>(m_pTarget);
    if (!pEnemyChamp) return;

    // =========================
    // "타겟이 콜라이더에 실제로 맞았는지" 체크
    // ★ ENEMY 그룹만 검사 (MINION 삭제)
    // =========================
    auto* pCM = CCollision_Manager::GetInstance();
    std::vector<CActor*> hits;

    bool bHitTarget = false;

    if (pCM->CollisionwithGroup_All(CCollision_Manager::COLLISION_ENEMY, m_pRCollider, &hits,
        CCollision_Manager::COLLIDER_END))
    {
        for (auto* t : hits)
        {
            if (!t || t->Get_Dead()) continue;
            if (t == pEnemyChamp) { bHitTarget = true; break; }
        }
    }

    if (!bHitTarget)
        return;

    // =========================
    // 데미지
    // =========================
    pEnemyChamp->Take_Damage(m_iDamageR, m_pLeeSin);

    // FX
    {
        _matrix world = pEnemyChamp->Get_Transform()->Get_WorldMatrix();
        _vector vPos = pEnemyChamp->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
        vPos = XMVectorSetW(vPos, 1.f);
        world.r[3] = vPos;
        CEffect::PlayEffectAtLocation(TEXT("LeeSin_R_Hit.dat"), world);
    }

    // =========================
    // 넉백 방향 = (리신 -> 타겟) (XZ)
    // =========================
    _vector vLee = m_pLeeSin->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
    _vector vTar = pEnemyChamp->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

    _vector vDir = XMVectorSetY(vTar - vLee, 0.f);
    if (XMVectorGetX(XMVector3LengthSq(vDir)) < 0.0001f)
        vDir = XMVectorSet(0.f, 0.f, 1.f, 0.f);
    else
        vDir = XMVector3Normalize(vDir);

    m_vKnockDir = vDir;
    m_bKnockActive = true;
    m_fKnockAcc = 0.f;
}

void CLeeSinSkillStateR::Update_Knockback(_float dt)
{
    if (!m_bKnockActive) return;
    if (!m_pTarget || m_pTarget->Get_Dead())
    {
        m_bKnockActive = false;
        return;
    }

    m_fKnockAcc += dt;
    if (m_fKnockAcc >= m_fKnockDuration)
    {
        m_bKnockActive = false;
        return;
    }

    CTransform* pT = m_pTarget->Get_Transform();
    _vector vPos = pT->Get_State(CTransform::STATE_TRANSLATION);

    _vector vNext = vPos + m_vKnockDir * (m_fKnockSpeed * dt);
    vNext = XMVectorSetY(vNext, XMVectorGetY(vPos));
    vNext = XMVectorSetW(vNext, 1.f);

    pT->Set_State(CTransform::STATE_TRANSLATION, vNext);
}

CPlayerState* CLeeSinSkillStateR::Update(_float dt)
{
    if (!m_pLeeSin) return nullptr;

    _uint curAnim = m_pLeeSin->Get_Model()->Get_CurrentAnimIndex();

    m_bAnimFinished =
        m_pLeeSin->Get_Model()->Play_Animation(
            dt,
            m_pLeeSin->Is_AnimationLoop(curAnim),
            nullptr,
            0.1f
        );

    m_fAcc += dt;

    Update_Knockback(dt);

    // ===== 이벤트 구간 체크 =====
    bool bNowEvent = false;
    const auto events = m_pLeeSin->Get_Model()->Get_Events();
    for (const auto& evt : events)
    {
        if (evt.isPlay && evt.eType == ANIMEVENT::EVENT_COLLIDER)
        {
            bNowEvent = true;
            break;
        }
    }

    if (bNowEvent)
    {
        if (!m_bColliderEventPlaying)
        {
            m_bColliderEventPlaying = true;

            Create_RCollider();
            Update_RCollider();

            if (!m_bDidHit)
            {
                m_bDidHit = true;
                Do_DragonRage();
            }
        }
        else
        {
            Create_RCollider();
            Update_RCollider();
        }
    }
    else
    {
        if (m_bColliderEventPlaying)
        {
            m_bColliderEventPlaying = false;
            Destroy_RCollider();
        }

        // 이벤트 타이밍 안전장치(원하면 삭제)
        if (!m_bDidHit && m_fAcc >= 0.15f)
        {
            m_bDidHit = true;
            Create_RCollider();
            Update_RCollider();
            Do_DragonRage();
            Destroy_RCollider();
        }
    }

    return nullptr;
}

CPlayerState* CLeeSinSkillStateR::Late_Update(_float dt)
{
#ifdef _DEBUG
    if (m_pRCollider && m_pOwner)
        m_pOwner->Get_Renderer()->Add_Debug(m_pRCollider);
#endif

    if (m_bAnimFinished && !m_bKnockActive)
        return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);

    return nullptr;
}
