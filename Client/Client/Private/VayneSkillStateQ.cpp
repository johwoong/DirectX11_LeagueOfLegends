#include "VayneSkillStateQ.h"
#include "VayneMoveState.h"
#include "VayneIdleState.h"
#include "Vayne.h"
#include "VayneReturnState.h"  
#include "GameInstance.h"
#include "Projectile.h"
#include "Champion.h"
#include "Minion.h"
#include "Building.h"

USING(Client);
using namespace Vayne;

CVayneSkillStateQ::CVayneSkillStateQ(CVayne* pVayne)
{
    m_pOwner = pVayne;
}

CVayneState* CVayneSkillStateQ::AI_Behaviour(_float fTimeDelta)
{
    if (!m_pOwner)
        return nullptr;

    // 1) 타겟 유효성 검사
    CActor* pTarget = m_pOwner->Check_FiledTarget();
    if (!pTarget)
    {
        // 타겟 없으면 라인 다시 타도록 MoveState로
        return new CMoveState(m_pOwner);
    }

    float dist = m_pOwner->Target_Distance(pTarget);
    float range = m_pOwner->Get_AttackRangeWorld();

    // 2) 사거리 완전히 벗어나면 다시 이동 (추격)
    if (dist > range * 1.1f)
    {
        return new CMoveState(m_pOwner);
    }

    return nullptr; // 계속 공격 상태 유지
}

CVayneState* CVayneSkillStateQ::Update(_float fTimeDelta)
{
    if (!m_pOwner)
        return nullptr;

    // 1) 타겟 다시 확인
    CActor* pTarget = m_pOwner->Check_FiledTarget();
    if (!pTarget)
        return new CMoveState(m_pOwner);   // ?? Idle 말고 Move

    // 2) 거리 체크 (안전)
    float dist = m_pOwner->Target_Distance(pTarget);
    float range = m_pOwner->Get_AttackRangeWorld();

    if (dist > range * 1.1f)
    {
        return new CMoveState(m_pOwner);
    }

    // 3) 타겟 바라보기
    CTransform* pTr = m_pOwner->Get_Transform();
    if (pTr)
    {
        _vector myPos = pTr->Get_State(CTransform::STATE_TRANSLATION);
        _vector tarPos = pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

        _vector lookDir = tarPos - myPos;
        lookDir = XMVectorSetY(lookDir, 0.f);
        if (XMVectorGetX(XMVector3LengthSq(lookDir)) > 0.0001f)
        {
            lookDir = XMVector3Normalize(lookDir);
            // 베인이 타겟을 바라보도록
            pTr->SmoothLookAt(-lookDir, fTimeDelta, 8.f);
        }
    }

    // 4) 공격 애니메이션 진행
    CModel* pModel = m_pOwner->Get_Model();
    m_bIsAnimFinished =
        pModel->Play_Animation(
            fTimeDelta,
            false,      // 평타 애니메이션은 루프 X
            nullptr,
            0.1f);

    // 5) 애니메이션 이벤트에서 투사체 발사
    vector<ANIMEVENT> events = pModel->Get_Events();
    for (auto& evt : events)
    {
        if (!evt.isPlay) continue;

        if (evt.eType == ANIMEVENT::EVENT_PROJECTILE)
        {
            if (!m_bProjectileSpawned)
            {
                CGameInstance* pGI = CGameInstance::GetInstance();

                CBone* pHand = m_pOwner->Get_Model()->Get_BonePtr("weapon_b");

                _float4x4 pivot = m_pOwner->Get_Model()->Get_PivotFloat4x4();
                _matrix pivotMat = XMLoadFloat4x4(&pivot);

                _matrix HandMatrix =
                    pHand->Get_CombinedTransformationMatrix() *
                    pivotMat *
                    m_pOwner->Get_Transform()->Get_WorldMatrix();

                HandMatrix.r[0] = XMVector4Normalize(HandMatrix.r[0]);
                HandMatrix.r[1] = XMVector4Normalize(HandMatrix.r[1]);
                HandMatrix.r[2] = XMVector4Normalize(HandMatrix.r[2]);

                _vector vHandVec = HandMatrix.r[3];
                CProjectile::PROJECTILEDESC desc{};
                desc.pOwner = m_pOwner;
                desc.pTarget = m_pOwner->Get_Target();
                desc.fSpeed = 4.f;
                desc.vStartPos = vHandVec;

                pGI->Add_GameObject(
                    TEXT("Prototype_GameObject_Projectile"),
                    LEVEL_GAMEPLAY,
                    L"Layer_Projectile",
                    &desc);

                m_bProjectileSpawned = true;
            }
        }
    }

    if (m_bIsAnimFinished)
    {
        m_bProjectileSpawned = false;
        m_bIsAnimFinished = false;

        // 아직 타겟 유효 + 사거리 안이면 계속 공격 루프
        pTarget = m_pOwner->Check_FiledTarget();
        if (!pTarget)
            return new CMoveState(m_pOwner);

        float dist2 = m_pOwner->Target_Distance(pTarget);
        float range2 = m_pOwner->Get_AttackRangeWorld();

        if (dist2 > range2 * 1.1f)
            return new CMoveState(m_pOwner);

        // ================================
        // ?? 10% 확률로 "도망가기(ReturnState)" 전환
        // ================================
        {
            int retreatRoll = rand() % 100;   // 0~99
            if (retreatRoll < 10)             // 10% 확률
            {
                return new CReturnState(m_pOwner);
            }
        }

        // ?? 같은 AttackState 안에서 새 평타 애니 다시 세팅 (연속 공격)
        const int AttackAnims[] =
        {
            CVayne::ANIM::ATTACK1,
            CVayne::ANIM::ATTACK2,
            CVayne::ANIM::CRIT,
        };

        int r = rand() % 10;
        int animIdx = 0;
        if (r < 4)      animIdx = AttackAnims[0];   // ATTACK1
        else if (r < 8) animIdx = AttackAnims[1];   // ATTACK2
        else            animIdx = AttackAnims[2];   // CRIT (10% 정도)

        m_pOwner->Get_Model()->Set_CurrentAnimIndex(animIdx);
    }

    return nullptr;
}

CVayneState* CVayneSkillStateQ::LateUpdate(_float fTimeDelta)
{
    return nullptr;
}

void CVayneSkillStateQ::Enter()
{
    m_bProjectileSpawned = false;
    m_bIsAnimFinished = false;

    if (!m_pOwner)
        return;

    // 첫 공격 애니 랜덤 설정
    const int AttackAnims[] =
    {
        CVayne::ANIM::ATTACK1,
        CVayne::ANIM::ATTACK2,
        CVayne::ANIM::CRIT,
    };

    int r = rand() % 10;
    int animIdx = 0;
    if (r < 4)      animIdx = AttackAnims[0];   // ATTACK1
    else if (r < 8) animIdx = AttackAnims[1];   // ATTACK2
    else            animIdx = AttackAnims[2];   // CRIT

    m_pOwner->Get_Model()->Set_CurrentAnimIndex(animIdx);
}

void CVayneSkillStateQ::Exit()
{
    m_bProjectileSpawned = false;
    m_bIsAnimFinished = false;
}