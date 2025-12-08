#pragma once
#include "Building.h"
class CNpc :
    public CActor
{
public:
	enum TEAMTYPE { RED, BLUE };

	enum STATE { IDLE, SPAWN };

private:
	CNpc(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CNpc(const CNpc& rhs);
	virtual ~CNpc() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual int		Update(_float fTimeDelta);
	virtual void	Late_Update(_float fTimeDelta);

public: /*For.State*/
	virtual _bool	Is_AnimationLoop(_uint eAnimId);
	void			Update_State(_float fTimeDelta);
	void			LateUpdate_State(_float fTimeDelta);

private:
	virtual HRESULT Ready_Components(void* pArg);

private:
	_uint m_eState = IDLE;

public:
	static CNpc* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free() override;
};

