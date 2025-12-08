#include "EzrealSkillStateW.h"
#include "Ezreal.h"
#include "GameInstance.h"
#include "PlayerIdleState.h"
#include "PlayerRunState.h"
#include "CursorManager.h"

using namespace Player;

using namespace Player;


CEzrealSkillStateW::CEzrealSkillStateW(CPlayer* pPlayer,
    CAST_MODE eMode,
    const _float3* pTargetPos)
{
    m_pOwner = pPlayer;
    m_eState = STATE_SKILL_ATTACK_W;

    m_bProjectileSpawned = false;
    m_bIdleWPhase = false;
    m_bIsAnimationFinished = false;

    m_eCastMode = eMode;
    m_bMoveStarted = false;

    if (pTargetPos)
        m_vRunTarget = *pTargetPos;
    else
        m_vRunTarget = pPlayer->Get_TargetPosition(); // 기본값
}


CPlayerState* CEzrealSkillStateW::HandleInput()
{
    return nullptr;
}

CPlayerState* CEzrealSkillStateW::Update(_float fDeltaTime)
{
    CModel* pModel = m_pOwner->Get_Model();

    const _float fBlend_Default = 0.10f;

    // ============================
    // ① 서 있을 때 W  (CAST_IDLE)
    // ============================
    if (m_eCastMode == CAST_IDLE)
    {
        const _float fAnimSpeed_W = 1.2f;
        const _float fAnimSpeed_IdleW = 1.6f;
        const _float fBlend_WtoIdleW = 0.05f;

        // 1단계 : W
        if (!m_bIdleWPhase)
        {
            m_bIsAnimationFinished = pModel->Play_Animation(
                fDeltaTime * fAnimSpeed_W,
                false,
                nullptr,
                fBlend_Default
            );

            vector<ANIMEVENT> events = pModel->Get_Events();
            for (auto& evt : events)
            {
                if (!evt.isPlay)          continue;
                if (m_bProjectileSpawned) break;

                if (evt.eType == ANIMEVENT::EVENT_PROJECTILE)
                {
                    m_bProjectileSpawned = true;

                    CGameInstance* GI = CGameInstance::GetInstance();
                    CBone* pHand = pModel->Get_BonePtr("L_Hand");

                    _float4x4 pivot = pModel->Get_PivotFloat4x4();
                    _matrix   pivotMat = XMLoadFloat4x4(&pivot);

                    _matrix HandMatrix =
                        pHand->Get_CombinedTransformationMatrix() *
                        pivotMat *
                        m_pOwner->Get_Transform()->Get_WorldMatrix();

                    HandMatrix.r[0] = XMVector4Normalize(HandMatrix.r[0]);
                    HandMatrix.r[1] = XMVector4Normalize(HandMatrix.r[1]);
                    HandMatrix.r[2] = XMVector4Normalize(HandMatrix.r[2]);

                    _float3 vStartPos;
                    XMStoreFloat3(&vStartPos, HandMatrix.r[3]);

                    CProjectile::PROJECTILEDESC desc{};
                    desc.eType = CProjectile::PROJ_LINEAR;
                    desc.vStartPos = XMLoadFloat3(&vStartPos);
                    desc.vDir = m_vDir;
                    desc.pOwner = m_pOwner;
                    desc.fSpeed = 20.f;
                    desc.fMaxDistance = 100.f;
                    desc.fLifeTime = 2.f;
                    desc.fDamage = 80.f;

                    desc.bUseCustomCollider = true;
                    desc.vColliderScale = _float3(0.4f, 0.5f, 1.f);
                    desc.eSkillTag = CProjectile::SKILL_EZREAL_W;

                    GI->Add_GameObject(
                        TEXT("Prototype_GameObject_Projectile"),
                        LEVEL_GAMEPLAY,
                        L"Layer_Projectile",
                        &desc
                    );
                }
            }

            // W 끝 → IDLEW로
            if (m_bIsAnimationFinished)
            {
                m_bIsAnimationFinished = false;
                m_bIdleWPhase = true;

                pModel->Set_CurrentAnimIndex(CEzreal::ANIM::IDLEW);
            }
        }
        // 2단계 : IDLEW → Idle
        else
        {
            m_bIsAnimationFinished = pModel->Play_Animation(
                fDeltaTime * fAnimSpeed_IdleW,
                false,
                nullptr,
                fBlend_WtoIdleW
            );

            if (m_bIsAnimationFinished)
            {
                m_bIsAnimationFinished = false;
                return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
            }
        }

        return nullptr;
    }

    // ============================
    // ② 달리다가 W  (CAST_RUN) : RUNW
    // ============================
    const _float fAnimSpeed_RunW = 1.6f;

    m_bIsAnimationFinished = pModel->Play_Animation(
        fDeltaTime * fAnimSpeed_RunW,
        false,
        nullptr,
        fBlend_Default      // Run → RUNW 보간
    );

    vector<ANIMEVENT> events = pModel->Get_Events();
    for (auto& evt : events)
    {
        if (!evt.isPlay) continue;

        // 1) 발사
        if (evt.eType == ANIMEVENT::EVENT_PROJECTILE && !m_bProjectileSpawned)
        {
            m_bProjectileSpawned = true;

            CGameInstance* GI = CGameInstance::GetInstance();
            CBone* pHand = pModel->Get_BonePtr("L_Hand");

            _float4x4 pivot = pModel->Get_PivotFloat4x4();
            _matrix   pivotMat = XMLoadFloat4x4(&pivot);

            _matrix HandMatrix =
                pHand->Get_CombinedTransformationMatrix() *
                pivotMat *
                m_pOwner->Get_Transform()->Get_WorldMatrix();

            HandMatrix.r[0] = XMVector4Normalize(HandMatrix.r[0]);
            HandMatrix.r[1] = XMVector4Normalize(HandMatrix.r[1]);
            HandMatrix.r[2] = XMVector4Normalize(HandMatrix.r[2]);

            _float3 vStartPos;
            XMStoreFloat3(&vStartPos, HandMatrix.r[3]);

            CProjectile::PROJECTILEDESC desc{};
            desc.eType = CProjectile::PROJ_LINEAR;
            desc.vStartPos = XMLoadFloat3(&vStartPos);
            desc.vDir = m_vDir;
            desc.pOwner = m_pOwner;
            desc.fSpeed = 20.f;
            desc.fMaxDistance = 10.f;
            desc.fLifeTime = 2.f;

            GI->Add_GameObject(
                TEXT("Prototype_GameObject_Projectile"),
                LEVEL_GAMEPLAY,
                L"Layer_Projectile",
                &desc
            );
        }
        // 2) 이동 시작 이벤트
        //   ?? RUNW 클립에 "EVENT_INPUT" 타입 이벤트를 달아둔 위치에서 호출됨
        else if (evt.eType == ANIMEVENT::EVENT_INPUT && !m_bMoveStarted)
        {
            m_bMoveStarted = true;

            CTransform* pTr = m_pOwner->Get_Transform();
            CNavigation* pNavi = m_pOwner->Get_Navigation();

            // 다시 예전에 클릭했던 목적지로 경로 설정
            pTr->Set_Destination(XMLoadFloat3(&m_vRunTarget), pNavi);
        }
    }

    // EVENT_INPUT 이후엔 여기서 직접 이동
    if (m_bMoveStarted)
    {
        CTransform* pTr = m_pOwner->Get_Transform();
        CNavigation* pNavi = m_pOwner->Get_Navigation();

        if (!pTr->Move_ByPath(fDeltaTime, pNavi))
        {
            // 목적지 도착 → Idle 로
            return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
        }

        m_pOwner->Check_Navigation();
    }

    // RUNW 애니 끝
    if (m_bIsAnimationFinished)
    {
        m_bIsAnimationFinished = false;

        if (m_bMoveStarted)
            return new CRunState(m_pOwner);     // 계속 달려야 하니까 RunState로
        else
            return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
    }

    return nullptr;
}

