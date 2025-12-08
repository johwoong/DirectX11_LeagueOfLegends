#include "Player.h"
#include "GameInstance.h"

#include "PlayerState.h"
#include "PlayerIdleState.h"
#include "Collision_Manager.h"

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

	HANDLE hFile = 0;
	_ulong dwByte = 0;

	NONANIMDESC PlayerDeSC;
	_uint iNum = 0;

	hFile = CreateFile(TEXT("../../../Bin/Data/PositionData/PlayerSpawnPos.dat"), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (0 == hFile)
		return E_FAIL;

	ReadFile(hFile, &(iNum), sizeof(_uint), &dwByte, nullptr);

	for (_uint i = 0; i < iNum; ++i)
	{
		ReadFile(hFile, &(PlayerDeSC), sizeof(NONANIMDESC), &dwByte, nullptr);
	}
	CloseHandle(hFile);

	

	// 1) 스폰 좌표 저장
	m_vSpawnPos = PlayerDeSC.vPosition;

	// 2) 반드시 w = 1.f 로 만들어서 Set_State
	_vector vPos = XMVectorSet(m_vSpawnPos.x, m_vSpawnPos.y, m_vSpawnPos.z, 1.f);
	m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vPos);

	// 3) 네비 인덱스 & 높이 보정 (있으면)
	m_pNavigationCom->Compute_CurrentIndex_byXZ(vPos);
	Check_Navigation();    // 지형 높이 맞추고 싶으면 여기서

	m_pPlayerState = new Player::CIdleState(this, Client::Player::CIdleState::IDLE_MAIN);
	CCollision_Manager::GetInstance()->Add_CollisionGroup(CCollision_Manager::COLLISION_PLAYER, this);
	return S_OK;
}

int CPlayer::Update(_float fTimeDelta)
{
	int iEvent = __super::Update(fTimeDelta);
	Update_Collider();
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

void CPlayer::Respawon()
{
	_vector vPos = XMVectorSet(m_vSpawnPos.x, m_vSpawnPos.y, m_vSpawnPos.z, 1.f);
	m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vPos);

	m_pNavigationCom->Compute_CurrentIndex_byXZ(vPos);
	Check_Navigation();   // 필요하면
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


