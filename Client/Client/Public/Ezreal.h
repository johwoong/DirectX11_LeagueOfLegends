#pragma once
#include "Champion.h"

NS_BEGIN(Client)
class CEzreal final : public CChampion
{
private:
	CEzreal(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CEzreal(const CEzreal& rhs);
	virtual ~CEzreal() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);


private:
	virtual HRESULT Ready_Components(void* pArg) override;

public:
	static CEzreal* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free() override;
};
NS_END

