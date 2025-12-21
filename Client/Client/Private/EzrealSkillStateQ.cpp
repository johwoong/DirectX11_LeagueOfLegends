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

    m_bIsAnimationFinished = pModel->Play_Animation(
        fDeltaTime,
        m_pOwner->Is_AnimationLoop(pModel->Get_CurrentAnimIndex()),
        nullptr, 0.1f);

    // 이미 발사했으면 더 이상 스폰 금지
    if (!m_bProjectileSpawned)
    {
        vector<ANIMEVENT> events = pModel->Get_Events();

        for (auto& evt : events)
        {
            if (!evt.isPlay)
                continue;

            if (evt.eType != ANIMEVENT::EVENT_PROJECTILE)
                continue;

            m_bProjectileSpawned = true;

            // 손 위치
            CBone* pHand = pModel->Get_BonePtr("L_Hand");
            if (!pHand) break;

            _float4x4 pivot = pModel->Get_PivotFloat4x4();
            _matrix pivotMat = XMLoadFloat4x4(&pivot);

            _matrix HandMatrix =
                pHand->Get_CombinedTransformationMatrix() *
                pivotMat *
                m_pOwner->Get_Transform()->Get_WorldMatrix();

            HandMatrix.r[0] = XMVector4Normalize(HandMatrix.r[0]);
            HandMatrix.r[1] = XMVector4Normalize(HandMatrix.r[1]);
            HandMatrix.r[2] = XMVector4Normalize(HandMatrix.r[2]);

            _vector vHandPos = HandMatrix.r[3];

            // ? 방향 (Enter에서 만든 m_vDir 사용) - 먼저 계산!
            _vector vDir = XMLoadFloat3(&m_vDir);
            vDir = XMVectorSetY(vDir, 0.f);

            if (XMVectorGetX(XMVector3LengthSq(vDir)) < 1e-6f)
                break;

            vDir = XMVector3Normalize(vDir);
            vDir = XMVectorSetW(vDir, 0.f);

            if (!m_bCastFxSpawned)
            {
                m_bCastFxSpawned = true;

                _vector vUpMesh = -vDir;                              
                _vector vLookMesh = XMVectorSet(0.f, 1.f, 0.f, 0.f);    
                _vector vRightMesh = XMVector3Cross(vUpMesh, vLookMesh);  

                if (XMVectorGetX(XMVector3LengthSq(vRightMesh)) < 1e-6f)
                    vRightMesh = XMVectorSet(1.f, 0.f, 0.f, 0.f);

                vRightMesh = XMVector3Normalize(vRightMesh);
                vUpMesh = XMVector3Normalize(XMVector3Cross(vLookMesh, vRightMesh)); 
                vLookMesh = XMVector3Normalize(vLookMesh);

                _matrix mWorld = XMMatrixIdentity();
                mWorld.r[0] = vRightMesh;    
                mWorld.r[1] = vUpMesh;        
                mWorld.r[2] = vLookMesh;    

                // 손에서 살짝 앞으로
                _vector vPos = XMVectorSetW(vHandPos + vDir * 0.25f, 1.f);
                mWorld.r[3] = vPos;

                CEffect::PlayEffectAtLocation(TEXT("Ezreal_QWait.dat"), mWorld);
            }

            // 투사체 생성
            CProjectile::PROJECTILEDESC desc{};
            desc.eCollisionGroup = PLAYER;
            desc.eProjType = CEzrealProjectile::Q;
            desc.vInitPositon = XMVectorSetW(vHandPos, 1.f);
            desc.vTargetDir = vDir;
            desc.fVelocity = 10.f;
            desc.fDeadTime = 1.2f;
            desc.iDamage = 90;
            desc.pOwner = m_pOwner;
            desc.pTarget = nullptr;

            CGameInstance* GI = CGameInstance::GetInstance();
            if (FAILED(GI->Add_GameObject(TEXT("Prototype_GameObject_EzrealProjectile"),
                LEVEL_GAMEPLAY, TEXT("Layer_Projectile"), &desc)))
            {
                m_bProjectileSpawned = false;
                m_bCastFxSpawned = false; // ? 실패하면 FX도 다시 가능하게(선택)
            }

            break; // 한 발만
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

    if (!m_pEffects.empty())
    {
        for (auto& iter : m_pEffects)
        {
            if (iter != nullptr && iter->Get_PreDead())
                iter = nullptr;
        }
    }
    return nullptr;
}

void CEzrealSkillStateQ::Enter()
{
    __super::Enter();
    m_bCastFxSpawned = false;
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

    // 실제 투사체 방향
    XMStoreFloat3(&m_vDir, -vLook);

    m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::Q);


}


void CEzrealSkillStateQ::Exit()
{
    m_pOwner->SetUsingSkill(false);
    for (auto& iter : m_pEffects)
    {
        if (iter != nullptr && iter->Get_PreDead())
            iter = nullptr;
    }

    __super::Exit();
}


