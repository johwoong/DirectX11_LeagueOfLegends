#pragma once
#include "Champion.h"
class CRenekton final : public CChampion
{
private:
	CRenekton(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CRenekton(const CRenekton& rhs);
	virtual ~CRenekton() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);


private:
	virtual HRESULT Ready_Components(void* pArg) override;
public:
	static CRenekton* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free() override;
};

