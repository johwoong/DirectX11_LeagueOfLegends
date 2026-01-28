#include "LeeSinSkillStateQ.h"
#include "LeeSin.h"

#include "GameInstance.h"
#include "LeeSinAttackState.h"
#include "PlayerRunState.h"
#include "PlayerIdleState.h"
#include "CursorManager.h"

#include "PoolManager.h"
#include "LeeSinProjectile.h"

using namespace Player;

CLeeSinSkillStateQ::CLeeSinSkillStateQ(CPlayer* pPlayer)
{
    m_pOwner = dynamic_cast<CLeeSin*>(pPlayer);
    m_eState = STATE_SKILL_ATTACK_Q;
}

CPlayerState* CLeeSinSkillStateQ::HandleInput()
{
    return nullptr;
}

CPlayerState* CLeeSinSkillStateQ::Update(_float dt)
{
    if (!m_pOwner)
        return nullptr; // 여기서 IdleState 만들면 m_pOwner null이라 터질 수 있음

    CModel* pModel = m_pOwner->Get_Model();

    const bool bAnimEnd = pModel->Play_Animation(
        dt,
        m_pOwner->Is_AnimationLoop(pModel->Get_CurrentAnimIndex()),
        nullptr,
        0.1f);

    switch (m_ePhase)
    {
    case PHASE::CAST_Q1:   Update_CastQ1(dt); break;
    case PHASE::DASH_Q2:   Update_DashQ2(dt); break;
    case PHASE::TRANS_HIT: Update_Trans(dt);  break;
    }

    // ★ 대시 중 타겟 소실/사망 등으로 강제 종료 요청
    if (m_bForceExitIdle)
    {
        m_bForceExitIdle = false;
        m_pOwner->SetUsingSkill(false);
        return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
    }

    if (bAnimEnd)
    {
        if (m_ePhase == PHASE::CAST_Q1)
        {
            m_pOwner->SetUsingSkill(false);
            return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
        }
        if (m_ePhase == PHASE::TRANS_HIT)
        {
            m_pOwner->SetUsingSkill(false);
            return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
        }
    }

    return nullptr;
}

CPlayerState* CLeeSinSkillStateQ::Late_Update(_float dt)
{
    return nullptr;
}

void CLeeSinSkillStateQ::Enter()
{
    __super::Enter();

    if (!m_pOwner)
        return;

    m_pOwner->SetUsingSkill(true);

    // Q2 가능하면 DASH, 아니면 CAST
    if (m_pOwner->Can_QRecast())
        Enter_DashQ2();
    else
        Enter_CastQ1();
}

void CLeeSinSkillStateQ::Exit()
{
    if (m_pQ2Collider)
    {
        CCollision_Manager::GetInstance()->Collect_Collider(CColider::TYPE_AABB, m_pQ2Collider);
        m_pQ2Collider = nullptr;
    }
    m_bQ2DamageApplied = false;

    m_bQ2Locked = false;
    m_bForceExitIdle = false;

    if (m_pOwner)
        m_pOwner->SetUsingSkill(false);

    __super::Exit();
}

