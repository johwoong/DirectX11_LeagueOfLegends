#include "EzrealSkillStateR.h"
#include "Ezreal.h"
#include "GameInstance.h"
#include "PlayerIdleState.h"
#include "PlayerRunState.h"
#include "CursorManager.h"
#include "Bow.h"
#include "GameInstance.h"

using namespace Player;

CEzrealSkillStateR::CEzrealSkillStateR(CPlayer* pPlayer)
{
    m_pOwner = pPlayer;
    m_eState = STATE_SKILL_ATTACK_R;
}

CPlayerState* CEzrealSkillStateR::HandleInput()
{
    return nullptr;
}

CPlayerState* CEzrealSkillStateR::Update(_float fDeltaTime)
{
    CModel* pModel = m_pOwner->Get_Model();
    if (!pModel)
        return nullptr;

    // =========================
    // 0) 현재 애니 재생
    // =========================
    const _uint curAnim = pModel->Get_CurrentAnimIndex();
    const bool bLoop = m_pOwner->Is_AnimationLoop(curAnim);

    m_bIsAnimationFinished = pModel->Play_Animation(
        fDeltaTime,
        bLoop,
        nullptr, 0.1f);

    // =========================
    // 1) 투사체 스폰(처음 1회)
    //    ※ R 애니 재생 중일 때만 쏘게 하고 싶으면
    //       if(curAnim == CEzreal::ANIM::R) 조건 추가하면 됨
    // =========================
    if (!m_bProjectileSpawned)
    {
        const vector<ANIMEVENT> events = pModel->Get_Events();

        for (const auto& evt : events)
        {
            if (!evt.isPlay) continue;
            if (evt.eType != ANIMEVENT::EVENT_PROJECTILE) continue;

            m_bProjectileSpawned = true;

            CBone* pHand = pModel->Get_BonePtr("L_Ring1");
            if (!pHand) { m_bProjectileSpawned = false; break; }

            _float4x4 pivot = pModel->Get_PivotFloat4x4();
            _matrix pivotMat = XMLoadFloat4x4(&pivot);

            _matrix handWorld =
                pHand->Get_CombinedTransformationMatrix() *
                pivotMat *
                m_pOwner->Get_Transform()->Get_WorldMatrix();

            handWorld.r[0] = XMVector3Normalize(handWorld.r[0]);
            handWorld.r[1] = XMVector3Normalize(handWorld.r[1]);
            handWorld.r[2] = XMVector3Normalize(handWorld.r[2]);

            _vector vHandPos = XMVectorSetW(handWorld.r[3], 1.f);

            // 방향(수평)
            _vector vBowDir = XMLoadFloat3(&m_vDir);
            vBowDir = XMVectorSetY(vBowDir, 0.f);

            if (XMVectorGetX(XMVector3LengthSq(vBowDir)) < 1e-6f)
                vBowDir = XMVectorSet(0.f, 0.f, 1.f, 0.f);

            vBowDir = XMVector3Normalize(vBowDir);
            vBowDir = XMVectorSetW(vBowDir, 0.f);

            // ? 방향 반대면 고정 반전
            vBowDir = -vBowDir;

            // 위치 오프셋(네가 조절)
            float forward = 1.f;
            float side = 0.f;
            float down = 2.0f;

            _vector worldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
            _vector vRightDir = XMVector3Normalize(XMVector3Cross(worldUp, vBowDir));
            if (XMVectorGetX(XMVector3LengthSq(vRightDir)) < 1e-6f)
                vRightDir = XMVectorSet(1.f, 0.f, 0.f, 0.f);

            _vector vSpawn = vHandPos;
            vSpawn += vBowDir * forward;
            vSpawn += vRightDir * side;
            vSpawn += worldUp * (-down);
            vSpawn = XMVectorSetW(vSpawn, 1.f);

            // 생성
            CBow::BOWDESC desc{};
            desc.pOwner = m_pOwner;
            desc.vPos = vSpawn;
            desc.vDir = vBowDir;
            CGameInstance* pGameInstance = CGameInstance::GetInstance();
            pGameInstance->PlaySounds(const_cast<TCHAR*>(L"EzrealRTalk.wav"), SOUND_PLAYERTALK, 0.4f);
            pGameInstance->PlaySounds(const_cast<TCHAR*>(L"EzrealRRedaySound.wav"), SOUND_PLAYER, 0.4f);
            CGameObject* pObj = nullptr;
            if (FAILED(CGameInstance::GetInstance()->Add_GameObject_Out(
                TEXT("Prototype_GameObject_Bow"),
                LEVEL_GAMEPLAY,
                TEXT("Layer_Weapon"),
                pObj,
                &desc)))
            {
                m_bProjectileSpawned = false;
            }

            break;
        }
    }

    // =========================
    // 2) R -> R_IDLE -> IDLE 자연 전환
    // =========================
    if (m_bIsAnimationFinished)
    {
        // (A) 지금 R 재생 끝났으면 -> R_IDLE로 전환 시작
        if (!m_bPlayRIdle && curAnim == CEzreal::ANIM::R)
        {
            m_bPlayRIdle = true;

            pModel->Set_CurrentAnimIndex(CEzreal::ANIM::R_IDLE);
            pModel->Set_TimeReset(); // 있으면 확실히
            return nullptr; // 이 프레임은 여기서 끝
        }

        // (B) R_IDLE 끝났으면 -> IdleState로
        if (m_bPlayRIdle && curAnim == CEzreal::ANIM::R_IDLE)
        {
            return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
        }
    }

    return nullptr;
}

CPlayerState* CEzrealSkillStateR::Late_Update(_float fDeltaTime)
{
    return nullptr;
}

void CEzrealSkillStateR::Enter()
{
    __super::Enter();
    m_pOwner->SetUsingSkill(true);

    m_bProjectileSpawned = false;
    m_bPlayRIdle = false; // ? 추가

    CCursorManager* CM = CCursorManager::GetInstance();

    _vector rayPos = CM->Get_RayPos();
    _vector rayDir = CM->Get_RayDir();

    _vector myPos = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
    float y = XMVectorGetY(myPos);

    float t = (y - XMVectorGetY(rayPos)) / XMVectorGetY(rayDir);
    _vector hit = rayPos + rayDir * t;

    _vector vDir = XMVector3Normalize(XMVectorSetY(hit - myPos, 0.f));
    _vector vLook = -vDir;

    _vector vUp = XMVectorSet(0, 1, 0, 0);
    _vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vLook));
    vUp = XMVector3Cross(vLook, vRight);

    m_pOwner->Get_Transform()->Set_State(CTransform::STATE_RIGHT, vRight);
    m_pOwner->Get_Transform()->Set_State(CTransform::STATE_UP, vUp);
    m_pOwner->Get_Transform()->Set_State(CTransform::STATE_LOOK, vLook);

    // 마우스 방향 저장
    XMStoreFloat3(&m_vDir, -vLook);

    // 애니 실행
    m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::R);
    m_pOwner->Get_Model()->Set_TimeReset(); // 있으면 추천
}

void CEzrealSkillStateR::Exit()
{
    m_pOwner->SetUsingSkill(false);
    __super::Exit();
}

