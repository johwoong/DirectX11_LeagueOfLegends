#pragma once
#include "Base.h"

NS_BEGIN(Client)
class CEffectManager final :  public CBase
{
	DECLARE_SINGLETON(CEffectManager)

private:
	CEffectManager();
	virtual ~CEffectManager() = default;


	

private:
	vector<class CEffect*> m_InstancedEffects;

public:
	virtual void Free() override;
};
NS_END

