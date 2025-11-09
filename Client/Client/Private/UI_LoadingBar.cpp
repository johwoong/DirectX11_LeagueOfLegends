#include "UI_LoadingBar.h"
#include "GameInstance.h"

CUI_LoadingBar::CUI_LoadingBar(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CUI_Base(pDevice, pContext)
{
}

CUI_LoadingBar::CUI_LoadingBar(const CUI_LoadingBar& rhs)
    : CUI_Base(rhs)
{
}

HRESULT CUI_LoadingBar::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CUI_LoadingBar::Initialize(void* pArg)
{
    if (pArg)
        m_pLoader = reinterpret_cast<CLoader*>(pArg);

    m_fSize.x = m_fMaxWidth;
    m_fSize.y = m_fHeight;
    m_fPosition.x = 1100.f;
    m_fPosition.y = 550.f;
    m_eShaderID = UI_ALPHABLEND;

    // ? 왼쪽 고정점 저장
    m_fAnchorLeftX = m_fPosition.x - (m_fMaxWidth * 0.5f);
    m_fAnchorY = m_fPosition.y;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    CGameObject* pObj = nullptr;
    if (FAILED(m_pGameInstance->Add_GameObject_Out(
        TEXT("Prototype_GameObject_UI_Text"),
        LEVEL_LOADING,
        TEXT("UI_LOADING"),
        pObj)))
        return E_FAIL;

    m_pPercentText = dynamic_cast<CUI_Text*>(pObj);

    if (m_pPercentText)
    {
        m_pPercentText->Set_Text(L"0%");
        m_pPercentText->Set_Scale(0.8f);
        m_pPercentText->Set_Color(_float4(1.f, 1.f, 1.f, 1.f));
        m_pPercentText->Set_Position(_float2(m_fPosition.x - 130.f, m_fPosition.y - 45.f));
    }
    return S_OK;
}

int CUI_LoadingBar::Update(_float fTimeDelta)
{
    if (!m_pLoader)
        return OBJ_NOEVENT;

    float fTargetProgress = m_pLoader->Get_Progress();

    float fFollowSpeed = 1.5f;
    m_fProgress += (fTargetProgress - m_fProgress) * fFollowSpeed * fTimeDelta;

    if (m_fProgress < 0.f)   m_fProgress = 0.f;
    if (m_fProgress > 100.f) m_fProgress = 100.f;

    m_fCurrentWidth = (m_fProgress / 100.f) * m_fMaxWidth;
    if (m_fCurrentWidth < 1.f) m_fCurrentWidth = 1.f;

    m_fSize.x = m_fCurrentWidth;
    m_fPosition.x = m_fAnchorLeftX + (m_fCurrentWidth * 0.5f);

    // ? 여기서 텍스트 갱신 추가
    if (m_pPercentText)
    {
        wchar_t szBuffer[16] = {};
        swprintf_s(szBuffer, L"%d%%", (int)m_fProgress);
        m_pPercentText->Set_Text(szBuffer);
    }

    return __super::Update(fTimeDelta);
}

void CUI_LoadingBar::Late_Update(_float fTimeDelta)
{
    if (m_pRendererCom)
        m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_UI_FRONT, this);
}

HRESULT CUI_LoadingBar::Render()
{
    if (!m_pShaderCom || !m_pVIBufferCom)
        return E_FAIL;

    if (FAILED(SetUp_ShaderResources()))
        return E_FAIL;

    m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture", m_pTextureCom->Get_SRV(0));
    m_pShaderCom->Begin(m_eShaderID);
    m_pVIBufferCom->Render();
    return S_OK;
}

HRESULT CUI_LoadingBar::Ready_Components(void* pArg)
{
    if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRendererCom)))
        return E_FAIL;

    if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC, TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom)))
        return E_FAIL;

    if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC, TEXT("Prototype_Component_Shader_UI"), (CComponent**)&m_pShaderCom)))
        return E_FAIL;

    if (FAILED(__super::Add_Components(TEXT("Com_Texture"), LEVEL_STATIC, TEXT("Prototype_Component_Texture_Loading_Bar"), (CComponent**)&m_pTextureCom)))
        return E_FAIL;

    if (FAILED(__super::Add_Components(TEXT("Com_VIBuffer"), LEVEL_STATIC, TEXT("Prototype_Component_VIBuffer_Rect"), (CComponent**)&m_pVIBufferCom)))
        return E_FAIL;

    return S_OK;
}

HRESULT CUI_LoadingBar::SetUp_ShaderResources()
{
    _float4x4 WorldMatrix = m_pTransformCom->Get_World_TP();
    m_pShaderCom->Set_RawValue("g_WorldMatrix", &WorldMatrix, sizeof(_float4x4));
    m_pShaderCom->Set_RawValue("g_ViewMatrix", &m_ViewMatrix, sizeof(_float4x4));
    m_pShaderCom->Set_RawValue("g_ProjMatrix", &m_ProjMatrix, sizeof(_float4x4));
    return S_OK;
}

CUI_LoadingBar* CUI_LoadingBar::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CUI_LoadingBar* pInstance = new CUI_LoadingBar(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype()))
        Safe_Release(pInstance);
    return pInstance;
}

CGameObject* CUI_LoadingBar::Clone(void* pArg)
{
    CUI_LoadingBar* pInstance = new CUI_LoadingBar(*this);
    if (FAILED(pInstance->Initialize(pArg)))
        Safe_Release(pInstance);
    return pInstance;
}

void CUI_LoadingBar::Free()
{
    __super::Free();
}
