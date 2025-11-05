#pragma once
#include "Client_Defines.h"
#include "Base.h"

NS_BEGIN(Client)
class CUI_Base;

class CUI_Manager :
    public CBase
{
    DECLARE_SINGLETON(CUI_Manager)
public:
    enum UITYPE { UI_INTERFACE, UI_MAP, UI_END};

private:
    CUI_Manager();
    virtual ~CUI_Manager() = default;

public:
    void Update_UI(_float fTimeDelta);
    void Update_Interface(_float fTimeDelta);
    void Update_Map(_float fTimeDelta);

private:
    UITYPE      m_eUIType = UI_END;


    CUI_Base* m_pInterface = { nullptr };
public:
    void Free() override;
};
NS_END

