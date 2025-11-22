#include "PlayerRunState.h"

#include "GameInstance.h"
#include "PlayerIdleState.h"
#include "PickingManager.h"
#include "Terrian.h"

using namespace Player;

CRunState::CRunState(CPlayer* pPlayer, DIRID eDir)
    : m_eDirection(eDir)
{
    m_pOwner = pPlayer;
    m_ePlayerID = m_pOwner->Get_PlayerID();
    m_vTargetPos = m_pOwner->Get_TargetPosition();
}

CPlayerState* CRunState::HandleInput()
{
    CGameInstance* pGameInstance = CGameInstance::GetInstance();

    if (pGameInstance->Mouse_Down(DIMK_RBUTTON))
    {

        _float3 vClickPos = {};

        if (CPickingManager::GetInstance()->Picking())
        { 
            vClickPos = CPickingManager::GetInstance()->Get_PickingPos();

            m_pOwner->Get_Transform()->Set_Destination(XMLoadFloat3(&vClickPos),
                m_pOwner->Get_Navigation());
        }
    }

    return nullptr;
}

CPlayerState* CRunState::Update(_float fTimeDelta)
{
    HandleInput();
    Move(fTimeDelta);
    m_pOwner->Check_Navigation();
    m_bIsAnimationFinished = m_pOwner->Get_Model()->Play_Animation(fTimeDelta, m_pOwner->Is_AnimationLoop(m_pOwner->Get_Model()->Get_CurrentAnimIndex()));
    return nullptr;
}

CPlayerState* CRunState::Late_Update(_float fTimeDelta)
{
    if (STATETYPE_END == m_eStateType)
        return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
	return nullptr;
}

void CRunState::Enter()
{

    m_eState = STATE_RUN; 
    m_eStateType = STATETYPE_START;
    switch (m_ePlayerID)
    {
    case CPlayer::EZREAL:
        if (STATETYPE_START == m_eStateType)
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CEzreal::ANIM::RUN);
        break;
    case CPlayer::RENEKTON:
        if (STATETYPE_START == m_eStateType)
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CRenekton::ANIM::RUN);
    case CPlayer::VI:
        if (STATETYPE_START == m_eStateType)
            m_pOwner->Get_Model()->Set_CurrentAnimIndex(CRenekton::ANIM::RUN);
    }
}

void CRunState::Exit()
{
}

void CRunState::Move(_float fTimeDelta)
{
       CTransform* pTr = m_pOwner->Get_Transform();
    CNavigation* pNavi = m_pOwner->Get_Navigation();

    // Transform이 스스로 경로 따라 감
    if (!pTr->Move_ByPath(fTimeDelta, pNavi))
    {
        m_eStateType = STATETYPE_END;
        return;
    }
}


