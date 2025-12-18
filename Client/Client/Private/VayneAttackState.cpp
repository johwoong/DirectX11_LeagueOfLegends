#include "VayneAttackState.h"
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

CAttackState::CAttackState(CVayne* pVayne)
{
    m_pOwner = pVayne;
}

CVayneState* CAttackState::AI_Behaviour(_float fTimeDelta)
{
    UNREFERENCED_PARAMETER(fTimeDelta);
    return nullptr;
}

CVayneState* CAttackState::Update(_float fTimeDelta)
{
    if (!m_pOwner)
        return nullptr;

    CActor* pTarget = m_pOwner->Check_FiledTarget();
    if (!pTarget)
        return new CMoveState(m_pOwner);

    float dist = m_pOwner->Target_Distance(pTarget);
    float range = m_pOwner->Get_AttackRangeWorld();

    // ?? 타겟이 포탑/건물인데, 내 앞에 우리 미니언이 없다 → 바로 도망
    if (auto pTower = dynamic_cast<CBuilding*>(pTarget))
    {
        if (!m_pOwner->Has_AllyMinionUnderEnemyTower(pTower, 15.f))
        {
            return new CReturnState(m_pOwner);
        }
    }

    if (dist > range * 1.1f)
        return new CMoveState(m_pOwner);

    CTransform* pTr = m_pOwner->Get_Transform();
    if (pTr)
    {
        _vector myPos = pTr->Get_State(CTransform::STATE_TRANSLATION);
        _vector tarPos = pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

        // ★ 미니언과 동일 컨벤션: Look = 내 위치 - 타겟 위치
        _vector vLook = XMVectorSetY(myPos - tarPos, 0.f);

        if (XMVectorGetX(XMVector3LengthSq(vLook)) > 0.0001f)
        {
            vLook = XMVector3Normalize(vLook);

            _vector vOldRight = pTr->Get_State(CTransform::STATE_RIGHT);
            float   fScale = XMVectorGetX(XMVector3Length(vOldRight));
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

                m_bProjectileSpawned = true;
            }
        }
    }

    if (m_bIsAnimFinished)
    {
        m_bProjectileSpawned = false;
        m_bIsAnimFinished = false;

        pTarget = m_pOwner->Check_FiledTarget();
        if (!pTarget)
            return new CMoveState(m_pOwner);

        float dist2 = m_pOwner->Target_Distance(pTarget);
        float range2 = m_pOwner->Get_AttackRangeWorld();

        // 사거리 완전 벗어나면 그냥 추격
        if (dist2 > range2 * 1.1f)
            return new CMoveState(m_pOwner);

        // ---------------------------
        // 여기부터 "도망 판단" 로직 강화
        // ---------------------------
        float myHpRate = m_pOwner->Get_HpPercent();

        // 타겟이 챔피언인지 체크
        CChampion* pChampTarget = dynamic_cast<CChampion*>(pTarget);
        if (pChampTarget)
        {
            float tarHpRate = pChampTarget->Get_HpPercent(); // 있으면 사용, 없으면 스킵

            int retreatRoll = rand() % 100;

            // 내가 피가 40% 이하 & 적은 60% 이상이면 → 많이 불리하니까 도망 우선
            if (myHpRate < 0.4f && tarHpRate > 0.6f)
            {
                if (retreatRoll < 70)     // 70% 확률로 뒤로 빼기
                    return new CReturnState(m_pOwner);
            }
            else if (dist2 < range2 * 0.7f)
            {
                // 너무 붙어있으면 → 기존보다 살짝 높은 확률로 회피
                if (retreatRoll < 50)
                    return new CReturnState(m_pOwner);
            }
            else
            {
                // 그 외 상황: 살짝만 빼주는 정도
                if (retreatRoll < 15)
                    return new CReturnState(m_pOwner);
            }
        }
        else
        {
            // 타겟이 미니언/건물인 경우: 이전 로직 그대로 (너무 붙었으면 조금 도망)
            if (dist2 < range2 * 0.7f)
            {
                int retreatRoll = rand() % 100;
                if (retreatRoll < 30)
                    return new CReturnState(m_pOwner);
            }
        }

        // ---------------------------
        // 안 도망치면 평타 애니 다시 설정
        // ---------------------------
        const int AttackAnims[] =
        {
            CVayne::ANIM::ATTACK1,
            CVayne::ANIM::ATTACK2,
            CVayne::ANIM::CRIT,
        };

        int r = rand() % 10;
        int animIdx = 0;
        if (r < 4)      animIdx = AttackAnims[0];
        else if (r < 8) animIdx = AttackAnims[1];
        else            animIdx = AttackAnims[2];

        m_pOwner->Get_Model()->Set_CurrentAnimIndex(animIdx);
    }
    return nullptr;
}

CVayneState* CAttackState::LateUpdate(_float fTimeDelta)
{
    return nullptr;
}

void CAttackState::Enter()
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

void CAttackState::Exit()
{
    m_bProjectileSpawned = false;
    m_bIsAnimFinished = false;
}