void CLeeSinSkillStateQ::Enter_CastQ1()
{
    m_ePhase = PHASE::CAST_Q1;
    m_bProjectileSpawned = false;
    m_bHitApplied = false;
   
    CCursorManager* CM = CCursorManager::GetInstance();
    _vector rayPos = CM->Get_RayPos();
    _vector rayDir = CM->Get_RayDir();

    CTransform* pTr = m_pOwner->Get_Transform();

    _vector myPos = pTr->Get_State(CTransform::STATE_TRANSLATION);
    float y = XMVectorGetY(myPos);

    const float rayDirY = XMVectorGetY(rayDir);
    float t = 0.f;
    if (fabsf(rayDirY) > 1e-6f)
        t = (y - XMVectorGetY(rayPos)) / rayDirY;

    _vector hit = rayPos + rayDir * t;

    // ===== 방향(0벡터 방지) =====
    _vector dirXZ = XMVectorSetY(hit - myPos, 0.f);
    if (XMVectorGetX(XMVector3LengthSq(dirXZ)) < 1e-6f)
    {
        // 커서가 내 발밑이면 기존 LOOK로 대체
        dirXZ = XMVectorSetY(pTr->Get_State(CTransform::STATE_LOOK), 0.f);
        if (XMVectorGetX(XMVector3LengthSq(dirXZ)) < 1e-6f)
            dirXZ = XMVectorSet(0.f, 0.f, 1.f, 0.f);
    }

    _vector vDir = XMVector3Normalize(dirXZ);
    _vector vLook = -vDir; // 네 코드 기준(모델 forward가 -LOOK이라면 유지)

    // ===== 스케일(축 길이) 보존 =====
    _vector oldR = pTr->Get_State(CTransform::STATE_RIGHT);
    _vector oldU = pTr->Get_State(CTransform::STATE_UP);
    _vector oldL = pTr->Get_State(CTransform::STATE_LOOK);

    float sx = XMVectorGetX(XMVector3Length(oldR));
    float sy = XMVectorGetX(XMVector3Length(oldU));
    float sz = XMVectorGetX(XMVector3Length(oldL));

    if (sx < 1e-6f) sx = 1.f;
    if (sy < 1e-6f) sy = 1.f;
    if (sz < 1e-6f) sz = 1.f;

    // ===== 축 재구성 =====
    _vector upWorld = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    _vector vRight = XMVector3Cross(upWorld, vLook);

    if (XMVectorGetX(XMVector3LengthSq(vRight)) < 1e-6f)
    {
        // 거의 수직이면 기존 RIGHT 방향을 사용(정규화)
        vRight = oldR;
        vRight = XMVectorSetY(vRight, 0.f);
        if (XMVectorGetX(XMVector3LengthSq(vRight)) < 1e-6f)
            vRight = XMVectorSet(1.f, 0.f, 0.f, 0.f);
    }

    vRight = XMVector3Normalize(vRight);
    _vector vUp = XMVector3Normalize(XMVector3Cross(vLook, vRight));

    // ★ 스케일 유지해서 세팅
    pTr->Set_State(CTransform::STATE_RIGHT, vRight * sx);
    pTr->Set_State(CTransform::STATE_UP, vUp * sy);
    pTr->Set_State(CTransform::STATE_LOOK, vLook * sz);

    // 실제 투사체 방향
    XMStoreFloat3(&m_vDir, -vLook);

    // 애니 세팅
    m_pOwner->Get_Model()->Set_CurrentAnimIndex(CLeeSin::ANIM::Q);
}

void CLeeSinSkillStateQ::Enter_DashQ2()
{
    m_ePhase = PHASE::DASH_Q2;
    m_bHitApplied = false;
    m_bQ2HitboxSpawned = false;

    m_bForceExitIdle = false;

    // ★ 여기서 타겟을 LOCK 한다
    m_pTarget = m_pOwner->Get_QMarkTarget();
    if (!m_pTarget)
    {
        m_pOwner->SetUsingSkill(false);
        m_bForceExitIdle = true;
        return;
    }

    // ★ Q2 캐스팅 순간에만 Can_QRecast 검사 (한 번만!)
    if (!m_pOwner->Can_QRecast())
    {
        // 아이콘 복구 포함 정리
        m_pOwner->Clear_QMarkTarget();
        m_pOwner->SetUsingSkill(false);
        m_bForceExitIdle = true;
        return;
    }

    // 이제부터는 표식 검사 끊고 LOCK 상태로 진행
    m_bQ2Locked = true;

    m_fQ2StopDist = 1.05f;

    _vector vPos = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
    _vector vTar = m_pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
    _vector d = XMVectorSetY(vTar - vPos, 0.f);
    float dist = XMVectorGetX(XMVector3Length(d));

    if (dist < 3.f) { m_iDashType = 0; m_pOwner->Get_Model()->Set_CurrentAnimIndex(CLeeSin::ANIM::Q_SHORT); }
    else if (dist < 7.f) { m_iDashType = 1; m_pOwner->Get_Model()->Set_CurrentAnimIndex(CLeeSin::ANIM::Q_MID); }
    else { m_iDashType = 2; m_pOwner->Get_Model()->Set_CurrentAnimIndex(CLeeSin::ANIM::Q_LONG); }
}

