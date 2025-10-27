#include "Terrian.h"
#include "GameInstance.h"

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

    if (m_eDebugtype == DEBUG_SOLID)
    {
        CTerrianManager::GetInstance()->Add_DebugTerrain(this);
    }

    CPickingMgr::GetInstance()->Add_PickingGroup(this);

    m_eObjectID = OBJ_BACKGROUND;

    return S_OK;
}

HRESULT CTerrian::Initialize_Load(const _tchar* VIBufferTag, void* pArg)
{
    CTerrianManager::TERRIANDESC TerrianDesc;
    if (pArg != nullptr)
    {
        ZeroMemory(&TerrianDesc, sizeof(CTerrianManager::TERRIANDESC));
        memcpy(&TerrianDesc, pArg, sizeof(CTerrianManager::TERRIANDESC));
        m_eDebugtype = (TERRIAN_DEBUG_TYPE)TerrianDesc.m_eDebugTerrian;
    }


    /* For.Com_Renderer */
    if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRenderCom)))
        return E_FAIL;

    /* For.Com_Texture */
    if (FAILED(__super::Add_Components(TEXT("Com_Texture"), LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_Terrain"), (CComponent**)&m_pTextureCom[TYPE_DIFFUSE])))
        return E_FAIL;

    /* For.Com_VIBuffer */
    if (FAILED(__super::Add_Components(TEXT("Com_VIBuffer"), LEVEL_STATIC, VIBufferTag, (CComponent**)&m_pVIBufferCom)))
        return E_FAIL;

    /* For.Com_Shader */
    if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxNorTex"), (CComponent**)&m_pShaderCom)))
        return E_FAIL;

    /* For.Com_Transform */
    CTransform::TRANSFORMDESC		TransformDesc;
    TransformDesc.fRotationPerSec = 1.f;
    TransformDesc.fSpeedPerSec = 3.f;

    if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC, TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom, &TransformDesc)))
        return E_FAIL;

    _vector vInitPosition = XMVectorSet(TerrianDesc.TerrianDesc.m_iPositionX, TerrianDesc.TerrianDesc.m_fHeight, TerrianDesc.TerrianDesc.m_iPositionZ, 1.f);
    m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vInitPosition);

    CPickingMgr::GetInstance()->Add_PickingGroup(this);
    m_eObjectID = OBJ_BACKGROUND;

    return S_OK;
}

int CTerrian::Update(_float fTimeDelta)
{
    if (m_bDead)
        return OBJ_DEAD;

    if (m_eDebugtype == DEBUG_SOLID)
    {
        CTerrianManager::TERRIANDESC TerrainDesc = CTerrianManager::GetInstance()->Get_TerrianDesc();
        _vector vPosition = XMVectorSet(TerrainDesc.TerrianDesc.m_iPositionX, TerrainDesc.TerrianDesc.m_fHeight, TerrainDesc.TerrianDesc.m_iPositionZ, 1.f);
        m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vPosition);
        m_bDebugShow = CTerrianManager::GetInstance()->Get_TerrianShow();
    }

    return OBJ_NOEVENT;
}

void CTerrian::Late_Update(_float fTimeDelta)
{
    __super::Late_Update(fTimeDelta);

    if (nullptr != m_pRenderCom)
        m_pRenderCom->Add_RenderGroup(CRenderer::RENDER_NOALPHABLEND, this);
}

HRESULT CTerrian::Render()
{
    if (m_eDebugtype == DEBUG_SOLID && !m_bDebugShow)
        return S_OK;

    if (nullptr == m_pShaderCom ||
        nullptr == m_pVIBufferCom)
        return E_FAIL;

    if (FAILED(SetUp_ShaderID()))
        return E_FAIL;

    if (FAILED(SetUp_ShaderResources()))
        return E_FAIL;

    m_pShaderCom->Begin(m_eShaderID);

    m_pVIBufferCom->Render();

    return S_OK;
}

_bool CTerrian::Picking(_float3* PickingPoint)
{
    if (m_pVIBufferCom->Picking(m_pTransformCom, PickingPoint) == true)
    {
        m_vMousePickPos = *PickingPoint;
        CTerrianManager::GetInstance()->Set_PickingWorldPos(m_vMousePickPos);
        return true;
    }
    else
    {
        ZeroMemory(&m_vMousePickPos, sizeof(_float3));
    }

    return false;
}

void CTerrian::PickingTrue()
{
    CImGui_Manager::PICKING_TYPE ePickingType = CImGui_Manager::GetInstance()->Get_PickingType();

    switch (ePickingType)
    {
    case Client::CImGui_Manager::PICKING_TERRAIN_TRANSFORM:
        Set_Picked();
        break;
    default:
        break;
    }

}

