#include "RenektonIdleState.h"

using namespace Renekton;

CIdleState::CIdleState(CRenekton* pRenekton)
{
	m_pOwner = pRenekton;

	m_fTimeDeltaAcc = 0;
	m_fIdleTime = ((rand() % 10000) * 0.001f) * ((rand() % 100) * 0.01f);
}

CRenektonState* CIdleState::AI_Behaviour(_float fTimeDelta)
{
	return nullptr;
}

CRenektonState* CIdleState::Update(_float fTimeDelta)
{
	m_bIsAnimationFinished = m_pOwner->Get_Model()->Play_Animation(fTimeDelta, m_pOwner->Is_AnimationLoop(m_pOwner->Get_Model()->Get_CurrentAnimIndex()));
	return nullptr;
}

CRenektonState* CIdleState::LateUpdate(_float fTimeDelta)
{
	m_pOwner->Check_Navigation();
	m_fTimeDeltaAcc += fTimeDelta;
	return nullptr;
}


void CIdleState::Enter()
{
	m_eStateId = STATE_ID::STATE_IDLE;

	m_pOwner->Get_Model()->Set_CurrentAnimIndex(CRenekton::ANIM::IDLE);
}

void CIdleState::Exit()
{

}