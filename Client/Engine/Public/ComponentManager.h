#pragma once
#include "Base.h"

#include "Renderer.h"
#include "VIBuffer_Rect.h"
#include "Shader.h"
#include "Transform.h"
#include "Navigation.h"
#include "Texture.h"
#include "Model.h"


// 컴포넌트 원형 생성 보관

NS_BEGIN(Engine)
class CComponentManager :
    public CBase
{
    DECLARE_SINGLETON(CComponentManager)
private:
    CComponentManager();
    virtual ~CComponentManager() = default;

public:
    HRESULT Reserve_Container(_uint iNumLevels);
    HRESULT Add_Prototype(_uint iLevelIdx, const _tchar* pPrototypeTag, class CComponent* pPrototype);
    class CComponent* Clone_Component(_uint iLevelIdx, const _tchar* pPrototypeTag, void* pArg = nullptr);
    HRESULT Find_Prototype(_uint iLevelIdx, const _tchar* pPrototypeTag);
    void  Clear(_uint iLevelIdx);

private:
    _uint               m_iNumLevels = 0;
    map<const _tchar*, class CComponent*>* m_pPrototypes = { nullptr };
    typedef map<const _tchar*, class CComponent*> PROTOTYPES;

private:
    class CComponent* Find_Component(_uint iLevelIdx, const _tchar* pPrototypeTag);

public:
    virtual void Free() override;

};

NS_END

