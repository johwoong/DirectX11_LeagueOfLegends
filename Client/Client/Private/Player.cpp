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

	CNavigation::NAVDESC NaviDesc;
	ZeroMemory(&NaviDesc, sizeof NaviDesc);
	if (FAILED(__super::Add_Components(TEXT("Com_Navigation"), LEVEL_STATIC, TEXT("Prototype_Component_ChampionNavigation"), (CComponent**)&m_pNavigationCom, &NaviDesc)))
		return E_FAIL;

	// 정상 코드
	_vector vPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
	vPos = XMVectorSetZ(vPos, XMVectorGetZ(vPos) - 4.f);
	vPos = XMVectorSetX(vPos, XMVectorGetX(vPos) - 2.f);
	m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vPos);

	// 반드시 셀 기준 업데이트
	m_pNavigationCom->Compute_CurrentIndex_byXZ(vPos);

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
	if (!m_pRendererCom)
		return;

	m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_NOALPHABLEND, this);

#ifdef _DEBUG
	m_pRendererCom->Add_Debug(m_pNavigationCom);
	__super::Late_Update(fTimeDelta);
#endif
	Late_Update_State(fTimeDelta);
}

HRESULT CPlayer::Render_ShadowDepth()
{
	return S_OK;
}

HRESULT CPlayer::Render_EdgeDetection()
{
	return S_OK;
}

void CPlayer::Reset_State()
{
	Safe_Delete(m_pPlayerState);
	m_pPlayerState = new Player::CIdleState(this, Client::Player::CIdleState::IDLE_MAIN);
}

void CPlayer::HandleInput()
{
	CGameInstance* pGI = CGameInstance::GetInstance();

	// ★ A 키 토글 : 활성 챔피언만 처리
	if (pGI->Key_Down(DIK_A))
	{
		CChampion* pChamp = dynamic_cast<CChampion*>(this);
		if (pChamp)
			pChamp->Toggle_AttackRange();
	}

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

void CPlayer::Check_Navigation()
{
	_vector vPosition = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
	_float m_fWlakingHeight = m_pNavigationCom->Compute_Height(vPosition, 0.f);

	vPosition = XMVectorSetY(vPosition, m_fWlakingHeight);
	m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vPosition);
}

void CPlayer::Compute_CurrentIndex()
{
	m_pNavigationCom->Compute_CurrentIndex_byXZ(m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION));
}




void CPlayer::Free()
{
	__super::Free();
	Safe_Delete(m_pPlayerState);
	Safe_Release(m_pPlayerManager);
}


