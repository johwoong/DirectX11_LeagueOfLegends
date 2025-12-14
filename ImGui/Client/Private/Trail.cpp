#include "Trail.h"

CTrail::CTrail(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CActor(pDevice, pContext)
{
}

CTrail::CTrail(const CTrail& rhs)
    : CActor(rhs)
{
}

HRESULT CTrail::Initialize_Prototype()
{
    if (FAILED(__super::Initialize_Prototype()))
        return E_FAIL;

    return S_OK;
}

HRESULT CTrail::Initialize(void* pArg)
{
    if (FAILED(Ready_Components(pArg)))
        return E_FAIL;

    return S_OK;
}

int CTrail::Update(_float fTimeDelta)
{

    return OBJ_NOEVENT;
}

void CTrail::Late_Update(_float fTimeDelta)
{
}

HRESULT CTrail::Render()
{
    if (nullptr == m_pShaderCom ||
        nullptr == m_pVIBufferCom)
        return E_FAIL;

    if (FAILED(SetUp_ShaderResources()))
        return E_FAIL;

    m_pShaderCom->Begin(m_eShaderID);

    m_pVIBufferCom->Render();

    return S_OK;
}

void CTrail::Update_Trail_Point(_float3 vPointDown, _float3 vPointUp, _fmatrix vWeaponMatrix)
{
    m_vPointDown = vPointDown;
    m_vPoint_Up = vPointUp;

    XMStoreFloat4x4(&m_WeaponMatrix, vWeaponMatrix);
}

HRESULT CTrail::Ready_Components(void* pArg)
{
    /* For.Com_Renderer */
    if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRenderCom)))
        return E_FAIL;

    /* For.Com_Transform */
    CTransform::TRANSFORMDESC		TransformDesc;
    ZeroMemory(&TransformDesc, sizeof(CTransform::TRANSFORMDESC));

    TransformDesc.fSpeedPerSec = 5.0f;
    TransformDesc.fRotationPerSec = XMConvertToRadians(1.0f);
    if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC, TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom, &TransformDesc)))
        return E_FAIL;

    /* For.Com_Texture */
    if (FAILED(__super::Add_Components(TEXT("Com_Texture"), LEVEL_STATIC, TEXT("Prototype_Component_Texture_SwordTrail"), (CComponent**)&m_pTextureCom)))
        return E_FAIL;

    /* For.Com_VIBuffer */
    if (FAILED(__super::Add_Components(TEXT("Com_VIBuffer"), LEVEL_STATIC, TEXT("Prototype_Component_VIBuffer_Trail"), (CComponent**)&m_pVIBufferCom)))
        return E_FAIL;
    return S_OK;
}

HRESULT CTrail::SetUp_ShaderID()
{
    return S_OK;
}

HRESULT CTrail::SetUp_ShaderResources()
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

CTrail* CTrail::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CTrail* pInstance = new CTrail(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CTrail");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CTrail::Clone(void* pArg)
{
    CTrail* pInstance = new CTrail(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CTrail");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CTrail::Free()
{
    __super::Free();
    Safe_Release(m_pVIBufferCom);
}