void CLeeSinSkillStateQ::Enter_Trans()
{
    m_ePhase = PHASE::TRANS_HIT;
    CGameInstance::GetInstance()->PlaySounds(const_cast<TCHAR*>(L"LeeSINQSound2.wav"), SOUND_PLAYERTALK, 0.4f);
    m_bHitApplied = false;
    m_bQ2DamageApplied = false;

    if (m_pQ2Collider)
    {
        CCollision_Manager::GetInstance()->Collect_Collider(CColider::TYPE_AABB, m_pQ2Collider);
        m_pQ2Collider = nullptr;
    }

    if (m_iDashType == 0)      m_pOwner->Get_Model()->Set_CurrentAnimIndex(CLeeSin::ANIM::Q_SHORTTRANS);
    else if (m_iDashType == 1) m_pOwner->Get_Model()->Set_CurrentAnimIndex(CLeeSin::ANIM::Q_MIDTRANS);
    else                       m_pOwner->Get_Model()->Set_CurrentAnimIndex(CLeeSin::ANIM::Q_LONGTRANS);

    // ===== 트랜스 시작 시 타겟 바라보기(방향 뒤집힘 방지) =====
    CActor* pT = m_pOwner->Get_QMarkTarget();
    if (!pT) return;

    CTransform* pTr = m_pOwner->Get_Transform();
    _vector vPos = pTr->Get_State(CTransform::STATE_TRANSLATION);
    _vector vTar = pT->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

    _vector forward = XMVectorSetY(vTar - vPos, 0.f);
    if (XMVectorGetX(XMVector3LengthSq(forward)) < 1e-6f)
        return;
    forward = XMVector3Normalize(forward);

    // 네 컨벤션: LOOK은 forward의 반대
    _vector look = -forward;

    // 스케일 보존
    _vector oldR = pTr->Get_State(CTransform::STATE_RIGHT);
    _vector oldU = pTr->Get_State(CTransform::STATE_UP);
    _vector oldL = pTr->Get_State(CTransform::STATE_LOOK);

    float sx = XMVectorGetX(XMVector3Length(oldR)); if (sx < 1e-6f) sx = 1.f;
    float sy = XMVectorGetX(XMVector3Length(oldU)); if (sy < 1e-6f) sy = 1.f;
    float sz = XMVectorGetX(XMVector3Length(oldL)); if (sz < 1e-6f) sz = 1.f;

    _vector upWorld = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    _vector right = XMVector3Normalize(XMVector3Cross(upWorld, look));
    _vector up = XMVector3Normalize(XMVector3Cross(look, right));

    pTr->Set_State(CTransform::STATE_RIGHT, right * sx);
    pTr->Set_State(CTransform::STATE_UP, up * sy);
    pTr->Set_State(CTransform::STATE_LOOK, look * sz);
}

void CLeeSinSkillStateQ::Update_CastQ1(_float dt)
{
    if (!m_bProjectileSpawned)
    {
        vector<ANIMEVENT> events = m_pOwner->Get_Model()->Get_Events();

        for (auto& evt : events)
        {
            if (!evt.isPlay) continue;
            if (evt.eType != ANIMEVENT::EVENT_PROJECTILE) continue;

            m_bProjectileSpawned = true;
            Spawn_QWaveProjectile();
            break;
        }
    }
}

void CLeeSinSkillStateQ::Update_DashQ2(_float dt)
{
    // ★ 대시 중엔 표식/Can_QRecast 재검사 절대 금지
   //    (리신 Update에서 Clear_QMarkTarget이 들어가면 1프레임에 끊겨서 멈춤)

    if (!m_bQ2Locked)
    {
        // 혹시라도 들어오면 바로 종료
        m_bForceExitIdle = true;
        return;
    }

    if (!m_pTarget || m_pTarget->Get_Dead())
    {
        // 타겟이 죽었으면 대시 취소 + 아이콘 복구
        m_pOwner->Clear_QMarkTarget();
        m_bQ2Locked = false;
        m_bForceExitIdle = true;
        return;
    }

    CTransform* pTr = m_pOwner->Get_Transform();

    _vector vPos = pTr->Get_State(CTransform::STATE_TRANSLATION);
    _vector vTar = m_pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

    _vector toTar = XMVectorSetY(vTar - vPos, 0.f);
    float dist = XMVectorGetX(XMVector3Length(toTar));

    if (dist <= m_fQ2StopDist + 0.001f)
    {
        Enter_Trans();
        return;
    }

    _vector forward = XMVector3Normalize(toTar);
    forward = XMVectorSetW(forward, 0.f);

    float moveMax = m_fDashSpeed * dt;
    float need = dist - m_fQ2StopDist;

    float move = (moveMax >= need) ? need : moveMax;

    _vector vNext = vPos + forward * move;
    vNext = XMVectorSetY(vNext, XMVectorGetY(vPos));
    pTr->Set_State(CTransform::STATE_TRANSLATION, XMVectorSetW(vNext, 1.f));

    // 방향 정렬(네 컨벤션: forward = -LOOK)
    {
        _vector look = XMVectorSetW(-forward, 0.f);
        _vector upWorld = XMVectorSet(0.f, 1.f, 0.f, 0.f);

        _vector oldR = pTr->Get_State(CTransform::STATE_RIGHT);
        _vector oldU = pTr->Get_State(CTransform::STATE_UP);
        _vector oldL = pTr->Get_State(CTransform::STATE_LOOK);
        float sx = XMVectorGetX(XMVector3Length(oldR)); if (sx < 1e-6f) sx = 1.f;
        float sy = XMVectorGetX(XMVector3Length(oldU)); if (sy < 1e-6f) sy = 1.f;
        float sz = XMVectorGetX(XMVector3Length(oldL)); if (sz < 1e-6f) sz = 1.f;

        _vector right = XMVector3Cross(upWorld, look);
        if (XMVectorGetX(XMVector3LengthSq(right)) < 1e-6f)
            right = XMVectorSet(1.f, 0.f, 0.f, 0.f);

        right = XMVector3Normalize(right);
        _vector up = XMVector3Normalize(XMVector3Cross(look, right));

        pTr->Set_State(CTransform::STATE_RIGHT, XMVectorSetW(right, 0.f) * sx);
        pTr->Set_State(CTransform::STATE_UP, XMVectorSetW(up, 0.f) * sy);
        pTr->Set_State(CTransform::STATE_LOOK, XMVectorSetW(look, 0.f) * sz);
    }

    // 도착 가능하면 Trans
    if (moveMax >= need)
    {
        m_pOwner->Check_Navigation(); // 1회만
        Enter_Trans();
    }
}

