#include "EzrealAttackState.h"

#include "GameInstance.h"
#include "Champion.h"
#include "Ezreal.h"
#include "Enemy.h"
#include "PickingManager.h"

#include "PlayerIdleState.h"
#include "PlayerRunState.h"

USING(Client)
USING(Player)

CEzrealAttackState::CEzrealAttackState(CPlayer* pPlayer, STATE_ID eStateType, _float /*fTime*/)
{
    m_pOwner = pPlayer;
    m_eState = eStateType;
    m_ePlayerID = m_pOwner->Get_PlayerID();
}

// 입력은 최소만 처리 (우클릭으로 이동 명령 등)
CPlayerState* CEzrealAttackState::HandleInput()
{
    CGameInstance* GI = CGameInstance::GetInstance();
    CPickingManager* PM = CPickingManager::GetInstance();

    // 공격 중 우클릭 → 즉시 이동
    if (GI->Mouse_Down(DIMK_RBUTTON))
    {
        if (!PM->Picking())
            return nullptr;

        _float3 clickPos = PM->Get_PickingPos();
        CGameObject* pickedObj = PM->Get_PickedObj();

        // Enemy 클릭 → 타겟만 변경
        if (CEnemy* enemy = dynamic_cast<CEnemy*>(pickedObj))
        {
            m_pOwner->Set_AttackTarget(enemy);
        }
        else
        {
            m_pOwner->Set_AttackTarget(nullptr);
        }

        // ★ 우클릭하면 무조건 이동. AttackState 강제 종료됨.
        m_pOwner->Set_TargetPosition(clickPos);
        return new CRunState(m_pOwner);
    }

    return nullptr;
}

CPlayerState* CEzrealAttackState::Update(_float fTimeDelta)
{
    CActor* target = m_pOwner->Get_AttackTarget();

    // 타겟 없음 → 종료
    if (!target || target->Get_Dead())
        return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);

    _vector myPos = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
    _vector tarPos = target->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

    float dist = GetDistanceXZ(myPos, tarPos);
    float range = dynamic_cast<CChampion*>(m_pOwner)->Get_AttackRange();

    // ?? 사거리 밖 → 추격
    if (dist > range)
    {
        _float3 go;
        XMStoreFloat3(&go, tarPos);
        m_pOwner->Set_TargetPosition(go);
        return new CRunState(m_pOwner);
    }

    // ?? 공격 방향 정상화 (반전 버그 수정)
    _vector lookDir = tarPos - myPos;  // 올바른 방향
    lookDir = XMVectorSetY(lookDir, 0.f);
    lookDir = XMVector3Normalize(lookDir);

    m_pOwner->Get_Transform()->SmoothLookAt(-lookDir, fTimeDelta, 8.f);

    // 애니메이션 진행
    m_bIsAnimationFinished = m_pOwner->Get_Model()->Play_Animation(
        fTimeDelta,
        m_pOwner->Is_AnimationLoop(m_pOwner->Get_Model()->Get_CurrentAnimIndex())
    );

    // ?? 공격 애니 끝 → Idle
    if (m_bIsAnimationFinished)
        return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);

    return nullptr;
}

CPlayerState* CEzrealAttackState::Late_Update(_float /*fTimeDelta*/)
{
    return nullptr;
}

void CEzrealAttackState::Enter()
{
    __super::Enter();
    m_eState = STATE_ID::STATE_ATTACK1;

    // Ezreal 평타 애니메이션으로 변경 (프로젝트에 맞게 수정)
    if (m_pOwner->Get_PlayerID() == CPlayer::EZREAL)
    {
        m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::ATTACK1);
    }

}

void CEzrealAttackState::Exit()
{
    __super::Exit();
    // AttackState에서 나갈 때 별도 처리 필요하면 여기
}
