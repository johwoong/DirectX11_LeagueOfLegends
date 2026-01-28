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
#include "VayneAttackState.h"

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
    m_fQMinTime -= fTimeDelta;

    CTransform* pTr = m_pOwner->Get_Transform();
    CNavigation* pNav = m_pOwner->Get_Navigation();
    CModel* pModel = m_pOwner->Get_Model();
    if (!pTr || !pNav || !pModel)
        return new CMoveState(m_pOwner);

    // 이동
    pTr->Move_ByPath(fTimeDelta * 2.5f, pNav);
    m_pOwner->Check_Navigation();

    // ?? Weapon Bone 기준 FX 위치 갱신
    if (!m_QFxList.empty())
    {
        CBone* pWeaponBone = pModel->Get_BonePtr("weapon");
        if (pWeaponBone)
        {
            _float4x4 pivot = pModel->Get_PivotFloat4x4();
            _matrix pivotMat = XMLoadFloat4x4(&pivot);

            _matrix weaponWorld =
                pWeaponBone->Get_CombinedTransformationMatrix() *
                pivotMat *
                pTr->Get_WorldMatrix();

            weaponWorld.r[0] = XMVector4Normalize(weaponWorld.r[0]);
            weaponWorld.r[1] = XMVector4Normalize(weaponWorld.r[1]);
            weaponWorld.r[2] = XMVector4Normalize(weaponWorld.r[2]);

            // 약간 전방 보정
            _vector vDir = XMVector3Normalize(
                XMVectorSetY(pTr->Get_State(CTransform::STATE_LOOK), 0.f));

            weaponWorld.r[3] = XMVectorSetW(
                weaponWorld.r[3] + vDir * 0.1f,
                1.f
            );

            // ?? 모든 FX 갱신
            for (auto& fx : m_QFxList)
            {
                if (fx)
                    fx->Set_WorldPosition(weaponWorld);
            }
        }
    }

    bool bEnd = pModel->Play_Animation(fTimeDelta, false);
    if (bEnd && m_fQMinTime <= 0.f)
        return new CAttackState(m_pOwner);

    return nullptr;
}

CVayneState* CVayneSkillStateQ::LateUpdate(_float fTimeDelta)
{
    return nullptr;
}

void CVayneSkillStateQ::Enter()
{
    m_fQMinTime = 0.18f;
    m_bQFxSpawned = false;
    m_QFxList.clear();

    m_pOwner->Play_VayneSFX(TEXT("VayneQTalk.wav"), SOUND_VAYNE_TALK, 0.70f);
    m_pOwner->Play_VayneSFX(TEXT("VayneQSound.wav"), SOUND_VAYNE, 0.70f);
   
    m_pOwner->m_bQEmpowered = true;
    m_pOwner->m_fQEmpoweredTime = m_pOwner->Q_EMPOWER_DURATION;

    CModel* pModel = m_pOwner->Get_Model();
    CTransform* pTr = m_pOwner->Get_Transform();
    if (!pModel || !pTr)
        return;

    pModel->Set_CurrentAnimIndex(CVayne::ANIM::Q);

    // ?? Q FX 1회 생성
    if (!m_bQFxSpawned)
    {
        CBone* pWeaponBone = pModel->Get_BonePtr("weapon");
        if (pWeaponBone)
        {
            _float4x4 pivot = pModel->Get_PivotFloat4x4();
            _matrix pivotMat = XMLoadFloat4x4(&pivot);

            _matrix weaponWorld =
                pWeaponBone->Get_CombinedTransformationMatrix() *
                pivotMat *
                pTr->Get_WorldMatrix();

            weaponWorld.r[0] = XMVector4Normalize(weaponWorld.r[0]);
            weaponWorld.r[1] = XMVector4Normalize(weaponWorld.r[1]);
            weaponWorld.r[2] = XMVector4Normalize(weaponWorld.r[2]);

            // ?? vector로 받는다 (이게 핵심)
            m_QFxList = CEffect::PlayEffectAtLocation(
                TEXT("Vayne_Q_Shine.dat"),
                weaponWorld
            );

            m_bQFxSpawned = true;
        }
    }

    CNavigation* pNav = m_pOwner->Get_Navigation();
    if (!pTr || !pNav)
        return;

    // =========================
    // 1. 기준 벡터
    // =========================
    _vector vLook = XMVector3Normalize(
        XMVectorSetY(pTr->Get_State(CTransform::STATE_LOOK), 0.f));
    _vector vRight = XMVector3Normalize(
        XMVectorSetY(pTr->Get_State(CTransform::STATE_RIGHT), 0.f));

    // =========================
    // 2. 방향 선택 (베인 느낌)
    // =========================
    int r = rand() % 100;
    _vector vDir;

    if (r < 40)        vDir = -vLook;       // 뒤로 (kite)
    else if (r < 65)   vDir = vRight;       // 오른쪽
    else if (r < 90)   vDir = -vRight;      // 왼쪽
    else               vDir = vLook;        // 앞으로 (가끔 공격적)

    // =========================
    // 3. 이동 거리 & 목적지
    // =========================
    const float Q_DIST = 2.7f;   // ★ 핵심 튜닝 포인트

    _vector vPos = pTr->Get_State(CTransform::STATE_TRANSLATION);
    _vector vDest = vPos + vDir * Q_DIST;

    // 네비 보정
    pTr->Set_Destination(vDest, pNav);

    // =========================
    // 4. "구르는 시간" 설정
    // =========================
    m_pOwner->m_bDoKiting = true;
    m_pOwner->m_vKitingDest = vDest;

    // 이 시간 동안은 이동 애니 + 공격 금지
    m_pOwner->m_fPostAttackMoveTime = 0.25f;
}

void CVayneSkillStateQ::Exit()
{
    for (auto& fx : m_QFxList)
    {
        if (fx)
            fx->Set_Dead(true);
    }

    m_QFxList.clear();
    m_bQFxSpawned = false;
    m_pOwner->m_fPostAttackMoveTime = 0.f;
}