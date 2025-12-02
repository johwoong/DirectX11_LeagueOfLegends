#include "PickingManager.h"
#include "GameInstance.h"
#include "Transform.h"
#include "GameObject.h"
#include "Actor.h"
#include "CursorManager.h"

IMPLEMENT_SINGLETON(CPickingManager)


CPickingManager::CPickingManager()
{
}


void CPickingManager::Clear_PickingMgr()
{
	m_GameObjects.clear();
}


void CPickingManager::Add_PickingGroup(CGameObject* pGameObject)
{
	if (nullptr == pGameObject)
		return;
	m_GameObjects.push_back(pGameObject);
}

void CPickingManager::Out_PickingGroup(CGameObject* pGameObject)
{
	auto iter = m_GameObjects.begin();
	while (iter != m_GameObjects.end())
	{
		if (*iter == pGameObject)
			iter = m_GameObjects.erase(iter);
		else
			++iter;
	}
}

void CPickingManager::Set_PickedObj(CGameObject* pGameObject)
{
	if (pGameObject == nullptr)
	{
		if (m_pPickedObj != nullptr)
			dynamic_cast<CActor*>(m_pPickedObj)->Set_Picked(false);
		m_pPickedObj = nullptr;
		return;
	}

	m_pPickedObj = pGameObject;

	for (auto& pGameObject : m_GameObjects)
		dynamic_cast<CActor*>(pGameObject)->Set_Picked(false);

	dynamic_cast<CActor*>(m_pPickedObj)->Set_Picked(true);
}

_bool CPickingManager::Picking()
{
    if (m_bMouseInUI)
        return false;

    CCursorManager::GetInstance()->SetState(CURSOR_DEFAULT);

    struct HitInfo
    {
        CGameObject* pObj;
        _float3      vPos;
    };

    vector<HitInfo> hits;

    _float3 vPosition;

    for (auto& pGameObject : m_GameObjects)
    {
        if (pGameObject->Picking(&vPosition))
        {
            HitInfo info;
            info.pObj = pGameObject;
            info.vPos = vPosition;
            hits.push_back(info);
        }
    }

    if (hits.empty())
    {
        Set_PickedObj(nullptr);
        return false;
    }

    // 카메라에 가장 가까운 오브젝트 기준으로 정렬
    std::sort(hits.begin(), hits.end(),
        [](const HitInfo& a, const HitInfo& b)
        {
            return a.pObj->Get_CamDistance() < b.pObj->Get_CamDistance();
        });

    CGameObject* pFront = hits.front().pObj;

    // ★ 여기서 PickedObj / PickingPos를 "같은 애" 기준으로 세팅
    Set_PickedObj(pFront);
    m_vPickingPos = hits.front().vPos;

    // 선택된 오브젝트에 피킹 true
    pFront->PickingTrue();

    return true;
}



void CPickingManager::Free()
{
	m_GameObjects.clear();
}