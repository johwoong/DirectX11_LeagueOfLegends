#include "LeeSinAttackState.h"
#include "PlayerRunState.h"
#include "PlayerIdleState.h"
#include "LeeSin.h"
#include "GameInstance.h"
#include "Collision_Manager.h"

using namespace Player;

CLeeSinAttackState::CLeeSinAttackState(CPlayer* pPlayer)
{
    m_pOwner = pPlayer;
    m_ePlayerID = m_pOwner->Get_PlayerID();
    m_pAttackCollider = nullptr;
    m_bDamageApplied = false;
}

void CLeeSinAttackState::Enter()
{
    __super::Enter();

    m_eState = STATE_ATTACK1;
    m_bIsAnimationFinished = false;
    m_fComboTimer = 0.f;
    m_bDamageApplied = false;

    CModel* pModel = m_pOwner->Get_Model();

    const bool bLeft = m_pOwner->Is_LeeSinLeftAttack();

    // ===============================
    // 좌 / 우 공격 애니 선택
    // ===============================
    if (bLeft)
    {
        int r = rand() % 2;
        switch (r)
        {
        case 0: pModel->Set_CurrentAnimIndex(CLeeSin::ATTACK_L_PUNCH); break;
        case 1: pModel->Set_CurrentAnimIndex(CLeeSin::ATTACK_KICK);    break;
        }
    }
    else
    {
        int r = rand() % 2;
        switch (r)
        {
        case 0: pModel->Set_CurrentAnimIndex(CLeeSin::ATTACK_R_PUCNH); break;
        case 1: pModel->Set_CurrentAnimIndex(CLeeSin::ATTACK_R_KICK);  break;
        }
    }
    const int pick = rand() % 3;
    switch (pick)
    {
    case 0:
        CGameInstance::GetInstance()->PlaySounds(const_cast<TCHAR*>(L"LeeSinAttack2.wav"), SOUND_PLAYERTALK, 0.4f);
        CGameInstance::GetInstance()->PlaySounds(const_cast<TCHAR*>(L"LeeSinAttackSound.wav"), SOUND_PLAYER, 0.05f);
        break;
    case 1:
        CGameInstance::GetInstance()->PlaySounds(const_cast<TCHAR*>(L"LeeSinAttack3.wav"), SOUND_PLAYERTALK, 0.4f);
        CGameInstance::GetInstance()->PlaySounds(const_cast<TCHAR*>(L"LeeSinAttackSound2.wav"), SOUND_PLAYER, 0.05f);
        break;
    case 2:
        CGameInstance::GetInstance()->PlaySounds(const_cast<TCHAR*>(L"LeeSinAttackSound1.wav"), SOUND_PLAYERTALK, 0.4f);
        CGameInstance::GetInstance()->PlaySounds(const_cast<TCHAR*>(L"LeeSinAttack4.wav"), SOUND_PLAYER, 0.05f);
        break;
    }
    // 다음 공격은 반대편
    m_pOwner->Toggle_LeeSinAttackSide();
}

CPlayerState* CLeeSinAttackState::HandleInput()
{
    CGameInstance* GI = CGameInstance::GetInstance();
    CPickingManager* PM = CPickingManager::GetInstance();

    // 우클릭 → 이동 / 공격무브
    if (GI->Mouse_Down(DIMK_RBUTTON))
    {
        _float3 clickPos = PM->Get_PickingPos();
        CGameObject* pickedObj = PM->Get_PickedObj();

        if (CEnemy* enemy = dynamic_cast<CEnemy*>(pickedObj))
        {
            m_pOwner->Set_AttackTarget(enemy);
            Set_AttackMoveToTarget(m_pOwner, enemy);
            return new CRunState(m_pOwner);
        }
        else
        {
            m_pOwner->Set_AttackTarget(nullptr);
            m_pOwner->Set_TargetPosition(clickPos);
            m_pOwner->Get_Transform()->Set_Destination(
                XMLoadFloat3(&clickPos),
                m_pOwner->Get_Navigation());
            return new CRunState(m_pOwner);
        }
    }

    // 콤보
    if (GI->Mouse_Down(DIMK_LBUTTON))
    {
        if (m_fComboTimer < m_fComboLimit)
            return new CLeeSinAttackState(m_pOwner);
    }

    return nullptr;
}

