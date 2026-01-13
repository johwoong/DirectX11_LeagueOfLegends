#include "LeeSinSkillStateE.h"

#include "LeeSin.h"
#include "PlayerIdleState.h"
#include "Collision_Manager.h"
#include "Effect.h"
#include "CColider.h"

using namespace Player;

CLeeSinSkillStateE::CLeeSinSkillStateE(CPlayer* pPlayer)
{
    m_pOwner = pPlayer;
    if (m_pOwner)
        m_ePlayerID = m_pOwner->Get_PlayerID();
}

CPlayerState* CLeeSinSkillStateE::HandleInput() { return nullptr; }

void CLeeSinSkillStateE::Enter()
{
    __super::Enter();
    m_eState = STATE_ID::STATE_SKILL_ATTACK_E;

    m_pLeeSin = dynamic_cast<CLeeSin*>(m_pOwner);
    if (!m_pLeeSin) return;

    m_fAcc = 0.f;
    m_bDidHit = false;
    m_bAnimFinished = false;
    m_bColliderEventPlaying = false;

    Destroy_E1Collider();

    // ★ E2는 "표식이 남아있을 때만" 진입 가능
    if (m_pLeeSin->Can_ERecast())
    {
        m_ePhase = PHASE::CAST_E2;
        m_pLeeSin->Consume_ERecast();                 // 아이콘 닫기
        m_pLeeSin->Get_Model()->Set_CurrentAnimIndex(CLeeSin::ANIM::E_B);
    }
    else
    {
        m_ePhase = PHASE::CAST_E1;

        // 새 E1 시작이면 이전 표식 비움
        m_pLeeSin->Clear_EMarkTargets();

        // 여기서 Start_ERecast() 절대 하지마
        m_pLeeSin->Get_Model()->Set_CurrentAnimIndex(CLeeSin::ANIM::E);
    }
}

void CLeeSinSkillStateE::Exit()
{
    Destroy_E1Collider();
    __super::Exit();
}

void CLeeSinSkillStateE::Create_E1Collider()
{
    if (m_pE1Collider) return;

    CColider::COLLIDERDESC desc{};
    desc.vScale = m_vE1BoxScale;
    desc.vRotation = _float3(0.f, 0.f, 0.f);
    desc.vPosition = _float3(0.f, 0.f, 0.f);

    m_pE1Collider =
        CCollision_Manager::GetInstance()->Reuse_Collider(
            CColider::TYPE_AABB,
            LEVEL_STATIC,
            TEXT("Prototype_Component_Colider_AABB"),
            &desc);
}

void CLeeSinSkillStateE::Update_E1Collider()
{
    if (!m_pE1Collider || !m_pLeeSin) return;

    CTransform* pTr = m_pLeeSin->Get_Transform();

    _vector vPos = pTr->Get_State(CTransform::STATE_TRANSLATION);
    _vector vLook = pTr->Get_State(CTransform::STATE_LOOK);

    // 네 AttackState랑 동일하게 -Look 보정
    vLook = -vLook;
    vLook = XMVectorSetY(vLook, 0.f);

    if (XMVectorGetX(XMVector3LengthSq(vLook)) < 0.0001f)
        vLook = XMVectorSet(0.f, 0.f, 1.f, 0.f);
    else
        vLook = XMVector3Normalize(vLook);

    _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    _vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vLook));
    vUp = XMVector3Normalize(XMVector3Cross(vLook, vRight));

    // 발밑 중심(0) or 살짝 전방
    _vector vCenter = vPos + vLook * m_fE1Offset;
    vCenter = XMVectorSetW(vCenter, 1.f);

    _matrix matWorld = XMMatrixIdentity();
    matWorld.r[0] = vRight;
    matWorld.r[1] = vUp;
    matWorld.r[2] = vLook;
    matWorld.r[3] = vCenter;

    m_pE1Collider->Update(matWorld);
}

void CLeeSinSkillStateE::Destroy_E1Collider()
{
    if (!m_pE1Collider) return;

    CCollision_Manager::GetInstance()->Collect_Collider(
        CColider::TYPE_AABB,
        m_pE1Collider);

    m_pE1Collider = nullptr;
}

