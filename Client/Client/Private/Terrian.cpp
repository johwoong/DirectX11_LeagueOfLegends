
#include "Terrian.h"

#include "GameInstance.h"
#include "PickingManager.h"

CTerrian::CTerrian(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CActor(pDevice, pContext)
{
}

CTerrian::CTerrian(const CTerrian& rhs)
    : CActor(rhs)
{
}

HRESULT CTerrian::Initialize_Prototype()
{
    if (FAILED(__super::Initialize_Prototype()))
        return E_FAIL;

    return S_OK;
}

HRESULT CTerrian::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (FAILED(Ready_Components(pArg)))
        return E_FAIL;
    CPickingManager::GetInstance()->Add_PickingGroup(this);

    return S_OK;
}

HRESULT CTerrian::Initialize_Load(const _tchar* VIBufferTag, void* pArg)
{
    /* For.Com_Renderer */
    if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRendererCom)))
        return E_FAIL;
    /* For.Com_Transform */
    if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC, TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom)))
        return E_FAIL;

    /* For.Com_Shader */
    if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxNorTex"), (CComponent**)&m_pShaderCom)))
        return E_FAIL;
    
    /* For.Com_VIBuffer */
    if (FAILED(__super::Add_Components(TEXT("Com_VIBuffer"), LEVEL_STATIC, VIBufferTag, (CComponent**)&m_pVIBufferCom)))
        return E_FAIL;
    
    CVIBuffer_Terrian::TERRIANDESC desc = m_pVIBufferCom->Get_TerrianDesc();
    _vector vInitPosition = XMVectorSet(desc.m_iPositionX, desc.m_fHeight, desc.m_iPositionZ, 1.f);
    m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vInitPosition);

    _float3 vRot = _float3(desc.m_fRotationX, desc.m_fRotationY, desc.m_fRotationZ);
    m_pTransformCom->Set_Rotation(vRot);

    CPickingManager::GetInstance()->Add_PickingGroup(this);

    return S_OK;
}

int CTerrian::Update(_float fTimeDelta)
{
    if (GetAsyncKeyState(VK_F2) & 0x0001)
        m_bWireFrame = !m_bWireFrame;

    if (GetAsyncKeyState(VK_F3) & 0x0001)
        m_bDebugShow = !m_bDebugShow;
    m_bDebugShow = false;
    return OBJ_NOEVENT;
}

void CTerrian::Late_Update(_float fTimeDelta)
{
    __super::Late_Update(fTimeDelta);

    if (nullptr != m_pRendererCom)
        m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_NOALPHABLEND, this);
}

HRESULT CTerrian::Render()
{
    if (!m_bDebugShow)
        return S_OK;

    if (nullptr == m_pShaderCom || nullptr == m_pVIBufferCom)
        return E_FAIL;

    if (FAILED(SetUp_ShaderResources()))
        return E_FAIL;

    // Pass 0 = Solid, Pass 1 = WireFrame
    _uint iPassIndex = m_bWireFrame ? 1 : 0;

    m_pShaderCom->Begin(iPassIndex);
    m_pVIBufferCom->Render();

    return S_OK;
}

_bool CTerrian::Picking(_float3* PickingPoint)
{
    if (m_pVIBufferCom->Picking(m_pTransformCom, PickingPoint) == true)
    {
        m_vMousePickedPos = *PickingPoint;
        return true;
    }
    else
    {
        ZeroMemory(&m_vMousePickedPos, sizeof(_float3));
    }
    return false;
}

void CTerrian::PickingTrue()
{

}

HRESULT CTerrian::Ready_Components(void* pArg)
{
    // Renderer
    if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC, TEXT("Prototype_Component_Renderer"),
        (CComponent**)&m_pRendererCom)))
        return E_FAIL;

    // Transform
    if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC, TEXT("Prototype_Component_Transform"),
        (CComponent**)&m_pTransformCom)))
        return E_FAIL;

    // Shader (TerrainColor)
    if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC, TEXT("Prototype_Component_Shader_TerrainColor"),
        (CComponent**)&m_pShaderCom)))
        return E_FAIL;

    // VIBuffer
    if (FAILED(__super::Add_Components(TEXT("Com_VIBuffer"), LEVEL_STATIC, TEXT("Prototype_Component_VIBuffer_Terrain"),
        (CComponent**)&m_pVIBufferCom)))
        return E_FAIL;

    return S_OK;
}

HRESULT CTerrian::SetUp_ShaderResources()
{
    if (nullptr == m_pShaderCom)
        return E_FAIL;

    _float4x4 WorldMatrix = m_pTransformCom->Get_World_TP();
    _float4x4 ViewMatrix = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_VIEW);
    _float4x4 ProjMatrix = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_PROJ);

    if (FAILED(m_pShaderCom->Set_RawValue("g_WorldMatrix", &WorldMatrix, sizeof(_float4x4))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Set_RawValue("g_ViewMatrix", &ViewMatrix, sizeof(_float4x4))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Set_RawValue("g_ProjMatrix", &ProjMatrix, sizeof(_float4x4))))
        return E_FAIL;

    return S_OK;
}

CTerrian* CTerrian::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CTerrian* pInstance = new CTerrian(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CTerrian");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CTerrian::Clone(void* pArg)
{
    CTerrian* pInstance = new CTerrian(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CTerrian");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CTerrian::Clone_Load(const _tchar* VIBufferTag, void* pArg)
{
    CTerrian* pInstance = new CTerrian(*this);

    if (FAILED(pInstance->Initialize_Load(VIBufferTag, pArg)))
    {
        MSG_BOX("Failed to Cloned : CTerrain");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CTerrian::Free()
{
    __super::Free();

    Safe_Release(m_pTransformCom);
    Safe_Release(m_pShaderCom);
    Safe_Release(m_pVIBufferCom);
    Safe_Release(m_pRendererCom);
}