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

private:
    /* OBJ_DEAD 가 된 오브젝트는 같은 프레임에 다른 오브젝트가 raw pointer 로 들고 있을 수 있다.
       그래서 바로 Release 하지 않고 유예 시간 뒤에 Release 한다. */
    struct DEADOBJECT
    {
        class CGameObject* pObject = nullptr;
        _float fElapsed = 0.f;
    };
    list<DEADOBJECT>                m_Graveyard;
    static constexpr _float         DEAD_RELEASE_DELAY = 5.f;

public:
    static CLayer* Create(_uint iLevelIdx);
    virtual void Free() override;

};

NS_END

