#include "EzrealSkillStateQ.h"
#include "Ezreal.h"
#include "GameInstance.h"
#include "PlayerIdleState.h"
#include "PlayerRunState.h"
#include "CursorManager.h"

using namespace Player;

CEzrealSkillStateQ::CEzrealSkillStateQ(CPlayer* pPlayer)
{
    m_pOwner = pPlayer;
    m_eState = STATE_SKILL_ATTACK_Q;
}


CPlayerState* CEzrealSkillStateQ::HandleInput()
{
    return nullptr;
}

CPlayerState* CEzrealSkillStateQ::Update(_float fDeltaTime)
{
    CModel* pModel = m_pOwner->Get_Model();

    m_bIsAnimationFinished = m_pOwner->Get_Model()->Play_Animation(fDeltaTime, m_pOwner->Is_AnimationLoop(m_pOwner->Get_Model()->Get_CurrentAnimIndex()), nullptr, 0.1f);

    // ===============================
    // 2) 애니메이션 이벤트 체크
    // ===============================
    vector<ANIMEVENT> events = pModel->Get_Events();

    for (auto& evt : events)
    {
        if (!evt.isPlay) continue;
        if (m_bProjectileSpawned) continue;

        if (evt.eType == ANIMEVENT::EVENT_PROJECTILE)
        {
            m_bProjectileSpawned = true;

            CGameInstance* GI = CGameInstance::GetInstance();

            CBone* pHand = pModel->Get_BonePtr("L_Hand");

            _float4x4 pivot = pModel->Get_PivotFloat4x4();
            _matrix pivotMat = XMLoadFloat4x4(&pivot);

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
            desc.fSpeed = 55.f;                    
            desc.fMaxDistance = 1150.f;              
            desc.fLifeTime = 2.f;

            GI->Add_GameObject(
                TEXT("Prototype_GameObject_Projectile"),
                LEVEL_GAMEPLAY,
                L"Layer_Projectile",
                &desc
            );
        }
    }

    if (m_bIsAnimationFinished)
    {
        m_pOwner->Set_PlayIdleQ(true);

        return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
    }


    return nullptr;
}

CPlayerState* CEzrealSkillStateQ::Late_Update(_float fDeltaTime)
{
    return nullptr;
}

void CEzrealSkillStateQ::Enter()
{
    __super::Enter();
    m_pOwner->SetUsingSkill(true);
    CCursorManager* CM = CCursorManager::GetInstance();

    _vector rayPos = CM->Get_RayPos();
    _vector rayDir = CM->Get_RayDir();

    _vector myPos = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
    float y = XMVectorGetY(myPos);

    float t = (y - XMVectorGetY(rayPos)) / XMVectorGetY(rayDir);
    _vector hit = rayPos + rayDir * t;

    _vector vDir = XMVector3Normalize(
        XMVectorSetY(hit - myPos, 0.f)
    );

    _vector vLook = -vDir; 

    _vector vUp = XMVectorSet(0, 1, 0, 0);
    _vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vLook));
    vUp = XMVector3Cross(vLook, vRight);

    m_pOwner->Get_Transform()->Set_State(CTransform::STATE_RIGHT, vRight);
    m_pOwner->Get_Transform()->Set_State(CTransform::STATE_UP, vUp);
    m_pOwner->Get_Transform()->Set_State(CTransform::STATE_LOOK, vLook);

    XMStoreFloat3(&m_vDir, -vLook);   



    m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::Q);
}


void CEzrealSkillStateQ::Exit()
{
    m_pOwner->SetUsingSkill(false);
    __super::Exit();
}


