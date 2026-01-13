#pragma once
#include "Base.h"
#include "CColider.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CGameObject;
NS_END

NS_BEGIN(Client)
class CActor;
class CCollision_Manager : public CBase
{
    DECLARE_SINGLETON(CCollision_Manager)
public:
    enum COLLISION_GROUP {COLLISION_PLAYER, COLLISION_ENEMY, COLLISION_MINION, COLLISION_PROJECT, COLLISION_BUILDING, COLLISION_NPC, COLLISION_NORMAL, COLLISION_END};
    enum COLLIDER_TYPE {COLLIDER_AABB, COLLIDER_OBB, COLLIDER_SPHERE, COLLIDER_END};

public:
    void Add_CollisionGroup(COLLISION_GROUP CollisionGroup, CActor* pGameObject);
    void Add_CollisionGroupCollider(COLLISION_GROUP CollisionGroup, CColider* pCollider, CActor* pGameObject);
    void	Out_CollisionGroup(COLLISION_GROUP CollisionGroup, CActor* pGameObject);
    void	Out_CollisionGroupCollider(COLLISION_GROUP CollisionGroup, CColider* pCollider, CActor* pGameObject);
    void	Clear_CollisionGroup(COLLISION_GROUP CollisionGroup);
    void	Clear_CollisionGroupExpect(COLLISION_GROUP CollisionGroup);
    void	Clear_AllCollisionGroup();

public:
    _bool	CollisionwithGroup(COLLISION_GROUP CollisionGroup, CColider* pCollider, CActor** pOut = nullptr, COLLIDER_TYPE eType = COLLIDER_END);
    _bool	CollisionwithGroupCollider(COLLISION_GROUP CollisionGroup, CColider* pCollider, CActor** pOut = nullptr, COLLIDER_TYPE eType = COLLIDER_END);
    _bool CollisionwithGroup_All(COLLISION_GROUP CollisionGroup, CColider* pCollider, std::vector<CActor*>* pOut,COLLIDER_TYPE eType = COLLIDER_END);
    _bool	CollisionwithGroup(COLLISION_GROUP SourGroup, COLLISION_GROUP DestGroup, CActor** outSour, CActor** outDest);
    void	Update_Collider();

public:
    const list<CActor*>& Get_CollisionGroup(COLLISION_GROUP group) const { return m_GameObjects[group]; }

public:
    CColider* Reuse_Collider(CColider::TYPE eType, _uint iLevelIdx, const _tchar* pPrototypeTag, void* pArg);
    void Collect_Collider(CColider::TYPE eType, CColider* pCollider);

private:
    explicit CCollision_Manager();
    virtual ~CCollision_Manager() = default;

private:
    list<CActor*>   m_GameObjects[COLLISION_END];
    
    list<CActor*>   m_ColliderGameObjects[COLLISION_END];
    list<CColider*> m_Colliders[COLLISION_END];

    list<CColider*> m_ColliderPool[COLLIDER_END];

public:
    virtual void Free() override;

};
NS_END

