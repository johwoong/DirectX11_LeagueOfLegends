#pragma once
#include "Building.h"

// 넥서스
// 미니언 생성 담당
NS_BEGIN(Client)
class CNexus final : public CBuilding
{
public:
	enum TEAMTYPE { RED, BLUE };

	enum STATE { IDLE, DESTORY };

private:
	CNexus(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CNexus(const CNexus& rhs);
	virtual ~CNexus() = default;

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
	static CNexus* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free() override;
};
NS_END

