#pragma once
#include "Actor.h"
#include "GameInstance.h"
#include "Effect.h"
#include "ParticleSystem.h"
#include "Collision_Manager.h"
#include "Enemy.h"
#include "Player.h"
#include "Minion.h"
#include "Building.h"

NS_BEGIN(Engine)
class CModel;
NS_END

NS_BEGIN(Client)
class CProjectile abstract : public CActor
{
public:
	typedef struct ProjectileTag
	{
		OBJECT_ID  eCollisionGroup = PLAYER;
		_uint      eProjType = 0;

		_vector    vInitPositon = XMVectorSet(0.f, 0.f, 0.f, 1.f);
		_float     fStartTime = 0.f;
		_float     fDeadTime = 10.f;
		_float     fVelocity = 5.f;
		_int       iDamage = 20;

		_vector    vTargetDir = XMVectorSet(0.f, 0.f, 0.f, 0.f);
		_vector    vTargetPosition = XMVectorSet(0.f, 0.f, 0.f, 0.f);

		CActor* pTarget = nullptr;
		CActor* pOwner = nullptr;
		_float  offsetX = 0.f;
		_float  offsetZ = 0.f;

		LEVEL        ePoolLevel = LEVEL_GAMEPLAY;
		const _tchar* pPoolLayerTag = nullptr;

		// ? 추가: 풀 프리워밍(미리 만들어 풀에 넣는 용도)일 때 true
		_bool bPrewarm = false;

	}PROJECTILEDESC;

protected:
	CProjectile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CProjectile(const CProjectile& rhs);
	virtual ~CProjectile() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;
	virtual int Update(_float fTimeDelta) override;
	virtual void Late_Update(_float fTimeDelta) override;
	virtual HRESULT Render() override;

protected:
	virtual HRESULT Ready_Components(void* pArg = nullptr);
	virtual HRESULT SetUp_ShaderResources();
	virtual HRESULT SetUp_ShaderID();
	virtual void Collision_Check();

protected:
	void ReUse_Setting(void* pArg) override;
protected:
	PROJECTILEDESC m_ProjectileDesc;

	CModel* m_pModelCom = nullptr;
	vector<CEffect*> m_pEffects;
	vector<CEffect*> m_pDeadEffects;
protected:
	_bool m_bCollisionRegistered = false;
protected:
	bool m_bInPool = false;
	void On_ReturnToPool();

	_float	m_fTime = 0;
	_bool	m_bDeadEffect = false;
protected:
	LEVEL        m_ePoolLevel = LEVEL_GAMEPLAY;
	const _tchar* m_pPoolLayerTag = nullptr;
protected:
	std::wstring m_PoolLayerTagStr;   // ★ 태그 수명 보장용
public:
	virtual void Free() override;
};
NS_END