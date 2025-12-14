#include "EffectManager.h"
#include "GameInstance.h"
#include "Effect.h"
#include "ParticleSystem.h"

IMPLEMENT_SINGLETON(CEffectManager)

CEffectManager::CEffectManager()
{
}

void CEffectManager::Add_Effect(CEffect* pEffect)
{
	m_InstancedEffects.push_back(pEffect);
}

void CEffectManager::Remove_Effect(CEffect* pEffect)
{
	for (auto iter = m_InstancedEffects.begin(); iter != m_InstancedEffects.end(); iter++)
	{
		if (!wcscmp((*iter)->Get_EffectName(), pEffect->Get_EffectName()))
		{
			pEffect->Set_Dead(true);
			m_InstancedEffects.erase(iter);

			return;
		}
	}
}

void CEffectManager::Clear_Effects()
{
	for (auto iter = m_InstancedEffects.begin(); iter != m_InstancedEffects.end(); iter++)
	{
		(*iter)->Set_Dead(true);
	}

	m_InstancedEffects.clear();
}

void CEffectManager::Free()
{
}
