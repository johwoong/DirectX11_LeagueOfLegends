#include "UI_Minion_HpBar.h"
#include "Minion.h"
#include "GameInstance.h"

CMinion_HpBar::CMinion_HpBar(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CActor(pDevice, pContext)
{
}

CMinion_HpBar::CMinion_HpBar(const CMinion_HpBar& rhs)
    : CActor(rhs)
    , m_pVIBufferCom(rhs.m_pVIBufferCom)
    , m_pTextureCom(rhs.m_pTextureCom)
{
    Safe_AddRef(m_pVIBufferCom);
    Safe_AddRef(m_pTextureCom);
}

HRESULT CMinion_HpBar::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CMinion_HpBar::Initialize(void* pArg)
{
    m_pOwner = static_cast<CMinion*>(pArg);

    __super::Initialize(pArg);

    Ready_Components(pArg);

    // ★ 여기 추가 : Minion 쪽에도 나를 알려주기
    if (m_pOwner)
        m_pOwner->Set_HpBar(this);

    return S_OK;
}

HRESULT CMinion_HpBar::Ready_Components(void* pArg)
{
    // ===================== 1) Renderer =====================
    if (FAILED(__super::Add_Components(
        TEXT("Com_Renderer"), LEVEL_STATIC,
        TEXT("Prototype_Component_Renderer"),
        (CComponent**)&m_pRendererCom)))
        return E_FAIL;

    // ===================== 2) Transform =====================
    CTransform::TRANSFORMDESC desc{};
    desc.fSpeedPerSec = 0.f;
    desc.fRotationPerSec = 0.f;

    if (FAILED(__super::Add_Components(
        TEXT("Com_Transform"), LEVEL_STATIC,
        TEXT("Prototype_Component_Transform"),
        (CComponent**)&m_pTransformCom, &desc)))
        return E_FAIL;

    // ===================== 3) Shader =====================
    if (FAILED(__super::Add_Components(
        TEXT("Com_Shader"), LEVEL_STATIC,
        TEXT("Prototype_Component_Shader_VtxTex"),
        (CComponent**)&m_pShaderCom)))
        return E_FAIL;

    m_eShaderID = 0;

    // ===================== 4) Texture  (팀에 따라 다른 HPBar) =====================
    const _tchar* szTexProto = TEXT("Prototype_Component_Texture_Minion_HpBar"); // 기본 : 레드용

    if (m_pOwner && m_pOwner->Get_Team() == TEAM_BLUE)
    {
        szTexProto = TEXT("Prototype_Component_Texture_Minion_Minion_HpBar_Blue");
    }

    if (FAILED(__super::Add_Components(
        TEXT("Com_Texture"), LEVEL_GAMEPLAY,
        szTexProto,
        (CComponent**)&m_pTextureCom)))
        return E_FAIL;

    // ===================== 5) VIBuffer (Rect) =====================
    if (FAILED(__super::Add_Components(
        TEXT("Com_VIBuffer"), LEVEL_STATIC,
        TEXT("Prototype_Component_VIBuffer_Rect"),
        (CComponent**)&m_pVIBufferCom)))
        return E_FAIL;

    return S_OK;
}

int CMinion_HpBar::Update(_float fDelta)
{
    // 이미 죽은 상태면 바로 제거
    if (m_bDead)
        return OBJ_DEAD;

    // ★ 주인이 없거나, 주인이 완전히 Dead 상태면 나도 제거
    if (!m_pOwner || m_pOwner->Get_Dead())
    {
        m_bDead = true;
        return OBJ_DEAD;
    }

    // ★ Dying(죽는 모션 재생 중)일 땐 그냥 안 움직이고 안 그리게만 처리
    if (m_pOwner->Is_Dying())
        return OBJ_NOEVENT;

    Update_Position_And_Scale();
    return __super::Update(fDelta);
}

void CMinion_HpBar::Update_Position_And_Scale()
{
    if (!m_pOwner)
        return;

    // 1) 기본 위치 : 미니언 머리 위
    _vector pos = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
    pos = XMVectorSetY(pos, XMVectorGetY(pos) + m_fOffsetY);

    // HP 비율
    float cur = m_pOwner->Get_Stats().m_fCurrentHp;
    float max = m_pOwner->Get_Stats().m_fMaxHp;
    if (max <= 0.f) max = 1.f;

    float ratio = cur / max;
    ratio = max(0.f, min(1.f, ratio));

    // 2) 먼저 빌보드 세팅
    SetUp_BillBoard();

    // 3) 스케일 적용
    const float fullWidth = 0.7f;
    const float fullHeight = 0.1f;

    m_pTransformCom->Set_Scale(CTransform::STATE_RIGHT, fullWidth * ratio);
    m_pTransformCom->Set_Scale(CTransform::STATE_UP, fullHeight);

    // 4) 화면 기준으로 살짝 오른쪽으로 이동
    const float xOffset = 0.15f; // 여기 값만 취향대로 조절

    _vector vRight = XMVector3Normalize(
        m_pTransformCom->Get_State(CTransform::STATE_RIGHT)); // 빌보드의 Right 축

    pos += vRight * xOffset;

    // 5) 최종 위치 적용
    m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, pos);
}

void CMinion_HpBar::Set_Owner(CMinion* pOwner)
{
    m_pOwner = pOwner;

    if (m_pOwner)
        m_pOwner->Set_HpBar(this);
}

void CMinion_HpBar::Late_Update(_float fDelta)
{
    if (m_bDead) return;
    if (!m_pOwner) return;                       // ★ Owner null 방지
    if (m_pOwner->IsDeadOrDying()) return;       // ★ Owner가 죽은 경우 절대 렌더하지 않기

    m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_ALPHABLEND, this);
}

HRESULT CMinion_HpBar::SetUp_ShaderResources()
{
    _float4x4 world = m_pTransformCom->Get_World_TP();
    _float4x4 view = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_VIEW);
    _float4x4 proj = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_PROJ);

    m_pShaderCom->Set_RawValue("g_WorldMatrix", &world, sizeof(world));
    m_pShaderCom->Set_RawValue("g_ViewMatrix", &view, sizeof(view));
    m_pShaderCom->Set_RawValue("g_ProjMatrix", &proj, sizeof(proj));

    m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture", m_pTextureCom->Get_SRV(0));

    return S_OK;
}

HRESULT CMinion_HpBar::Render()
{
    SetUp_ShaderResources();

    m_pShaderCom->Begin(0);
    m_pVIBufferCom->Render();

    return S_OK;

}


CMinion_HpBar* CMinion_HpBar::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CMinion_HpBar* pInstance = new CMinion_HpBar(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed To Created : CMinion_HpBar");
        Safe_Release(pInstance);
    }
    return pInstance;
}

CGameObject* CMinion_HpBar::Clone(void* pArg)
{
    CMinion_HpBar* pInstance = new CMinion_HpBar(*this);
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed To Cloned : CMinion_HpBar");
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CMinion_HpBar::Free()
{
    __super::Free();

}
