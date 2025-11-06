#include "UI_Text.h"
#include "GameInstance.h"

CUI_Text::CUI_Text(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CUI_Base(pDevice, pContext)
{
}

CUI_Text::CUI_Text(const CUI_Text& rhs)
    : CUI_Base(rhs)
{
    m_strText = rhs.m_strText;
    m_vColor = rhs.m_vColor;
    m_fScale = rhs.m_fScale;
}

HRESULT CUI_Text::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CUI_Text::Initialize(void* pArg)
{
    if (pArg != nullptr)
        m_strText = *(wstring*)pArg;

    // 디폴트 위치 UI 중앙
    m_fPosition = _float2(g_WinSizeX * 0.5f, g_WinSizeY * 0.5f);

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    return S_OK;
}

int CUI_Text::Update(_float fTimeDelta)
{
    return __super::Update(fTimeDelta);
}

void CUI_Text::Late_Update(_float fTimeDelta)
{
    if (m_pRendererCom)
        m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_UI_FRONT, this);
}

HRESULT CUI_Text::Render()
{
    if (m_strText.empty())
        return S_OK;

    CGameInstance* pGI = CGameInstance::GetInstance();

    // 화면 좌표 → SpriteBatch 기준 좌표로 변경
    XMVECTOR vPos = XMVectorSet(
        m_fPosition.x - (g_WinSizeX * 0.5f),
        -(m_fPosition.y - (g_WinSizeY * 0.5f)),
        0.f, 1.f
    );

    pGI->Render_Font(TEXT("Font_Message"),
        m_strText.c_str(),
        vPos,
        XMLoadFloat4(&m_vColor),
        m_fScale);

    return S_OK;
}

HRESULT CUI_Text::Ready_Components(void* pArg)
{
    // Renderer & Transform 만 필요함
    if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRendererCom)))
        return E_FAIL;

    if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC, TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom)))
        return E_FAIL;

    return S_OK;
}

HRESULT CUI_Text::SetUp_ShaderResources()
{
    // 텍스트는 쉐이더를 사용하지 않으므로 아무것도 하지 않음
    return S_OK;
}

CUI_Text* CUI_Text::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CUI_Text* pInstance = new CUI_Text(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype()))
        Safe_Release(pInstance);
    return pInstance;
}

CGameObject* CUI_Text::Clone(void* pArg)
{
    CUI_Text* pInstance = new CUI_Text(*this);
    if (FAILED(pInstance->Initialize(pArg)))
        Safe_Release(pInstance);
    return pInstance;
}

void CUI_Text::Free()
{
    __super::Free();
}

