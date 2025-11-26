#include "VIBuffer_Quad.h"

CVIBuffer_Quad::CVIBuffer_Quad(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CVIBuffer(pDevice, pContext)
{
}

CVIBuffer_Quad::CVIBuffer_Quad(const CVIBuffer_Quad& rhs)
    : CVIBuffer(rhs)
{
}

HRESULT CVIBuffer_Quad::Initialize_Prototype()
{
#pragma region VERTICES
    ZeroMemory(&m_BufferDesc, sizeof(D3D11_BUFFER_DESC));

    m_iStride = sizeof(VTXTEX);
    m_iNumVertices = 4;
    m_iNumVertexBuffers = 1;
    m_eFormat = DXGI_FORMAT_R16_UINT;
    m_eTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    m_BufferDesc.ByteWidth = m_iStride * m_iNumVertices;
    m_BufferDesc.Usage = D3D11_USAGE_DEFAULT;
    m_BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    VTXTEX* pVertices = new VTXTEX[m_iNumVertices];

    // *** XZ 평면용 사각형 ***
    pVertices[0].vPosition = _float3(-0.5f, 0.f, -0.5f);
    pVertices[0].vTexture = _float2(0.f, 0.f);

    pVertices[1].vPosition = _float3(0.5f, 0.f, -0.5f);
    pVertices[1].vTexture = _float2(1.f, 0.f);

    pVertices[2].vPosition = _float3(0.5f, 0.f, 0.5f);
    pVertices[2].vTexture = _float2(1.f, 1.f);

    pVertices[3].vPosition = _float3(-0.5f, 0.f, 0.5f);
    pVertices[3].vTexture = _float2(0.f, 1.f);

    ZeroMemory(&m_SubResourceData, sizeof(D3D11_SUBRESOURCE_DATA));
    m_SubResourceData.pSysMem = pVertices;

    if (FAILED(__super::Create_VertexBuffer()))
        return E_FAIL;

    Safe_Delete_Array(pVertices);
#pragma endregion

#pragma region INDICES
    ZeroMemory(&m_BufferDesc, sizeof(D3D11_BUFFER_DESC));

    m_iIndicesByte = sizeof(FACEINDICES16);
    m_iNumPrimitive = 2;
    m_iNumIndicesPerPrimitive = 3;

    m_BufferDesc.ByteWidth = m_iIndicesByte * m_iNumPrimitive;
    m_BufferDesc.Usage = D3D11_USAGE_DEFAULT;
    m_BufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    FACEINDICES16* pIndices = new FACEINDICES16[m_iNumPrimitive];

    pIndices[0] = { 0, 1, 2 };
    pIndices[1] = { 0, 2, 3 };

    ZeroMemory(&m_SubResourceData, sizeof(D3D11_SUBRESOURCE_DATA));
    m_SubResourceData.pSysMem = pIndices;

    if (FAILED(__super::Create_IndexBuffer()))
        return E_FAIL;

    Safe_Delete_Array(pIndices);
#pragma endregion

    return S_OK;
}

HRESULT CVIBuffer_Quad::Initialize(void* pArg)
{
    return S_OK;
}

CVIBuffer_Quad* CVIBuffer_Quad::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CVIBuffer_Quad* pInstance = new CVIBuffer_Quad(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : CVIBuffer_Quad");
        Safe_Release(pInstance);
    }
    return pInstance;
}

CComponent* CVIBuffer_Quad::Clone(void* pArg)
{
    CVIBuffer_Quad* pInstance = new CVIBuffer_Quad(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Clone : CVIBuffer_Quad");
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CVIBuffer_Quad::Free()
{
    __super::Free();
}
