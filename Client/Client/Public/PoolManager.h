#pragma once
#include "Base.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CModel;
class CGameObject;
NS_END

NS_BEGIN(Client)
class CPoolManager :
    public CBase
{
    DECLARE_SINGLETON(CPoolManager)

public:
    CPoolManager();
    virtual ~CPoolManager() = default;

public:
    void Add_Pooling_Object(LEVEL iLevelIdx, const _tchar* pLayerTag, CGameObject* pGameObject);
    void Add_Pooling_Layer(LEVEL iLevelIdx, const _tchar* pLayerTag);

    _bool Reuse_Pooling_Layer(LEVEL iLevelIdx, const _tchar* pLayerTag, void* pArg = nullptr);
    _bool Reuse_Pooling_Object(LEVEL iLevelIdx, const _tchar* pLayerTag, void* pArg = nullptr);
    _bool Clear_AllPooling_Layer();
    CGameObject* Pop_Pooling_Object(LEVEL iLevelIdx, const _tchar* pLayerTag);

private:
    list<class CGameObject*>* m_pLayers;
    typedef list<class CGameObject*> LAYERS;

    map<const _tchar*, LAYERS*>* m_pObjects = nullptr;
    typedef map<const _tchar*, LAYERS*>		POOLINGOJECTS;

private:
    list<class CGameObject*>* Find_PoolingObjects(_uint iLevelIdx, const _tchar* pLayerTag);
    _bool Find_LayerTag(_uint iLevelIdx, const _tchar* pLayerTag);


public:
    virtual void Free() override;
};
NS_END

