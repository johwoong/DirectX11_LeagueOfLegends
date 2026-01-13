#include "PlayerManager.h"
#include "Champion.h"
#include "Ezreal.h"
#include "LeeSin.h"
#include "Temoo.h"
#include "GameInstance.h"
#include "UI_SkillMenu.h"
#include "UI_ExpBar.h"
#include "UI_Player_HpBar.h"
#include "CCamera_Dynamic.h"

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

    // 이즈리얼
    auto pEzreal = CEzreal::Create(m_pDevice, m_pContext);
    pEzreal->Initialize(nullptr);
    pEzreal->Set_PlayerID(CChampion::EZREAL);
    pEzreal->Compute_CurrentIndex();
    pEzreal->Check_Navigation();
    m_vChampions.push_back(pEzreal);


    // 리신
    auto pLeeSin = CLeeSin::Create(m_pDevice, m_pContext);
    pLeeSin->Initialize(nullptr);
    pLeeSin->Set_PlayerID(CChampion::LEESIN);   // ?? 여기 중요
    pLeeSin->Compute_CurrentIndex();
    pLeeSin->Check_Navigation();
    m_vChampions.push_back(pLeeSin);


    m_iCurrentChampionIndex = 0;
    m_pActivePlayer = pEzreal;

    // 티모
    auto pTeemo = CTemoo::Create(m_pDevice, m_pContext);
    pTeemo->Initialize(nullptr);
    pTeemo->Set_PlayerID(CChampion::TEEMO);
    pTeemo->Compute_CurrentIndex();
    pTeemo->Check_Navigation();
    m_vChampions.push_back(pTeemo);


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

    // ===============================
// AttackRange 1회 생성
// ===============================
    CGameObject* pObj = nullptr;

    if (FAILED(pGI->Add_GameObject_Out(
        TEXT("Prototype_GameObject_UI_AttackRange"),
        LEVEL_GAMEPLAY,
        TEXT("Layer_Effect"),
        pObj,
        m_pActivePlayer)))
    {
        return E_FAIL;
    }

    {
        if (m_pPlayerHpBar == nullptr)
        {
            CGameObject* pHpObj = nullptr;

            if (FAILED(pGI->Add_GameObject_Out(
                TEXT("Prototype_GameObject_PlayerHpBar"),
                LEVEL_GAMEPLAY,
                TEXT("Layer_UI_HpBar"),
                pHpObj,
                m_pActivePlayer))) // 초기 owner
            {
                return E_FAIL;
            }

            m_pPlayerHpBar = dynamic_cast<CPlayer_HpBar*>(pHpObj);

            if (m_pPlayerHpBar)
            {
                m_pPlayerHpBar->Set_Owner(m_pActivePlayer);
            }
        }
    }

    m_pAttackRange = dynamic_cast<CAttackRange*>(pObj);

    if (m_pAttackRange)
    {
        m_pAttackRange->Set_Visible(false);
        m_pAttackRange->Set_Alpha(1.f);
        m_pAttackRange->Set_Owner(m_pActivePlayer);
        m_pActivePlayer->Set_AttackRangeObj(m_pAttackRange);

        m_pAttackRange->Set_RadiusWorld(
            m_pActivePlayer->Get_WorldAttackRange());
    }

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
    if (m_pPlayerHpBar)
    {
        m_pPlayerHpBar->Set_Owner(m_pActivePlayer);
    }
    m_iCurrentChampionIndex = iNext;
    m_pActivePlayer->Set_PlayerID((CPlayer::PLAYERID)iNext);

    // ===============================
   // ★ AttackRange Owner 교체
   // ===============================
    if (m_pAttackRange)
    {
        m_pAttackRange->Set_Owner(m_pActivePlayer);
        m_pActivePlayer->Set_AttackRangeObj(m_pAttackRange);

        m_pAttackRange->Set_RadiusWorld(
            m_pActivePlayer->Get_WorldAttackRange());

        m_pAttackRange->Set_Visible(false);
    }

    // 위치 유지
    m_pActivePlayer->Get_Transform()->Set_State(CTransform::STATE_TRANSLATION, vPos);
    m_pActivePlayer->Set_Dead(false);
    m_pActivePlayer->Reset_State();

    CCamera_Dynamic* pCam =
        dynamic_cast<CCamera_Dynamic*>(pGI->Get_Object(
            LEVEL_GAMEPLAY, TEXT("Layer_Camera"), 0));

    if (pCam)
        pCam->Set_Target(m_pActivePlayer);

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
    // ★ dangling 방지
    if (m_pPlayerHpBar)
    {
        m_pPlayerHpBar->Set_Owner(nullptr);
        m_pPlayerHpBar = nullptr; // 레벨/레이어가 실제 삭제 담당
    }
    for (auto& pChampion : m_vChampions)
        Safe_Release(pChampion);

    m_vChampions.clear();
    Safe_Release(m_pDevice);
    Safe_Release(m_pContext);
}