void CLeeSinSkillStateQ::Update_Trans(_float dt)
{
    CActor* pTarget = m_pOwner->Get_QMarkTarget();

    // ===============================
    // ★ 애니 이벤트 → 콜라이더 ON/OFF
    // ===============================
    bool bColliderEventPlaying = false;

    vector<ANIMEVENT> events = m_pOwner->Get_Model()->Get_Events();
    for (auto& evt : events)
    {
        if (evt.isPlay && evt.eType == ANIMEVENT::EVENT_COLLIDER)
        {
            bColliderEventPlaying = true;

            // 이벤트 시작 순간에 콜라이더 확보(1회)
            if (!m_pQ2Collider)
            {
                CColider::COLLIDERDESC desc{};
                desc.vScale = m_vQ2HitScale;     // 튜닝
                desc.vRotation = _float3(0.f, 0.f, 0.f);
                desc.vPosition = _float3(0.f, 0.f, 0.f);

                m_pQ2Collider =
                    CCollision_Manager::GetInstance()->Reuse_Collider(
                        CColider::TYPE_AABB,
                        LEVEL_STATIC,
                        TEXT("Prototype_Component_Colider_AABB"),
                        &desc);
            }
            break;
        }
    }

    // 이벤트가 끝났으면 반납(공격이랑 동일)
    if (!bColliderEventPlaying && m_pQ2Collider)
    {
        CCollision_Manager::GetInstance()->Collect_Collider(CColider::TYPE_AABB, m_pQ2Collider);
        m_pQ2Collider = nullptr;
        m_bQ2DamageApplied = false;
        return;
    }

    // ===============================
    // 콜라이더 월드 갱신
    // ===============================
    if (m_pQ2Collider)
    {
        CTransform* pTr = m_pOwner->Get_Transform();

        _vector vPos = pTr->Get_State(CTransform::STATE_TRANSLATION);

        // Q2는 "타겟을 향한 방향"이 가장 자연스러움
        _vector vForward = pTr->Get_State(CTransform::STATE_LOOK);
        vForward = -vForward; // 너 프레임워크 기준 forward 보정(공격이랑 동일)

        if (pTarget && !pTarget->Get_Dead())
        {
            _vector vTar = pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
            _vector dir = XMVectorSetY(vTar - vPos, 0.f);
            if (XMVectorGetX(XMVector3LengthSq(dir)) > 1e-6f)
                vForward = XMVector3Normalize(dir);
        }

        vForward = XMVectorSetY(vForward, 0.f);
        if (XMVectorGetX(XMVector3LengthSq(vForward)) < 1e-6f)
            vForward = XMVectorSet(0.f, 0.f, 1.f, 0.f);
        vForward = XMVector3Normalize(vForward);

        _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
        _vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vForward));
        vUp = XMVector3Normalize(XMVector3Cross(vForward, vRight));

        // 리신 앞쪽에 콜라이더 배치(타겟과 거리두는 만큼 offset이 중요)
        _vector vHitPos = vPos + vForward * m_fQ2HitOffset;
        vHitPos = XMVectorSetW(vHitPos, 1.f);

        _matrix matWorld = XMMatrixIdentity();
        matWorld.r[0] = vRight;
        matWorld.r[1] = vUp;
        matWorld.r[2] = vForward;
        matWorld.r[3] = vHitPos;

        m_pQ2Collider->Update(matWorld);
    }

    // ===============================
    // 데미지 판정(1회)
    // ===============================
    if (m_pQ2Collider && !m_bQ2DamageApplied && pTarget && !pTarget->Get_Dead())
    {
        CColider* pTarCol = pTarget->Get_Collider();
        if (pTarCol && m_pQ2Collider->Collision(pTarCol))
        {
            pTarget->Take_Damage(80, m_pOwner); // Q2 데미지 튜닝
            pTarget->Remove_Mark(CActor::MARK_TYPE::LEE_SIN_Q);

            m_bQ2DamageApplied = true;

            // Q2 사용 끝났으니 표식/타겟 정리 (아이콘 복귀 포함)
            m_pOwner->Clear_QMarkTarget();
        }
    }
}

