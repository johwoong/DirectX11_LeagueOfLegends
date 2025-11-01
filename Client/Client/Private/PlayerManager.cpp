#include "PlayerManager.h"
#include "Champion.h"
#include "Ezreal.h"
#include "Renekton.h"
#include "GameInstance.h"

IMPLEMENT_SINGLETON(CPlayerManager)

CPlayerManager::CPlayerManager()
{
}

HRESULT CPlayerManager::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	m_pDevice = pDevice;
	m_pContext = pContext;

	CChampion* pEzreal = CEzreal::Create(m_pDevice, m_pContext);
	CChampion* pRenekton = CRenekton::Create(m_pDevice, m_pContext);

	if (pEzreal)
		pEzreal->Initialize(nullptr);

	if (pRenekton)
		pRenekton->Initialize(nullptr);

	Add_Player(PLAYER, pEzreal);

	m_pActivePlayer = pEzreal;

	return S_OK;
}

HRESULT CPlayerManager::Add_Player(TYPE eType, CChampion* pPlayer)
{
	if (!pPlayer)
		return E_FAIL;

	m_AllPlayers.push_back(pPlayer);
	return S_OK;
}

void CPlayerManager::Update(_float fTimeDelta)
{
	for (auto& player : m_AllPlayers)
		player->Update(fTimeDelta);

	// ?? F4 키로 모델 교체 (챔피언 변경)
	if (GetAsyncKeyState(VK_F4) & 0x0001)
	{
		ChangeChampion();
	}
}

void CPlayerManager::Late_Update(_float fTimeDelta)
{
	for (auto& player : m_AllPlayers)
		player->Late_Update(fTimeDelta);
}

void CPlayerManager::Render()
{
	for (auto& player : m_AllPlayers)
		player->Render();
}

void CPlayerManager::ChangeActivePlayer(TYPE eType)
{
	if (eType < m_AllPlayers.size())
		m_pActivePlayer = m_AllPlayers[eType];
}

void CPlayerManager::ChangeChampion()
{
	if (!m_pActivePlayer)
		return;

	// 현재 챔피언 타입 확인
	CChampion::PLAYERID eID = m_pActivePlayer->Get_PlayerID();

	// 1?? 기존 포인터 벡터에서 제거
	for (auto it = m_AllPlayers.begin(); it != m_AllPlayers.end(); ++it)
	{
		if (*it == m_pActivePlayer)
		{
			Safe_Release(*it);
			m_AllPlayers.erase(it);
			break;
		}
	}

	// 2?? 새 챔피언 생성
	if (eID == CChampion::EZREAL)
	{
		m_pActivePlayer = CRenekton::Create(m_pDevice, m_pContext);
		m_pActivePlayer->Initialize(nullptr);
		m_pActivePlayer->Set_PlayerID(CChampion::RENEKTON);
	}
	else
	{
		m_pActivePlayer = CEzreal::Create(m_pDevice, m_pContext);
		m_pActivePlayer->Initialize(nullptr);
		m_pActivePlayer->Set_PlayerID(CChampion::EZREAL);
	}

	m_AllPlayers.push_back(m_pActivePlayer);
}



void CPlayerManager::Free()
{
	for (auto& player : m_AllPlayers)
		Safe_Release(player);

	m_AllPlayers.clear();

	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);
}