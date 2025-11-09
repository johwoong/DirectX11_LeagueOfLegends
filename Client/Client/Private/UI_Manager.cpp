#include "UI_Manager.h"
#include "UI_Base.h"
#include "GameInstance.h"
#include "Champion.h"
#include "UI_SkillImage.h"
#include "UI_SkillMenu.h"

IMPLEMENT_SINGLETON(CUI_Manager)

CUI_Manager::CUI_Manager()
{
}

void CUI_Manager::Update_UI(_float fTimeDelta)
{
	Update_PlayerState();
}

void CUI_Manager::Update_PlayerState()
{
    CChampion* pActiveChampion = CPlayerManager::GetInstance()->Get_ActivePlayer();
    if (!pActiveChampion) return;

    auto pSkillMenu = dynamic_cast<CUI_SkillMenu*>(m_pInterface);
    if (pSkillMenu)
        pSkillMenu->Set_SkillIcons(pActiveChampion->Get_Skills());
}

void CUI_Manager::Free()
{

}
