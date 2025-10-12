#pragma once
#include "Base.h"

NS_BEGIN(Engine)
// 생성된 게임오브젝트들을 모아서 보관
class CObject_Manager final : public CBase
{
	DECLARE_SINGLETON(CObject_Manager)
private:
	CObject_Manager();
	virtual ~CObject_Manager() = default;

public:
 	class CComponent* Get_Component(_uint iLevelIdx, const _tchar* pLayerTag, const _tchar* pComponentTag, _uint iIndex = 0);

public:
	HRESULT Reserve_Container(_uint iNumLevels);
	HRESULT Add_Prototype(const _tchar* pPrototypeTag, class CGameObject* pPrototype);
	HRESULT Add_GameObject(const _tchar* pPrototypeTag, _uint iLevelIdx, const _tchar* pLayerTag, void* pArg = nullptr);
	HRESULT Add_GameObject_Out(const _tchar* pPrototypeTag, _uint iLevelIdx, const _tchar* pLayerTag, OUT CGameObject*& pGameObjectOut, void* pArg = nullptr);
	HRESULT Add_GameObjectLoad(const _tchar* pPrototypeTag, _uint iLevelIdx, const _tchar* pLayerTag, const _tchar* pVIBufferTag, void* pArg = nullptr);
	class CGameObject* Clone_GameObject(const _tchar* pPrototypeTag, void* pArg = nullptr);

public:
	void Update(_float fTimeDelta);
	void Late_Update(_float fTimeDelta);

public:
	class CGameObject* Get_Objects(_uint iLevelIdx, const _tchar* pLayerTag, _uint iIdx = 0);
	list<CGameObject*>* Get_ObjectList(_uint iSceneID, const _tchar* pLayerTag);
	HRESULT Out_GameObject(_uint iLevelIdx, const _tchar* pLayerTag, class CGameObject* pGameObject);
	HRESULT Out_GameObjectList(_uint iLevelIdx, const _tchar* pLayerTag);
	HRESULT ReAdd_GameObject(_uint iLevelIdx, const _tchar* pLayerTag, CGameObject* pGameObject);

public:
	void Clear(_uint iLevelIdx);
	void Clear_Layer(_uint iLevelIdx, const _tchar* pLayerTag);

private:
	map<const _tchar*, class CGameObject*>		m_Prototypes;
	typedef map<const _tchar*, class CGameObject*> PROTOTYPES;
	
private:
	_uint		m_iNumLevels = 0;
	map<const _tchar*, class CLayer*>*	m_pLayers = { nullptr };
	typedef map<const _tchar*, class CLayer*>	LAYERS;

private:
	class CGameObject* Find_Prototype(const _tchar* pPrototypeTag);
	class CLayer* Find_Layer(_uint iLevelIdx, const _tchar* pLayerTag);

public:
	virtual void Free() override;
};

NS_END

