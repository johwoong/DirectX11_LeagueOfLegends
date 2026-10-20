#include "UI_ChampionIcon.h"
#include "GameInstance.h"

CUI_ChampionIcon::CUI_ChampionIcon(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CUI_Base(pDevice, pContext)
{
}

CUI_ChampionIcon::CUI_ChampionIcon(const CUI_ChampionIcon& rhs)
    : CUI_Base(rhs)
{
}

HRESULT CUI_ChampionIcon::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CUI_ChampionIcon::Initialize(void* pArg)
{
    m_fSize = { 160.f, 160.f };
    m_fPosition = { 590.f, 984.f };
    m_fAlpha = 1.f;
    m_eShaderID = UI_ALPHABLEND;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    return S_OK;
}

int CUI_ChampionIcon::Update(_float fTimeDelta)
{
    return OBJ_NOEVENT;
}

void CUI_ChampionIcon::Late_Update(_float fTimeDelta)
{
    if (m_pRendererCom)
        m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_UI_FRONT, this);
}

HRESULT CUI_ChampionIcon::Render()
{
    if (!m_pShaderCom || !m_pVIBufferCom || !m_pTextureCom)
        return E_FAIL;

    SetUp_ShaderID();
    SetUp_ShaderResources();

    m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture", m_pTextureCom->Get_SRV(0));

    m_pShaderCom->Begin(m_eShaderID);
    m_pVIBufferCom->Render();

    return S_OK;
}

HRESULT CUI_ChampionIcon::Ready_Components(void* pArg)
{
    __super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRendererCom);
    __super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC, TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom);
    __super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC, TEXT("Prototype_Component_Shader_UI"), (CComponent**)&m_pShaderCom);
    __super::Add_Components(TEXT("Com_VIBuffer"), LEVEL_STATIC, TEXT("Prototype_Component_VIBuffer_Rect"), (CComponent**)&m_pVIBufferCom);
    return S_OK;
}

HRESULT CUI_ChampionIcon::SetUp_ShaderResources()
{
    _float4x4 WorldMatrix = m_pTransformCom->Get_World_TP();
    m_pShaderCom->Set_RawValue("g_WorldMatrix", &WorldMatrix, sizeof(WorldMatrix));
    m_pShaderCom->Set_RawValue("g_ViewMatrix", &m_ViewMatrix, sizeof(m_ViewMatrix));
    m_pShaderCom->Set_RawValue("g_ProjMatrix", &m_ProjMatrix, sizeof(m_ProjMatrix));
    return S_OK;
}

void CUI_ChampionIcon::Set_Icon(const wstring& IconTag)
{
    Safe_Release(m_pTextureCom);
    m_pTextureCom = static_cast<CTexture*>(CGameInstance::GetInstance()->Clone_Component(LEVEL_GAMEPLAY, IconTag.c_str()));

}

CUI_ChampionIcon* CUI_ChampionIcon::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CUI_ChampionIcon* pInstance = new CUI_ChampionIcon(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype()))
        Safe_Release(pInstance);
    return pInstance;
}

CGameObject* CUI_ChampionIcon::Clone(void* pArg)
{
    CUI_ChampionIcon* pInstance = new CUI_ChampionIcon(*this);
    if (FAILED(pInstance->Initialize(pArg)))
        Safe_Release(pInstance);
    return pInstance;
}

void CUI_ChampionIcon::Free()
{
    // m_pTextureCom 은 Set_Icon 에서 직접 Clone 한 것이라 m_Components 에 없다. 여기서 직접 해제한다.
    Safe_Release(m_pTextureCom);
    __super::Free();
}