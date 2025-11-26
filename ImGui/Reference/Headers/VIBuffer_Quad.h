#pragma once
#include "VIBuffer.h"

NS_BEGIN(Engine)
class ENGINE_DLL CVIBuffer_Quad final : public CVIBuffer
{
protected:
    CVIBuffer_Quad(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CVIBuffer_Quad(const CVIBuffer_Quad& rhs);
    virtual ~CVIBuffer_Quad() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;

public:
    static CVIBuffer_Quad* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CComponent* Clone(void* pArg = nullptr) override;
    virtual void Free() override;
};
NS_END