CPlayerState* CLeeSinSkillStateE::Update(_float dt)
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

    // ===== 이벤트 시작/유지/종료 =====
    if (bNowEvent)
    {
        if (!m_bColliderEventPlaying)
        {
            m_bColliderEventPlaying = true;

            // ★ E1만 콜라이더 생성/업데이트
            if (m_ePhase == PHASE::CAST_E1)
            {
                Create_E1Collider();
                Update_E1Collider();
            }

            // ★ 데미지/처리는 딱 1번만
            if (!m_bDidHit)
            {
                m_bDidHit = true;
                if (m_ePhase == PHASE::CAST_E1) Do_Tempest();
                else                            Do_Cripple();
            }
        }
        else
        {
            // 이벤트 진행 중이면 E1만 위치 갱신
            if (m_ePhase == PHASE::CAST_E1)
            {
                Create_E1Collider();
                Update_E1Collider();
            }
        }
    }
    else
    {
        if (m_bColliderEventPlaying)
        {
            m_bColliderEventPlaying = false;

            // ★ E1만 콜라이더 반환
            if (m_ePhase == PHASE::CAST_E1)
                Destroy_E1Collider();
        }

        // 안전장치(이벤트가 아예 없을 때)
        if (!m_bDidHit)
        {
            const float hitTime = (m_ePhase == PHASE::CAST_E1) ? 0.12f : 0.08f;
            if (m_fAcc >= hitTime)
            {
                m_bDidHit = true;

                if (m_ePhase == PHASE::CAST_E1)
                {
                    Create_E1Collider();
                    Update_E1Collider();
                    Do_Tempest();
                    Destroy_E1Collider();
                }
                else
                {
                    Do_Cripple(); // ★ E2는 콜라이더 없음
                }
            }
        }
    }

    return nullptr;
}

CPlayerState* CLeeSinSkillStateE::Late_Update(_float dt)
{
#ifdef _DEBUG
    if (m_pE1Collider && m_pOwner)
        m_pOwner->Get_Renderer()->Add_Debug(m_pE1Collider);
#endif

    if (m_bAnimFinished)
        return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);

    return nullptr;
}

void CLeeSinSkillStateE::Do_Tempest()
{
    // FX (기존)
    {
        _matrix world = m_pLeeSin->Get_Transform()->Get_WorldMatrix();
        _vector vPos = m_pLeeSin->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
        vPos = XMVectorSetW(vPos, 1.f);
        world.r[3] = vPos;
        CEffect::PlayEffectAtLocation(TEXT("LeeSin_E1.dat"), world);
    }

    if (!m_pE1Collider) return;

    auto* pCM = CCollision_Manager::GetInstance();
    std::vector<CActor*> hits;
    bool bAnyHit = false;

    auto ApplyHit = [&](CActor* t)
        {
            if (!t || t->Get_Dead()) return;

            t->Take_Damage(m_iDamageE1, m_pLeeSin);

            // 1) E2 대상 리스트
            m_pLeeSin->Add_EMarkTarget(t);

            // 2) 타겟 위에 마크 이펙트
            //    파일명은 네가 가진 dat로 맞춰 (예: "LeeSin_EMark.dat")
            t->Apply_Mark(
                CActor::MARK_TYPE::LEE_SIN_E,
                (CActor*)m_pLeeSin,
                3.f,
                TEXT("LeeSin_EMark.dat"),
                _float3(0.f, 1.1f, 0.f)   // 타겟 머리 위(원하는 높이로 튜닝)
            );

            bAnyHit = true;
        };

    hits.clear();
    if (pCM->CollisionwithGroup_All(CCollision_Manager::COLLISION_ENEMY, m_pE1Collider, &hits,
        CCollision_Manager::COLLIDER_END))
    {
        for (auto* t : hits) ApplyHit(t);
    }

    hits.clear();
    if (pCM->CollisionwithGroup_All(CCollision_Manager::COLLISION_MINION, m_pE1Collider, &hits,
        CCollision_Manager::COLLIDER_END))
    {
        for (auto* t : hits) ApplyHit(t);
    }

    if (bAnyHit)
    {
        m_pLeeSin->Start_ERecast(3.f);
    }
    else
    {
        m_pLeeSin->Consume_ERecast();
        m_pLeeSin->Clear_EMarkTargets();
    }
}

void CLeeSinSkillStateE::Do_Cripple()
{
    // FX
    {
        _matrix world = m_pLeeSin->Get_Transform()->Get_WorldMatrix();
        _vector vPos = m_pLeeSin->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
        vPos = XMVectorSetW(vPos, 1.f);
        world.r[3] = vPos;
        CEffect::PlayEffectAtLocation(TEXT("LeeSin_E2.dat"), world);
    }

    // ★ 콜라이더 없음. 표식(리신이 저장한 리스트) 대상만 처리
    const auto& marks = m_pLeeSin->Get_EMarkTargets();
    for (auto* t : marks)
    {
        if (!t || t->Get_Dead()) continue;

        // 여기서 "표식 대상만 데미지/슬로우"
        t->Take_Damage(m_iDamageE2, m_pLeeSin);

        // 슬로우 시스템 있으면 여기서:
        // t->Apply_Slow(0.4f, 2.0f);
    }

    // ★ E2 사용하면 표식 소모
    m_pLeeSin->Clear_EMarkTargets();
}
