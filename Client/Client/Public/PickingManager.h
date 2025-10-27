#pragma once
#include "Base.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CGameObject;
NS_END

NS_BEGIN(Client)
class CPickingManager : public CBase
{
	DECLARE_SINGLETON(CPickingManager)
private:
	explicit CPickingManager();
	virtual ~CPickingManager() = default;

public:
	void	Clear_PickingMgr();

public:
	void	Add_PickingGroup(CGameObject* pGameObject);
	void	Out_PickingGroup(CGameObject* pGameObject);
	void	Set_PickedObj(CGameObject* pGameObject);
	void	Mouse_Intersect_UI(_bool Data) { m_bMouseInUI = Data; }


public: /*Get*/
	_float3 Get_PickingPos() { return m_vPickingPos; }
	class CGameObject* Get_PickedObj() { return m_pPickedObj; }

public:
	_bool	Picking();

private:
	class CGameObject* m_pPickedObj = nullptr;
	list<CGameObject*>	m_GameObjects;
	_bool				m_bMouseInUI = false;
	_float3				m_vPickingPos = _float3(0.f, 0.f, 0.f);

public:
	virtual void Free() override;
};
NS_END

