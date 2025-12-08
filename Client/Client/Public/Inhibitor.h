#pragma once
#include "Building.h"


// Æ÷Å¾
NS_BEGIN(Client)
class CInhibitor final : public CBuilding
{
public:
	enum TEAMTYPE { RED, BLUE };

	enum STATE { IDLE, ATTACK, DESTORY };

private:
	CInhibitor(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CInhibitor(const CInhibitor& rhs);
	virtual ~CInhibitor() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual int		Update(_float fTimeDelta);
	virtual void	Late_Update(_float fTimeDelta);

public:
	virtual HPBARDESC Get_HpBarDesc() const override;
private:
	virtual HRESULT Ready_Components(void* pArg);

private:
	_uint m_eState = IDLE;

public:
	static CInhibitor* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free() override;
};
NS_END

