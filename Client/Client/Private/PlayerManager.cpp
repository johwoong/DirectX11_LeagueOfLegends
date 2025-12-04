#include "PlayerManager.h"
#include "Champion.h"
#include "Ezreal.h"
#include "Vi.h"
#include "GameInstance.h"
#include "UI_SkillMenu.h"
#include "UI_ExpBar.h"

IMPLEMENT_SINGLETON(CPlayerManager)

CPlayerManager::CPlayerManager()
{
}

void CPlayerManager::Set_ActivePlayer(CChampion* pPlayer)
{
	if (!pPlayer) return;

	m_pActivePlayer = pPlayer;
}

void CPlayerManager::Set_SkillMenu(CUI_SkillMenu* pMenu)
{
    m_pSkillMenu = pMenu;
    if (m_pActivePlayer && m_pSkillMenu)
        m_pSkillMenu->Set_SkillIcons(m_pActivePlayer->Get_Skills());
}

void CPlayerManager::Set_ExpBar(CUI_ExpBar* pBar)
{
    m_pExpBar = pBar;

    if (m_pActivePlayer && m_pExpBar)
        m_pExpBar->Set_ChampionIcon(m_pActivePlayer->Get_ChampionIconTag());

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

    auto pEzreal = CEzreal::Create(m_pDevice, m_pContext);
    pEzreal->Initialize(nullptr);
    pEzreal->Set_PlayerID(CChampion::EZREAL);
    pEzreal->Compute_CurrentIndex();
    pEzreal->Check_Navigation();
    m_vChampions.push_back(pEzreal);

    auto pVi = CVi::Create(m_pDevice, m_pContext);
    pVi->Initialize(nullptr);
    pVi->Set_PlayerID(CChampion::VI);
    m_vChampions.push_back(pVi);

    m_iCurrentChampionIndex = 0;
    m_pActivePlayer = pEzreal;



    pGI->ReAdd_GameObject(LEVEL_GAMEPLAY, TEXT("Layer_Player"), pEzreal);
    m_pSkillMenu = dynamic_cast<CUI_SkillMenu*>(
        pGI->Get_Object(LEVEL_GAMEPLAY, TEXT("Layer_UI"), 0)
        );

    if (m_pSkillMenu)
        m_pSkillMenu->Set_SkillIcons(m_pActivePlayer->Get_Skills());

    if (m_pExpBar)
        m_pExpBar->Set_ChampionIcon(m_pActivePlayer->Get_ChampionIconTag());

    return S_OK;
}



void CPlayerManager::ChangeChampion()
{
    if (m_vChampions.size() < 2 || !m_pActivePlayer)
        return;

    CGameInstance* pGI = CGameInstance::GetInstance();
    if (!pGI)
        return;

    _int iCurrent = -1;
    for (_uint i = 0; i < m_vChampions.size(); ++i)
    {
        if (m_vChampions[i] == m_pActivePlayer)
        {
            iCurrent = i;
            break;
        }
    }

    if (iCurrent == -1)
        return;

    _int iNext = (iCurrent + 1) % m_vChampions.size();
    _vector vPos = m_pActivePlayer->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

    // 기존 캐릭터 레벨에서 제거
    pGI->Out_GameObject(LEVEL_GAMEPLAY, TEXT("Layer_Player"), m_pActivePlayer);

    // 새 활성 캐릭터로 변경
    m_pActivePlayer = m_vChampions[iNext];
    m_iCurrentChampionIndex = iNext;

    // 위치 유지
    m_pActivePlayer->Get_Transform()->Set_State(CTransform::STATE_TRANSLATION, vPos);
    m_pActivePlayer->Set_Dead(false);
    m_pActivePlayer->Reset_State();

    // 레벨에 다시 등록
    pGI->ReAdd_GameObject(LEVEL_GAMEPLAY, TEXT("Layer_Player"), m_pActivePlayer);

    // UI 갱신
    if (m_pSkillMenu)
        m_pSkillMenu->Set_SkillIcons(m_pActivePlayer->Get_Skills());

    if (m_pExpBar)
        m_pExpBar->Set_ChampionIcon(m_pActivePlayer->Get_ChampionIconTag());



}



void CPlayerManager::Free()
{
    for (auto& pChampion : m_vChampions)
        Safe_Release(pChampion);

    m_vChampions.clear();
    Safe_Release(m_pActivePlayer);
    Safe_Release(m_pDevice);
    Safe_Release(m_pContext);
}