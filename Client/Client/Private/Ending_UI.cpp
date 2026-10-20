#include "Ending_UI.h"
#include "GameInstance.h"

USING(Client)

CEnding_UI::CEnding_UI(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CUI_Base(pDevice, pContext)
{
}

CEnding_UI::CEnding_UI(const CEnding_UI& rhs)
    : CUI_Base(rhs)
    , m_pTextureTextCom(rhs.m_pTextureTextCom)
{
    Safe_AddRef(m_pTextureTextCom);
}

HRESULT CEnding_UI::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CEnding_UI::Initialize(void* pArg)
{
    const float boardW = g_WinSizeX * 0.23f;   
    const float boardH = g_WinSizeY * 0.25f;   

    m_fSize = { boardW, boardH };
    m_fPosition = { g_WinSizeX * 0.5f, g_WinSizeY * 0.5f - boardH * 0.35f };
    m_fAlpha = 1.f;
    m_eShaderID = UI_ALPHABLEND;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    m_vTextSize = { boardW * 0.60f, boardH * 0.28f };            
   m_vTextPos = { m_fPosition.x, m_fPosition.y - boardH * 0.02f };

    return S_OK;
}

int CEnding_UI::Update(_float fTimeDelta)
{
    return OBJ_NOEVENT;
}

void CEnding_UI::Late_Update(_float fTimeDelta)
{
    if (nullptr != m_pRendererCom)
        m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_UI_FRONT, this); // 최상단에 뜨게
}

HRESULT CEnding_UI::Render()
{
    if (nullptr == m_pShaderCom || nullptr == m_pVIBufferCom)
        return E_FAIL;

    if (FAILED(SetUp_ShaderID()))
        return E_FAIL;

    if (FAILED(SetUp_ShaderResources()))
        return E_FAIL;

    // 1) 배경(풀스크린)
    m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture", m_pTextureCom->Get_SRV(0));
    m_pShaderCom->Begin(m_eShaderID);
    m_pVIBufferCom->Render();

    // 2) 텍스트(작게, 중앙)
    if (m_pTextureTextCom)
    {
        // UI_Base가 보통 쓰는 방식(화면 좌표 -> 오쏘 좌표)
        _matrix matText =
            XMMatrixScaling(m_vTextSize.x, m_vTextSize.y, 1.f) *
            XMMatrixTranslation(
                m_vTextPos.x - g_WinSizeX * 0.5f,
                -(m_vTextPos.y - g_WinSizeY * 0.5f),
                0.f
            );

        _float4x4 TextWorldTP;
        XMStoreFloat4x4(&TextWorldTP, XMMatrixTranspose(matText));

        m_pShaderCom->Set_RawValue("g_WorldMatrix", &TextWorldTP, sizeof(_float4x4));

        m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture", m_pTextureTextCom->Get_SRV(0));
        m_pShaderCom->Begin(m_eShaderID);
        m_pVIBufferCom->Render();
    }

    return S_OK;
}

HRESULT CEnding_UI::Ready_Components(void* pArg)
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

    // VIBuffer
    if (FAILED(__super::Add_Components(TEXT("Com_VIBuffer"), LEVEL_STATIC,
        TEXT("Prototype_Component_VIBuffer_Rect"), (CComponent**)&m_pVIBufferCom)))
        return E_FAIL;

    // Texture 1 : Ending Base (eog_base.dds)
    if (FAILED(__super::Add_Components(TEXT("Com_Texture"), LEVEL_GAMEPLAY,
        TEXT("Prototype_Component_Texture_Ending"), (CComponent**)&m_pTextureCom)))
        return E_FAIL;

    // Texture 2 : Ending Text/Board (Texture.dds)
    if (FAILED(__super::Add_Components(TEXT("Com_Texture_Text"), LEVEL_GAMEPLAY,
        TEXT("Prototype_Component_Texture_EndingText"), (CComponent**)&m_pTextureTextCom)))
        return E_FAIL;

    return S_OK;
}

HRESULT CEnding_UI::SetUp_ShaderResources()
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

CEnding_UI* CEnding_UI::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CEnding_UI* pInstance = new CEnding_UI(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CEnding_UI");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CEnding_UI::Clone(void* pArg)
{
    CEnding_UI* pInstance = new CEnding_UI(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CEnding_UI");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CEnding_UI::Free()
{
    __super::Free();
}
