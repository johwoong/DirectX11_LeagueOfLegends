#include "UI_BackGround.h"
#include "GameInstance.h"
#include "Champion.h"
#include "NonAnim.h"


CUI_BackGround::CUI_BackGround(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CUI_Base(pDevice, pContext)
{
}

CUI_BackGround::CUI_BackGround(const CUI_BackGround& rhs)
    : CUI_Base(rhs)
{
}


HRESULT CUI_BackGround::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CUI_BackGround::Initialize(void* pArg)
{
    m_eShaderID = UI_DEFAULT;

    // 스크린 전체 크기
    m_fSize.x = g_WinSizeX;
    m_fSize.y = g_WinSizeY;
    m_fPosition.x = g_WinSizeX >> 1;
    m_fPosition.y = g_WinSizeY >> 1;


    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;


    m_pTransformCom->Set_Scale(CTransform::STATE_RIGHT, m_fSize.x);
    m_pTransformCom->Set_Scale(CTransform::STATE_UP, m_fSize.y);

    m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, XMVectorSet(m_fPosition.x - g_WinSizeX * 0.5f, -m_fPosition.y + g_WinSizeY * 0.5f, 0.f, 1.f));

    return S_OK;
}

int CUI_BackGround::Update(_float fTimeDelta)
{
    __super::Update(fTimeDelta);

    return OBJ_NOEVENT;
}

void CUI_BackGround::Late_Update(_float fTimeDelta)
{
    __super::Late_Update(fTimeDelta);
}

HRESULT CUI_BackGround::Render()
{
    if (nullptr == m_pShaderCom || nullptr == m_pVIBufferCom)
        return E_FAIL;

    if (FAILED(SetUp_ShaderID()))
        return E_FAIL;

    if (FAILED(SetUp_ShaderResources()))
        return E_FAIL;

    int iCurrentIdx = m_pGameInstance->Get_CurrentLevelIndex();
    if (iCurrentIdx == LEVEL_LOGO)
        m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture", m_pTextureCom->Get_SRV(0));
    else
        m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture", m_pTextureCom->Get_SRV(1));

    m_pShaderCom->Begin(m_eShaderID);
    m_pVIBufferCom->Render();

    return S_OK;
}


HRESULT CUI_BackGround::Ready_Components(void* pArg)
{
    // Renderer
    if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC,
        TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRendererCom)))
        return E_FAIL;

    // Transform
    if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC,
        TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom)))
        return E_FAIL;

    // Shader
    if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC,
        TEXT("Prototype_Component_Shader_UI"), (CComponent**)&m_pShaderCom)))
        return E_FAIL;

    // Texture
    if (FAILED(__super::Add_Components(TEXT("Com_Texture"), LEVEL_STATIC,
        TEXT("Prototype_Component_Texture_Logo"), (CComponent**)&m_pTextureCom)))
        return E_FAIL;

    // VIBuffer
    if (FAILED(__super::Add_Components(TEXT("Com_VIBuffer"), LEVEL_STATIC,
        TEXT("Prototype_Component_VIBuffer_Rect"), (CComponent**)&m_pVIBufferCom)))
        return E_FAIL;

    return S_OK;
}

HRESULT CUI_BackGround::SetUp_ShaderResources()
{
    if (nullptr == m_pShaderCom)
        return E_FAIL;

    _float4x4 WorldMatrix = m_pTransformCom->Get_World_TP();

    if (FAILED(m_pShaderCom->Set_RawValue("g_WorldMatrix", &WorldMatrix, sizeof(_float4x4))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Set_RawValue("g_ViewMatrix", &m_ViewMatrix, sizeof(_float4x4))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Set_RawValue("g_ProjMatrix", &m_ProjMatrix, sizeof(_float4x4))))
        return E_FAIL;

    return S_OK;
}

CUI_BackGround* CUI_BackGround::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CUI_BackGround* pInstance = new CUI_BackGround(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : UI_BackGround");
        Safe_Release(pInstance);
    }
    return pInstance;
}


CGameObject* CUI_BackGround::Clone(void* pArg)
{
    CUI_BackGround* pInstance = new CUI_BackGround(*this);
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Created : UI_BackGround");
        Safe_Release(pInstance);
    }
    return pInstance;
}



void CUI_BackGround::Free()
{
    __super::Free();
}

