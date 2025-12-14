#pragma once
#include "Base.h"

NS_BEGIN(Client)
class CEffectManager final :  public CBase
{
	DECLARE_SINGLETON(CEffectManager)

private:
	CEffectManager();
	virtual ~CEffectManager() = default;

public:
	vector<class CEffect*> Get_InstancedEffects() { return m_InstancedEffects; }

public:
	void Add_Effect(class CEffect* pEffect);
	void Remove_Effect(class CEffect* pEffect);
	void Clear_Effects();

private:
	vector<class CEffect*> m_InstancedEffects;

public:
	virtual void Free() override;
};
NS_END

