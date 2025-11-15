#pragma once
#include "Building.h"
class CStoreNpc :
    public CBuilding
{
public:
	enum TEAMTYPE { RED, BLUE };

	enum STATE { IDLE, DESTORY };

private:
	CStoreNpc(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CStoreNpc(const CStoreNpc& rhs);
	virtual ~CStoreNpc() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual int		Update(_float fTimeDelta);
	virtual void	Late_Update(_float fTimeDelta);


private:
	virtual HRESULT Ready_Components(void* pArg);

private:
	_uint m_eState = IDLE;

public:
	static CStoreNpc* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free() override;
};

