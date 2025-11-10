#include "Player.h"
#include "GameInstance.h"

#include "PlayerState.h"
#include "PlayerIdleState.h"

using namespace Player;

CPlayer::CPlayer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CActor(pDevice, pContext)
{

}

CPlayer::CPlayer(const CPlayer& rhs)
	: CActor(rhs)
{
}

HRESULT CPlayer::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CPlayer::Initialize(void* pArg)
{
	if (FAILED(Ready_Components(pArg)))
		return E_FAIL;

	m_pPlayerState = new Player::CIdleState(this, Client::Player::CIdleState::IDLE_MAIN);	
	
	return S_OK;
}

int CPlayer::Update(_float fTimeDelta)
{
	int iEvent = __super::Update(fTimeDelta);

	if (iEvent == OBJ_DEAD)
		return OBJ_DEAD;

	HandleInput();
	Update_State(fTimeDelta);
	return OBJ_NOEVENT;
}

void CPlayer::Late_Update(_float fTimeDelta)
{
	Late_Update_State(fTimeDelta);

	if (m_pRendererCom != nullptr)
	{
		m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_NOALPHABLEND, this);
	}
}

void CPlayer::Reset_State()
{
	Safe_Delete(m_pPlayerState);
	m_pPlayerState = new Player::CIdleState(this, Client::Player::CIdleState::IDLE_MAIN);
}

void CPlayer::HandleInput()
{
	if (!m_pPlayerState)
		return;

	CPlayerState* pNewState = m_pPlayerState->HandleInput();
	if (pNewState)
		m_pPlayerState = m_pPlayerState->ChangeState(m_pPlayerState, pNewState);
}

void CPlayer::Update_State(_float fTimeDelta)
{
	if (!m_pPlayerState)
		return;

	CPlayerState* pNewState = m_pPlayerState->Update(fTimeDelta);
	if (pNewState)
		m_pPlayerState = m_pPlayerState->ChangeState(m_pPlayerState, pNewState);
}

void CPlayer::Late_Update_State(_float fTimeDelta)
{
	if (!m_pPlayerState)
		return;

	CPlayerState* pNewState = m_pPlayerState->Late_Update(fTimeDelta);
	if (pNewState)
		m_pPlayerState = m_pPlayerState->ChangeState(m_pPlayerState, pNewState);
}


void CPlayer::Free()
{
	__super::Free();
	Safe_Delete(m_pPlayerState);
	Safe_Release(m_pPlayerManager);
}


