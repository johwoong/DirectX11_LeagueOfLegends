#pragma once
#include "Projectile.h"

NS_BEGIN(Client)
class CEzrealProjectile final : public CProjectile
{
public:

	enum TYPE { BASE, Q, W, R};

public:
	CEzrealProjectile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CEzrealProjectile(const CEzrealProjectile& rhs);
	virtual ~CEzrealProjectile() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;
	virtual int Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta) override;
	virtual void Collision_Check() override;
	void Dead_Effect();

protected:
	virtual HRESULT Ready_Components(void* pArg = nullptr) override;

private:
	void Update_Projectile(_float fTimeDelta);

public:
	static CEzrealProjectile* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free() override;
};
NS_END