HRESULT CTerrian::Ready_Components(void* pArg)
{
    CTerrianManager::TERRIANDESC TerrianDesc;
    if (pArg != nullptr)
    {
        ZeroMemory(&TerrianDesc, sizeof(CTerrianManager::TERRIANDESC));
        memcpy(&TerrianDesc, pArg, sizeof(CTerrianManager::TERRIANDESC));
        m_eDebugtype = (TERRIAN_DEBUG_TYPE)TerrianDesc.m_eDebugTerrian;
    }

    /* For.Com_Renderer */
    if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRenderCom)))
        return E_FAIL;

    /* For.Com_Texture */
    if (FAILED(__super::Add_Components(TEXT("Com_Texture"), LEVEL_GAMEPLAY, TEXT("Prototype_Component_Texture_Terrain"), (CComponent**)&m_pTextureCom[TYPE_DIFFUSE])))
        return E_FAIL;

    if (pArg != nullptr)
    {
        /* For.Com_VIBuffer */
        if (FAILED(__super::Add_Components(TEXT("Com_VIBuffer"), LEVEL_STATIC, TEXT("Prototype_Component_VIBuffer_Terrain"), (CComponent**)&m_pVIBufferCom, &TerrianDesc.TerrianDesc)))
            return E_FAIL;
    }
    else
    {
        /* For.Com_VIBuffer */
        if (FAILED(__super::Add_Components(TEXT("Com_VIBuffer"), LEVEL_STATIC, TEXT("Prototype_Component_VIBuffer_Terrain_Height"), (CComponent**)&m_pVIBufferCom)))
            return E_FAIL;
    }


    /* For.Com_Shader */
    if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxNorTex"), (CComponent**)&m_pShaderCom)))
        return E_FAIL;

    /* For.Com_Transform */
    CTransform::TRANSFORMDESC		TransformDesc;
    TransformDesc.fRotationPerSec = 1.f;
    TransformDesc.fSpeedPerSec = 3.f;

    if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC, TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom, &TransformDesc)))
        return E_FAIL;

    _vector vInitPosition = XMVectorSet(TerrianDesc.TerrianDesc.m_iPositionX, TerrianDesc.TerrianDesc.m_fHeight, TerrianDesc.TerrianDesc.m_iPositionZ, 1.f);
    m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vInitPosition);

    return S_OK;
}

HRESULT CTerrian::SetUp_ShaderID()
{
    m_bWireFrame = CTerrianManager::GetInstance()->Get_TerrianDesc().m_bShowWireFrame;

    switch (m_eDebugtype)
    {
    case Client::CTerrian::DEBUG_SOLID:
        if (m_bWireFrame)
            m_eShaderID = WIRE;
        else
            m_eShaderID = DEFAULLT;
        break;
    case Client::CTerrian::DEBUG_WIRE:
        m_eShaderID = WIRE;
        break;
    case Client::CTerrian::DEBUG_NONE:
        if (m_bWireFrame)
            m_eShaderID = WIRE;
        else if (m_bPicked)
            m_eShaderID = PICKED;
        else
            m_eShaderID = DEFAULLT;
        break;
    default:
        break;
    }

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

    ID3D11ShaderResourceView* pSRVs[] = {
    m_pTextureCom[TYPE_DIFFUSE]->Get_SRV(0),
    };

    if (FAILED(m_pShaderCom->Set_ShaderResourceViewArray("g_DiffuseTexture", pSRVs, 1)))
        return E_FAIL;

    return S_OK;
}

void CTerrian::Set_Picked()
{
    if (m_pGameInstance->Mouse_Down(DIMK::DIMK_LBUTTON))
    {
        if (m_bPicked)
            CPickingMgr::GetInstance()->Set_PickedObj(nullptr);
        else
            CPickingMgr::GetInstance()->Set_PickedObj(this);
    }
}

void CTerrian::Save_Terrain(HANDLE hFile, _ulong* dwByte)
{
    m_pVIBufferCom->Save_VertexPosition(hFile, dwByte);
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
        MSG_BOX("Failed to Cloned : CTerrian");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CTerrian::Free()
{
    __super::Free();

    Safe_Delete_Array(pPixel);
    Safe_Delete_Array(pPixelColor);

    Safe_Release(m_pTransformCom);
    Safe_Release(m_pShaderCom);
    Safe_Release(m_pVIBufferCom);

    for (_uint i = 0; i < TYPE_END; ++i)
        Safe_Release(m_pTextureCom[i]);
}