#pragma once

#include "VayneState.h"

NS_BEGIN(Client)
NS_BEGIN(Vayne)
class CReturnState final : public CVayneState
{
public:
    CReturnState(CVayne* pVayne);

    virtual CVayneState* AI_Behaviour(_float fTimeDelta) override;
    virtual CVayneState* Update(_float fTimeDelta) override;
    virtual CVayneState* LateUpdate(_float fTimeDelta) override;

    virtual void Enter() override;
    virtual void Exit() override;

private:
    _float m_fReturnTimeAcc = 0.f;   // ´©Àû ½Ã°£
    _float m_fReturnTime = 0.f;   // ÀÌ¹ø¿¡ µµ¸ÁÄ¥ ÃÑ ½Ã°£ (0~5ÃÊ ·£´ý)
};
NS_END
NS_END

