#include "CColider.h"
#include "DebugDraw.h"
#include "GameInstance.h"

CColider::CColider(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CComponent(pDevice, pContext)
{
}

CColider::CColider(const CColider& rhs)
    : CComponent(rhs)
    , m_eType(rhs.m_eType)
#ifdef _DEBUG
    , m_pBatch(rhs.m_pBatch)
    , m_pEffect(rhs.m_pEffect)
    , m_pInputLayout(rhs.m_pInputLayout)
    , m_pDepthStencilState(rhs.m_pDepthStencilState)
#endif
{
#ifdef _DEBUG
    Safe_AddRef(m_pInputLayout);
    Safe_AddRef(m_pDepthStencilState);
#endif
}

HRESULT CColider::Initialize_Prototype(TYPE eType)
{
    m_eType = eType;

#ifdef _DEBUG
    m_pBatch = new PrimitiveBatch<VertexPositionColor>(m_pContext);
    m_pEffect = new BasicEffect(m_pDevice);

    m_pEffect->SetVertexColorEnabled(true);

    const void* pShaderbyteCode = nullptr;
    size_t      iShaderByteCodeLength = 0;

    m_pEffect->GetVertexShaderBytecode(&pShaderbyteCode, &iShaderByteCodeLength);

    if (FAILED(m_pDevice->CreateInputLayout(VertexPositionColor::InputElements, VertexPositionColor::InputElementCount, pShaderbyteCode, iShaderByteCodeLength, &m_pInputLayout)))
        return E_FAIL;

    D3D11_DEPTH_STENCIL_DESC			DepthStecilDesc;
    ZeroMemory(&DepthStecilDesc, sizeof(DepthStecilDesc));

    DepthStecilDesc.DepthEnable = TRUE;
    DepthStecilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    DepthStecilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    DepthStecilDesc.StencilEnable = FALSE;

    if (FAILED(m_pDevice->CreateDepthStencilState(&DepthStecilDesc, &m_pDepthStencilState)))
        return E_FAIL;
#endif
     
    return S_OK;
}

HRESULT CColider::Initialize(void* pArg)
{
    if (nullptr == pArg)
        return E_FAIL;

    memcpy(&m_ColiderDesc, pArg, sizeof(COLLIDERDESC));



}

void CColider::Update(_fmatrix WorldMatrix)
{
}

HRESULT CColider::Render()
{
    return S_OK;
}

HRESULT CColider::Set_Colider_Desc(void* pArg)
{
    return E_NOTIMPL;
}

_bool CColider::Collision(CColider* pTargetColider)
{
    return _bool();
}

_bool CColider::Collision_AABB(CColider* pTargetColider)
{
    return _bool();
}

_bool CColider::Collision_OBB(CColideR* pTargetColider)
{
    return _bool();
}

_matrix CColider::Remove_Rotation(_fmatrix Matrix)
{
    return _matrix();
}

_float3 CColider::Compute_Min()
{
    return _float3();
}

_float3 CColider::Compute_Max()
{
    return _float3();
}

CColider::OBBDESC CColider::Compute_OBBDesc()
{
    return OBBDESC();
}

CColider* CColider::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, TYPE eType)
{
    return nullptr;
}

CComponent* CColider::Clone(void* pArg)
{
    return nullptr;
}

void CColider::Free()
{
}
