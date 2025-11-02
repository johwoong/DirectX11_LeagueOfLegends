#include "PlayerManager.h"
#include "Champion.h"
#include "Ezreal.h"
#include "Renekton.h"
#include "GameInstance.h"

IMPLEMENT_SINGLETON(CPlayerManager)

CPlayerManager::CPlayerManager()
{
}

void CPlayerManager::Set_ActivePlayer(CChampion* pPlayer)
{
	if (!pPlayer) return;

	m_pActivePlayer = pPlayer;
}

HRESULT CPlayerManager::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    m_pDevice = pDevice;
    m_pContext = pContext;
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);

    CGameInstance* pGI = CGameInstance::GetInstance();
    if (!pGI)
        return E_FAIL;

    // Ezreal 생성
    auto pEzreal = CEzreal::Create(m_pDevice, m_pContext);
    pEzreal->Initialize(nullptr);
    pEzreal->Set_PlayerID(CChampion::EZREAL);
    m_vChampions.push_back(pEzreal);

    // Renekton 생성
    auto pRenekton = CRenekton::Create(m_pDevice, m_pContext);
    pRenekton->Initialize(nullptr);
    pRenekton->Set_PlayerID(CChampion::RENEKTON);
    m_vChampions.push_back(pRenekton);

    // 첫 챔피언을 활성화
    m_iCurrentChampionIndex = 0;
    m_pActivePlayer = pEzreal;

    // ? 반드시 Layer에 직접 등록해야 Update/Render가 동작함
    pGI->ReAdd_GameObject(LEVEL_GAMEPLAY, TEXT("Layer_Player"), pEzreal);

    return S_OK;
}



void CPlayerManager::ChangeChampion()
{
    if (m_vChampions.size() < 2 || !m_pActivePlayer)
        return;

    CGameInstance* pGI = CGameInstance::GetInstance();
    if (!pGI)
        return;

    // ? 현재 활성 챔피언 인덱스 찾기
    _int iCurrent = -1;
    for (_uint i = 0; i < m_vChampions.size(); ++i)
    {
        if (m_vChampions[i] == m_pActivePlayer)
        {
            iCurrent = i;
            break;
        }
    }
    if (iCurrent == -1) return;

    // ? 다음 챔피언 인덱스 (2개만 존재하므로 토글)
    _int iNext = (iCurrent + 1) % m_vChampions.size();

    // ? 현재 위치 저장
    _vector vPos = m_pActivePlayer->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

    // ? 이전 챔피언 완전히 비활성화
    // (Layer에서 제거하고, 렌더링/업데이트에서 제외)
    pGI->Out_GameObject(LEVEL_GAMEPLAY, TEXT("Layer_Player"), m_pActivePlayer);

    // ? 새 챔피언 교체
    m_pActivePlayer = m_vChampions[iNext];
    m_iCurrentChampionIndex = iNext;

    // ? 위치 복원
    m_pActivePlayer->Get_Transform()->Set_State(CTransform::STATE_TRANSLATION, vPos);
    m_pActivePlayer->Set_Dead(false);

    // ? Layer에 등록 (새로 활성화된 챔피언만)
    pGI->ReAdd_GameObject(LEVEL_GAMEPLAY, TEXT("Layer_Player"), m_pActivePlayer);

}



void CPlayerManager::Free()
{
    for (auto& pChampion : m_vChampions)
        Safe_Release(pChampion);

    m_vChampions.clear();
    Safe_Release(m_pDevice);
    Safe_Release(m_pContext);
}