CPlayerState* CLeeSinAttackState::Update(_float fTimeDelta)
{
    m_fComboTimer += fTimeDelta;

    CActor* pTarget = m_pOwner->Get_AttackTarget();

    // ===============================
    // 타겟 바라보기
    // ===============================
    if (pTarget && !pTarget->Get_Dead())
    {
        _vector myPos = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
        _vector tarPos = pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

        _vector dir = tarPos - myPos;
        dir = XMVectorSetY(dir, 0.f);

        if (XMVectorGetX(XMVector3LengthSq(dir)) > 0.0001f)
        {
            dir = XMVector3Normalize(dir);
            m_pOwner->Get_Transform()->SmoothLookAt(-dir, fTimeDelta, 10.f);
        }
    }

    // ===============================
    // 애니메이션 재생
    // ===============================
    const float ATTACK_SPEED = 1.35f;

    m_bIsAnimationFinished =
        m_pOwner->Get_Model()->Play_Animation(
            fTimeDelta * ATTACK_SPEED,
            false,
            nullptr,
            0.05f
        );

    // ===============================
    // ★ 애니메이션 이벤트 → 콜라이더 관리
    // ===============================
    bool bColliderEventPlaying = false;

    vector<ANIMEVENT> events = m_pOwner->Get_Model()->Get_Events();
    for (auto& evt : events)
    {
        if (evt.isPlay && evt.eType == ANIMEVENT::EVENT_COLLIDER)
        {
            bColliderEventPlaying = true;
            if (!m_pAttackCollider)
            {
                CColider::COLLIDERDESC desc{};
                desc.vScale = _float3(1.5f, 1.2f, 1.5f); // ★ 네가 조절
                desc.vRotation = _float3(0.f, 0.f, 0.f);
                desc.vPosition = _float3(0.f, 0.f, 0.f);

                m_pAttackCollider =
                    CCollision_Manager::GetInstance()->Reuse_Collider(
                        CColider::TYPE_AABB,
                        LEVEL_STATIC,
                        TEXT("Prototype_Component_Colider_AABB"),
                        &desc);
            }
            break;
        }
    }

    // 이벤트 종료 시 제거
    if (!bColliderEventPlaying && m_pAttackCollider)
    {
        CCollision_Manager::GetInstance()->Collect_Collider(
            CColider::TYPE_AABB,
            m_pAttackCollider);
        m_pAttackCollider = nullptr;
        m_bDamageApplied = false;
    }

    // ===============================
// ★ 콜라이더 위치 갱신 (공격 방향 제대로 반영)
// ===============================
    if (m_pAttackCollider)
    {
        CTransform* pTr = m_pOwner->Get_Transform();

        _vector vPos = pTr->Get_State(CTransform::STATE_TRANSLATION);
        _vector vLook = pTr->Get_State(CTransform::STATE_LOOK);

        // ★ 방향 보정 (핵심)
        vLook = -vLook;

        // 수평 방향만 사용
        vLook = XMVectorSetY(vLook, 0.f);
        vLook = XMVector3Normalize(vLook);

        _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
        _vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vLook));
        vUp = XMVector3Normalize(XMVector3Cross(vLook, vRight));

        const float ATTACK_OFFSET = 1.2f;
        _vector vAttackPos = vPos + vLook * ATTACK_OFFSET;
        vAttackPos = XMVectorSetW(vAttackPos, 1.f);

        _matrix matWorld = XMMatrixIdentity();
        matWorld.r[0] = vRight;
        matWorld.r[1] = vUp;
        matWorld.r[2] = vLook;
        matWorld.r[3] = vAttackPos;

        m_pAttackCollider->Update(matWorld);
    }
    // ===============================
    // 데미지 판정
    // ===============================
    if (m_pAttackCollider && !m_bDamageApplied && pTarget)
    {
        CColider* pTargetCol = pTarget->Get_Collider();
        if (pTargetCol && m_pAttackCollider->Collision(pTargetCol))
        {
            pTarget->Take_Damage(
                static_cast<CChampion*>(m_pOwner)->Get_Stat().AttackDamage,
                m_pOwner);

            m_bDamageApplied = true;
        }
    }

    return nullptr;
}

CPlayerState* CLeeSinAttackState::Late_Update(_float fTimeDelta)
{
#ifdef _DEBUG
    if (m_pAttackCollider)
        m_pOwner->Get_Renderer()->Add_Debug(m_pAttackCollider);
#endif

    if (!m_bIsAnimationFinished)
        return nullptr;

    return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
}

void CLeeSinAttackState::Exit()
{
    if (m_pAttackCollider)
    {
        CCollision_Manager::GetInstance()->Collect_Collider(
            CColider::TYPE_AABB,
            m_pAttackCollider);
        m_pAttackCollider = nullptr;
    }

    m_bDamageApplied = false;
    __super::Exit();
}
