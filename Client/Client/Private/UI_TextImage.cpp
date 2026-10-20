#include "UI_TextImage.h"
#include "GameInstance.h"

USING(Client)

CUI_TextImage::CUI_TextImage(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CUI_Base(pDevice, pContext)
{
}

CUI_TextImage::CUI_TextImage(const CUI_TextImage& rhs)
    : CUI_Base(rhs)
    , m_pTextureTextCom(rhs.m_pTextureTextCom)
    , m_tDesc(rhs.m_tDesc)
    , m_fAcc(0.f)
    , m_iTextTexIndex(rhs.m_iTextTexIndex)
    , m_vTextSize(rhs.m_vTextSize)
    , m_vTextPos(rhs.m_vTextPos)
{
    Safe_AddRef(m_pTextureTextCom);
}

HRESULT CUI_TextImage::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CUI_TextImage::Initialize(void* pArg)
{
    m_tDesc = TEXTIMAGEDESC{};

    if (pArg)
        memcpy(&m_tDesc, pArg, sizeof(TEXTIMAGEDESC));

    m_fAlpha = m_tDesc.fAlpha;
    m_eShaderID = UI_ALPHABLEND;

    m_vTextSize = m_tDesc.vSize;
    m_vTextPos = m_tDesc.vCenterPos;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    return S_OK;
}

int CUI_TextImage::Update(_float fTimeDelta)
{
    // 자동 삭제
    if (m_tDesc.bAutoDead)
    {
        m_fAcc += fTimeDelta;
        if (m_fAcc >= m_tDesc.fLifeTime)
        {
            m_bDead = true;              // UI_Base가 SetDead()를 쓰면 그걸로 바꿔
            return OBJ_DEAD;
        }
    }

    return OBJ_NOEVENT;
}

void CUI_TextImage::Late_Update(_float fTimeDelta)
{
    if (nullptr != m_pRendererCom)
        m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_UI_FRONT, this);
}

HRESULT CUI_TextImage::Render()
{
    if (nullptr == m_pShaderCom || nullptr == m_pVIBufferCom)
        return E_FAIL;

    if (FAILED(SetUp_ShaderID()))
        return E_FAIL;

    if (FAILED(SetUp_ShaderResources()))
        return E_FAIL;

    // "텍스트 이미지만" 렌더
    if (m_pTextureTextCom)
    {
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

        // Render() 텍스트 그리는 부분에서
        m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture", m_pTextureTextCom->Get_SRV(0));
        m_pShaderCom->Begin(m_eShaderID);
        m_pVIBufferCom->Render();
    }

    return S_OK;
}

HRESULT CUI_TextImage::Ready_Components(void* pArg)
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

    // ===== 텍스트 타입 결정 (pArg 우선) =====
    TEXT_TYPE eType = TEXT_TYPE::WELCOME_SUMMONERS_RIFT;
    if (pArg)
    {
        TEXTIMAGEDESC desc{};
        memcpy(&desc, pArg, sizeof(TEXTIMAGEDESC));
        eType = desc.eType;
    }

    // ===== 타입별 텍스처 컴포넌트 붙이기 =====
    const _tchar* pTexProto = TEXT("Prototype_Component_Texture_WelcomeText");

    switch (eType)
    {
    case TEXT_TYPE::WELCOME_SUMMONERS_RIFT:
        pTexProto = TEXT("Prototype_Component_Texture_WelcomeText");
        break;

    case TEXT_TYPE::MINION_HAS_SPAWNED:
        pTexProto = TEXT("Prototype_Component_Texture_MinionCreateText");
        break;

    default:
        pTexProto = TEXT("Prototype_Component_Texture_WelcomeText");
        break;
    }

    if (FAILED(__super::Add_Components(TEXT("Com_Texture_Text"), LEVEL_GAMEPLAY,
        pTexProto, (CComponent**)&m_pTextureTextCom)))
        return E_FAIL;

    return S_OK;
}

HRESULT CUI_TextImage::SetUp_ShaderResources()
{
    if (nullptr == m_pShaderCom)
        return E_FAIL;

    // UI_Base 방식 그대로
    _float4x4 WorldMatrix = m_pTransformCom->Get_World_TP();

    if (FAILED(m_pShaderCom->Set_RawValue("g_WorldMatrix", &WorldMatrix, sizeof(_float4x4))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Set_RawValue("g_ViewMatrix", &m_ViewMatrix, sizeof(_float4x4))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Set_RawValue("g_ProjMatrix", &m_ProjMatrix, sizeof(_float4x4))))
        return E_FAIL;

    // 알파를 쓰는 UI 쉐이더라면
    m_pShaderCom->Set_RawValue("g_fAlpha", &m_fAlpha, sizeof(float));

    return S_OK;
}

CUI_TextImage* CUI_TextImage::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CUI_TextImage* pInstance = new CUI_TextImage(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CUI_TextImage");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CUI_TextImage::Clone(void* pArg)
{
    CUI_TextImage* pInstance = new CUI_TextImage(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CUI_TextImage");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CUI_TextImage::Free()
{
    __super::Free();
}


