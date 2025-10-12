#pragma once
#include "GameObject.h"

NS_BEGIN(Engine)

class CLayer final : public CBase
{
private:
    CLayer();
    virtual ~CLayer() = default;

public:
    class CComponent* Get_Component(const _tchar* pComponentTag, _uint iIndex = 0);
    class CGameObject* Get_Object(_uint iIndex = 0);
    list<class CGameObject*>* Get_ObjectList() { return &m_GameObjects; }

public:
    HRESULT Initialize(_uint iLevelIdx);
    HRESULT Add_GameObject(class CGameObject* pGameObject);
    HRESULT Out_GameObject(class CGameObject* pGameObject);
    HRESULT Out_GameObjectList();
    void Update(_float fTimeDelta);
    void Late_Update(_float fTimeDelta);

private:
    list<class CGameObject*>        m_GameObjects;
    typedef list<class CGameObject*> GAMEOBJECTS;
    _uint m_iLevel = 0;

public:
    static CLayer* Create(_uint iLevelIdx);
    virtual void Free() override;

};

NS_END

