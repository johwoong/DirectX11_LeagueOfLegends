#pragma once
#include "Building.h"

// ¾ïÁ¦±â 
NS_BEGIN(Client)
class CTurret final : public CBuilding
{
public:
	enum TEAMTYPE{RED, BLUE};
	enum STATE { DEATH, DESTORY, IDLE };

private:
	CTurret(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CTurret(const CTurret& rhs);
	virtual ~CTurret() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual int		Update(_float fTimeDelta);
	virtual void	Late_Update(_float fTimeDelta);

public: /*For.State*/
	virtual _bool	Is_AnimationLoop(_uint eAnimId);
	void			Update_State(_float fTimeDelta);
	void			LateUpdate_State(_float fTimeDelta);

public:
	virtual HPBARDESC Get_HpBarDesc() const override;
private:
	virtual HRESULT Ready_Components(void* pArg);

private:
	_uint m_eState = IDLE;
public:
	static CTurret* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free() override;
};
NS_END

