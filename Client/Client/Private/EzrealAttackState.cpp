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

    // 프로그램 어디선가 한 번만 해도 됨 (WinMain 같은 곳)
    static bool s_bSeeded = false;
    if (!s_bSeeded)
    {
        srand((unsigned int)time(nullptr));
        s_bSeeded = true;
    }
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

    if (dist > range)
    {
        _float3 go;
        XMStoreFloat3(&go, tarPos);
        m_pOwner->Set_TargetPosition(go);
        return new CRunState(m_pOwner);
    }

    _vector lookDir = tarPos - myPos;  // 올바른 방향
    lookDir = XMVectorSetY(lookDir, 0.f);
    lookDir = XMVector3Normalize(lookDir);

    m_pOwner->Get_Transform()->SmoothLookAt(-lookDir, fTimeDelta, 8.f);

    // 애니메이션 진행
    m_bIsAnimationFinished = m_pOwner->Get_Model()->Play_Animation(
        fTimeDelta,
        m_pOwner->Is_AnimationLoop(m_pOwner->Get_Model()->Get_CurrentAnimIndex())
    );

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

    if (m_pOwner->Get_PlayerID() == CPlayer::EZREAL)
    {
        // 사용할 공격 애니메이션 목록
        const int AttackAnims[] =
        {
            CEzreal::ANIM::ATTACK1,
            CEzreal::ANIM::ATTACK2,
            CEzreal::ANIM::ATTACK3,
            CEzreal::ANIM::ATTACK4,
            CEzreal::ANIM::CRIT,
        };

        const int count = sizeof(AttackAnims) / sizeof(int);
        int idx = rand() % count;   // 0 ~ count-1

        m_pOwner->Get_Model()->Set_CurrentAnimIndex(AttackAnims[idx]);
    }

}

void CEzrealAttackState::Exit()
{
    m_pOwner->Set_AttackTarget(nullptr);
    __super::Exit();
}
