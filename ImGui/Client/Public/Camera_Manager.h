#pragma once
#include "Base.h"
#include "Client_Defines.h"
#include "Camera_Dynamic.h"

NS_BEGIN(Client)
class CCamera_Manager final: public CBase
{
    DECLARE_SINGLETON(CCamera_Manager)

public:
    enum CAM_MODE { DYNAMIC, ACTION };

private:
    CCamera_Manager();
    virtual ~CCamera_Manager() = default;

public:
    void        Set_CamMode(CAM_MODE eMode);
    CAM_MODE    Get_CamMode() const { return m_eCamMode; }

    CCamera* Get_CurrentCamera() const { return m_pCurrentCamera; }

    HRESULT     Add_Camera(CAM_MODE eMode);
    void        Clear_Cameras();

private:
    CAM_MODE    m_eCamMode = DYNAMIC;
    CCamera* m_pCurrentCamera = nullptr;

public:
    virtual void Free() override;
};
NS_END

