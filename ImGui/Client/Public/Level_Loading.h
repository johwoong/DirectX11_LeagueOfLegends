#pragma once
#include "Client_Defines.h"
#include "Level.h"

NS_BEGIN(Client)
class CLevel_Loading final :
    public CLevel
{

private:
    CLevel_Loading(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CLevel_Loading() = default;

public:
    virtual HRESULT Initialize(LEVEL eNextLevel);
    virtual void Update(_float fTimeDelta) override;
    virtual void Late_Update(_float fTimeDelta) override;

private:
    LEVEL               m_eNextLevel        = LEVEL_END;
    class   CLoader*    m_pLoader      = { nullptr };

public:
    static CLevel_Loading* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eNextLevel);
    virtual void Free() override;
};

NS_END


