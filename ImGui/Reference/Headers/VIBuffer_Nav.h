#pragma once
#include "VIBuffer.h"

NS_BEGIN(Engine)
class ENGINE_DLL CVIBuffer_Nav final :public CVIBuffer
{
protected:
	CVIBuffer_Nav(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CVIBuffer_Nav(const CVIBuffer_Nav& rhs);
	virtual ~CVIBuffer_Nav() = default;

public:
	virtual HRESULT Initialize_Prototype(const _float3* pPoints);
	virtual HRESULT Initialize(void* pArg);
	virtual HRESULT Render() override;

public:
	static CVIBuffer_Nav* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _float3* pPoints);
	virtual CComponent* Clone(void* pArg = nullptr);
	virtual void Free() override;
};

NS_END

