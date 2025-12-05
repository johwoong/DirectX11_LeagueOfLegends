#include "VayneIdleState.h"

using namespace Vayne;

CIdleState::CIdleState(CVayne* pVayne)
{
	m_pOwner = pVayne;

	m_fTimeDeltaAcc = 0;
	m_fIdleTime = ((rand() % 10000) * 0.001f) * ((rand() % 100) * 0.01f);
}

CVayneState* CIdleState::AI_Behaviour(_float fTimeDelta)
{
	return nullptr;
}

CVayneState* CIdleState::Update(_float fTimeDelta)
{
	m_bIsAnimationFinished = m_pOwner->Get_Model()->Play_Animation(fTimeDelta, m_pOwner->Is_AnimationLoop(m_pOwner->Get_Model()->Get_CurrentAnimIndex()));
	return nullptr;
}

CVayneState* CIdleState::LateUpdate(_float fTimeDelta)
{
	m_pOwner->Check_Navigation();
	m_fTimeDeltaAcc += fTimeDelta;

	return nullptr;
}


void CIdleState::Enter()
{
	m_eStateId = STATE_ID::STATE_IDLE;
	

	m_pOwner->Get_Model()->Set_CurrentAnimIndex(CVayne::ANIM::IDLE);


}

void CIdleState::Exit()
{

}