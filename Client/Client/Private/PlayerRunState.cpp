#include "PlayerRunState.h"

#include "GameInstance.h"
#include "PlayerIdleState.h"

using namespace Player;

CRunState::CRunState(CPlayer* pPlayer, DIRID eDir)
	: m_eDirection(eDir)
{
	m_pOwner = pPlayer;
	m_ePlayerID = m_pOwner->Get_PlayerID();
}

CPlayerState* CRunState::HandleInput()
{
	return nullptr;
}

CPlayerState* CRunState::Update(_float fTimeDelta)
{
	Move(fTimeDelta);
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
	__super::Enter();
	m_eState = STATE::STATE_RUN;
}

void CRunState::Exit()
{
}

void CRunState::Move(_float fTimeDelta)
{
	CTransform* pPlayerTransform = m_pOwner->Get_Transform();

	_vector vPlayerScale, vPlayerRotQuat, vPlayerPos;
	_vector vNewCamearScale, vNewCameraRot, vNewCameraPos;

	XMMatrixDecompose(&vPlayerScale, &vPlayerRotQuat, &vPlayerPos, pPlayerTransform->Get_WorldMatrix());

	_matrix vPlayerWorld = XMMatrixScalingFromVector(vPlayerScale) * XMMatrixRotationQuaternion(vPlayerRotQuat) * XMMatrixTranslationFromVector(vPlayerPos);

	pPlayerTransform->Set_State(CTransform::STATE_RIGHT, vPlayerWorld.r[0]);
	pPlayerTransform->Set_State(CTransform::STATE_UP, vPlayerWorld.r[1]);
	pPlayerTransform->Set_State(CTransform::STATE_LOOK, vPlayerWorld.r[2]);
	pPlayerTransform->Set_State(CTransform::STATE_TRANSLATION, vPlayerWorld.r[3]);
}
