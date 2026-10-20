#include "UI_SkillImage.h"

#include "GameInstance.h"
#include "PlayerManager.h"
#include "Champion.h"
CUI_SkillImage::CUI_SkillImage(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CUI_Base(pDevice, pContext)
{
}

CUI_SkillImage::CUI_SkillImage(const CUI_SkillImage& rhs)
    : CUI_Base(rhs)
{
}

HRESULT CUI_SkillImage::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CUI_SkillImage::Initialize(void* pArg)
{
    m_fSize = { 60.f, 60.f };

    const float fBaseY = 970.f;
    const float fCenterX = 915.f;

    std::wstring initTag;

    if (pArg)
    {
        auto pDesc = reinterpret_cast<SkillDesc*>(pArg);
        m_iSlotIndex = pDesc->SlotIndex;
        initTag = pDesc->IconTag;

        switch (m_iSlotIndex)
        {
        case 0: // Passive
            m_fPosition = { 758.f, 960.f };
            m_fSize = { 44.f, 44.f };
            break;
        case 1: // Q
            m_fPosition = { 825.f, fBaseY };
            break;
        case 2: // W
            m_fPosition = { 898.f, fBaseY };
            break;
        case 3: // E
            m_fPosition = { 974.f, fBaseY };
            break;
        case 4: // R
            m_fPosition = { 1053.f, fBaseY };
            break;
        default:
            m_iSlotIndex = -1;
            break;
        }
    }
    else
    {
        m_fPosition = { fCenterX, fBaseY };
    }

    m_eShaderID = UI_DEFAULT;
    m_fAlpha = 1.f;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    // ★ 초기 아이콘 세팅(성공 시 m_CurIconTag 갱신됨)
    if (!initTag.empty())
        Set_Icon(initTag);

    return S_OK;
}

int CUI_SkillImage::Update(_float fTimeDelta)
{
    const int iRet = __super::Update(fTimeDelta);
    if (iRet == OBJ_DEAD)
        return OBJ_DEAD;

    if (m_iSlotIndex < 0 || m_iSlotIndex > 4)
        return OBJ_NOEVENT;

    // ★ 활성 챔피언의 스킬 아이콘 태그를 매 프레임 확인해서 변경
    CPlayer* pActive = CPlayerManager::GetInstance()->Get_ActivePlayer();
    CChampion* pChamp = dynamic_cast<CChampion*>(pActive);
    if (!pChamp)
        return OBJ_NOEVENT;

    const CChampion::SkillInfo* skills = pChamp->Get_Skills();
    const std::wstring& newTag = skills[m_iSlotIndex].IconTag;

    if (!newTag.empty() && newTag != m_CurIconTag)
        Set_Icon(newTag);

    return OBJ_NOEVENT;
}

void CUI_SkillImage::Late_Update(_float fTimeDelta)
{
    if (nullptr != m_pRendererCom)
        m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_UI_FRONT, this);
}

HRESULT CUI_SkillImage::Render()
{
    if (nullptr == m_pShaderCom || nullptr == m_pVIBufferCom || nullptr == m_pTextureCom)
        return E_FAIL;

    if (FAILED(SetUp_ShaderID()))
        return E_FAIL;

    if (FAILED(SetUp_ShaderResources()))
        return E_FAIL;

    m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture", m_pTextureCom->Get_SRV(0));

    m_pShaderCom->Begin(m_eShaderID);
    m_pVIBufferCom->Render();

    return S_OK;
}

HRESULT CUI_SkillImage::Ready_Components(void* pArg)
{
    if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC,
        TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRendererCom)))
        return E_FAIL;

    if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC,
        TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom)))
        return E_FAIL;

    if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC,
        TEXT("Prototype_Component_Shader_UI"), (CComponent**)&m_pShaderCom)))
        return E_FAIL;

    if (FAILED(__super::Add_Components(TEXT("Com_VIBuffer"), LEVEL_STATIC,
        TEXT("Prototype_Component_VIBuffer_Rect"), (CComponent**)&m_pVIBufferCom)))
        return E_FAIL;

    return S_OK;
}

HRESULT CUI_SkillImage::SetUp_ShaderResources()
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

void CUI_SkillImage::Set_Icon(const std::wstring& IconTag)
{
    if (IconTag.empty())
        return;

    if (m_CurIconTag == IconTag && m_pTextureCom)
        return;

    // ★ 새 텍스처 먼저 확보 (실패하면 기존 텍스처 유지)
    CTexture* pNewTex = static_cast<CTexture*>(
        m_pGameInstance->Clone_Component(LEVEL_STATIC, IconTag.c_str())
        );

    if (!pNewTex)
    {
        const int curLv = m_pGameInstance->Get_CurrentLevelIndex();
        pNewTex = static_cast<CTexture*>(
            m_pGameInstance->Clone_Component(curLv, IconTag.c_str())
            );
    }

    if (!pNewTex)
        return;

    // 성공했을 때만 교체
    Safe_Release(m_pTextureCom);
    m_pTextureCom = pNewTex;
    m_CurIconTag = IconTag;
}

CUI_SkillImage* CUI_SkillImage::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CUI_SkillImage* pInstance = new CUI_SkillImage(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Cloned : CUI_SkillImage");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CUI_SkillImage::Clone(void* pArg)
{
    CUI_SkillImage* pInstance = new CUI_SkillImage(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CUI_Skillbutton");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CUI_SkillImage::Free()
{
    // m_pTextureCom 은 Set_Icon 에서 직접 Clone 한 것이라 m_Components 에 없다. 여기서 직접 해제한다.
    Safe_Release(m_pTextureCom);
    __super::Free();
}