CPlayerState* CEzrealSkillStateW::Late_Update(_float fDeltaTime)
{
    return nullptr;
}

void CEzrealSkillStateW::Enter()
{
    __super::Enter();
    m_pOwner->SetUsingSkill(true);

    m_bProjectileSpawned = false;
    m_bIdleWPhase = false;
    m_bIsAnimationFinished = false;
    m_bMoveStarted = false;

    // --- 방향 세팅 (공통) ---
    CCursorManager* CM = CCursorManager::GetInstance();

    _vector rayPos = CM->Get_RayPos();
    _vector rayDir = CM->Get_RayDir();

    _vector myPos = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
    float   y = XMVectorGetY(myPos);

    float   t = (y - XMVectorGetY(rayPos)) / XMVectorGetY(rayDir);
    _vector hit = rayPos + rayDir * t;

    _vector vDir = XMVector3Normalize(
        XMVectorSetY(hit - myPos, 0.f)
    );

    _vector vLook = -vDir;
    _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    _vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vLook));
    vUp = XMVector3Cross(vLook, vRight);

    m_pOwner->Get_Transform()->Set_State(CTransform::STATE_RIGHT, vRight);
    m_pOwner->Get_Transform()->Set_State(CTransform::STATE_UP, vUp);
    m_pOwner->Get_Transform()->Set_State(CTransform::STATE_LOOK, vLook);

    XMStoreFloat3(&m_vDir, -vLook);

    // 어떤 애니부터 시작할지
    if (m_eCastMode == CAST_IDLE)
        m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::W);
    else
        m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::RUNW);
}

void CEzrealSkillStateW::Exit()
{
    m_pOwner->SetUsingSkill(false);
    __super::Exit();
}