void CLeeSinSkillStateQ::Spawn_QWaveProjectile()
{
    CModel* pModel = m_pOwner->Get_Model();

    CBone* pHand = pModel->Get_BonePtr("R_Hand");
    if (!pHand) pHand = pModel->Get_BonePtr("L_Hand");
    if (!pHand) return;

    _float4x4 pivot = pModel->Get_PivotFloat4x4();
    _matrix pivotMat = XMLoadFloat4x4(&pivot);

    _matrix handMatrix =
        pHand->Get_CombinedTransformationMatrix() *
        pivotMat *
        m_pOwner->Get_Transform()->Get_WorldMatrix();

    handMatrix.r[0] = XMVector4Normalize(handMatrix.r[0]);
    handMatrix.r[1] = XMVector4Normalize(handMatrix.r[1]);
    handMatrix.r[2] = XMVector4Normalize(handMatrix.r[2]);

    _vector vHandPos = handMatrix.r[3];

    _vector vDir = XMLoadFloat3(&m_vDir);
    vDir = XMVectorSetY(vDir, 0.f);
    if (XMVectorGetX(XMVector3LengthSq(vDir)) < 1e-6f)
        return;

    vDir = XMVector3Normalize(vDir);
    vDir = XMVectorSetW(vDir, 0.f);

    // =========================
    // ★ 핵심: 리신 전용 풀/레이어로 분리
    // =========================
    const _tchar* LAYER_LEESIN_PROJ = TEXT("Layer_LeeSinProjectile");

    CProjectile::PROJECTILEDESC desc{};
    desc.eCollisionGroup = PLAYER;
    desc.eProjType = 0; // (원하면 enum으로 Q1 넣어라)
    desc.vInitPositon = XMVectorSetW(vHandPos, 1.f);
    desc.vTargetDir = vDir;
    desc.fVelocity = 18.f;
    desc.fDeadTime = 1.2f;
    desc.iDamage = 60;
    desc.pOwner = m_pOwner;
    desc.pTarget = nullptr;

    desc.ePoolLevel = LEVEL_GAMEPLAY;
    desc.pPoolLayerTag = LAYER_LEESIN_PROJ;

    CPoolManager* PM = CPoolManager::GetInstance();
    CGameInstance* GI = CGameInstance::GetInstance();
    CGameInstance::GetInstance()->PlaySounds(const_cast<TCHAR*>(L"LeeSinQSound.wav"), SOUND_PLAYERTALK, 0.4f);
    CGameInstance::GetInstance()->PlaySounds(const_cast<TCHAR*>(L"LeeSinSonicWave.wav"), SOUND_PLAYER, 0.05f);
    // 1) 리신 전용 풀에서만 재사용
    if (!PM->Reuse_Pooling_Object(LEVEL_GAMEPLAY, LAYER_LEESIN_PROJ, &desc))
    {
        // 2) 없으면 리신 투사체 프로토타입으로 생성해서 리신 전용 레이어에 넣기
        if (FAILED(GI->Add_GameObject(TEXT("Prototype_GameObject_LeeSinProjectile"),
            LEVEL_GAMEPLAY, LAYER_LEESIN_PROJ, &desc)))
        {
            m_bProjectileSpawned = false;
        }
    }
}



