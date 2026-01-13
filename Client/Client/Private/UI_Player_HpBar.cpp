#include "UI_Player_HpBar.h"
#include "GameInstance.h"
#include "PoolManager.h"
#include "Champion.h"

CPlayer_HpBar::CPlayer_HpBar(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CActor(pDevice, pContext)
{
}
CPlayer_HpBar::CPlayer_HpBar(const CPlayer_HpBar& rhs)

    : CActor(rhs)
    , m_pVIBufferCom(rhs.m_pVIBufferCom)
    , m_pTextureCom(rhs.m_pTextureCom)
{
    Safe_AddRef(m_pVIBufferCom);
    Safe_AddRef(m_pTextureCom);
}

HRESULT CPlayer_HpBar::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CPlayer_HpBar::Initialize(void* pArg)
{
    m_pOwner = static_cast<CChampion*>(pArg);

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (FAILED(Ready_Components(pArg)))
        return E_FAIL;

    // 풀/초기화 안전
    Set_Dead(false);

    return S_OK;
}

HRESULT CPlayer_HpBar::Ready_Components(void* pArg)
{
    // 1) Renderer
    if (FAILED(__super::Add_Components(
        TEXT("Com_Renderer"), LEVEL_STATIC,
        TEXT("Prototype_Component_Renderer"),
        (CComponent**)&m_pRendererCom)))
        return E_FAIL;

    // 2) Transform
    CTransform::TRANSFORMDESC desc{};
    desc.fSpeedPerSec = 0.f;
    desc.fRotationPerSec = 0.f;

    if (FAILED(__super::Add_Components(
        TEXT("Com_Transform"), LEVEL_STATIC,
        TEXT("Prototype_Component_Transform"),
        (CComponent**)&m_pTransformCom, &desc)))
        return E_FAIL;

    // 3) Shader
    if (FAILED(__super::Add_Components(
        TEXT("Com_Shader"), LEVEL_STATIC,
        TEXT("Prototype_Component_Shader_VtxTex"),
        (CComponent**)&m_pShaderCom)))
        return E_FAIL;

    m_eShaderID = 0;

    // 4) Texture (Bar + Frame)
    if (FAILED(__super::Add_Components(
        TEXT("Com_Texture"), LEVEL_GAMEPLAY,
        TEXT("Prototype_Component_Texture_PlayerHpBar_HpBar"),      // 바 색
        (CComponent**)&m_pTextureCom)))
        return E_FAIL;

    if (FAILED(__super::Add_Components(
        TEXT("Com_FrameTexture"), LEVEL_GAMEPLAY,
        TEXT("Prototype_Component_Texture_Champion_HpBar_Frame"), // 프레임
        (CComponent**)&m_pFrameTextureCom)))
        return E_FAIL;

    // 5) VIBuffer
    if (FAILED(__super::Add_Components(
        TEXT("Com_VIBuffer"), LEVEL_STATIC,
        TEXT("Prototype_Component_VIBuffer_Rect"),
        (CComponent**)&m_pVIBufferCom)))
        return E_FAIL;

    return S_OK;
}

int CPlayer_HpBar::Update(_float fDelta)
{
    if (m_bDead)
        return OBJ_DEAD;   // 풀에 들어가 있는 상태

    // 주인 없으면 숨김
    if (!m_pOwner)
    {
        m_bVisible = false;
        return OBJ_NOEVENT;
    }

    // ★ "죽음"은 Get_Dead가 아니라 Down 상태로 판단해야 함
    if (m_pOwner->Is_Down())
    {
        m_bVisible = false;
        return OBJ_NOEVENT; // 풀로 보내지 말고 그냥 숨기기
    }

    // 살아있으면 다시 보이기
    m_bVisible = true;

    Update_Position_And_Scale();
    return __super::Update(fDelta);
}

void CPlayer_HpBar::Update_Position_And_Scale()
{
    if (!m_pOwner)
        return;

    // 1) 위치
    _vector pos = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
    pos = XMVectorSetY(pos, XMVectorGetY(pos) + m_fOffsetY);

    // 2) HP 비율 (현재 HP / MaxHp)
    float cur = m_pOwner->Get_Stat().Hp;
    float max = m_pOwner->Get_Stat().MaxHp;

    if (max <= 0.f) max = 1.f;
    m_fHpRatio = cur / max;
    m_fHpRatio = max(0.f, min(1.f, m_fHpRatio));

    // 3) 빌보드
    SetUp_BillBoard();

    // 4) 프레임 기준 기본 스케일
    const float fullWidth = m_fMaxWidth;
    const float fullHeight = m_fHeight;

    m_pTransformCom->Set_Scale(CTransform::STATE_RIGHT, fullWidth);
    m_pTransformCom->Set_Scale(CTransform::STATE_UP, fullHeight);

    // 5) 살짝 오른쪽 오프셋
    _vector vRight = XMVector3Normalize(
        m_pTransformCom->Get_State(CTransform::STATE_RIGHT));

    const float xOffset = 0.15f;
    pos += vRight * xOffset;

    // 6) 최종 위치
    m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, pos);
}


void CPlayer_HpBar::Late_Update(_float fDelta)
{
    if (m_bDead) return;
    if (!m_pOwner) return;

    // ★ Down이면 렌더그룹에 안 넣어서 안 보이게
    if (!m_bVisible)
        return;

    m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_ALPHABLEND, this);
}

HRESULT CPlayer_HpBar::SetUp_ShaderResources()
{
    _float4x4 view = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_VIEW);
    _float4x4 proj = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_PROJ);

    m_pShaderCom->Set_RawValue("g_ViewMatrix", &view, sizeof(view));
    m_pShaderCom->Set_RawValue("g_ProjMatrix", &proj, sizeof(proj));

    return S_OK;
}

HRESULT CPlayer_HpBar::Render()
{
    // ★ 숨김 상태면 그냥 스킵
    if (m_bDead || !m_bVisible)
        return S_OK;

    if (!m_pShaderCom || !m_pVIBufferCom || !m_pTextureCom || !m_pFrameTextureCom)
        return E_FAIL;

    if (FAILED(SetUp_ShaderResources()))
        return E_FAIL;

    // ===== 아래는 너 기존 Render 그대로 =====
    _float4x4 worldNative = m_pTransformCom->Get_World();

    // ================== 1) Frame ==================
    _float4x4 frameNative = worldNative;

    _vector fRight = XMLoadFloat4((_float4*)&frameNative.m[0][0]);
    _vector fUp = XMLoadFloat4((_float4*)&frameNative.m[1][0]);
    _vector fPos = XMLoadFloat4((_float4*)&frameNative.m[3][0]);

    float baseWidth = XMVectorGetX(XMVector3Length(fRight));
    float baseHeight = XMVectorGetX(XMVector3Length(fUp));

    _vector rightDir = XMVector3Normalize(fRight);
    _vector upDir = XMVector3Normalize(fUp);

    const float fFrameScaleX = 1.15f;
    const float fFrameScaleY = 2.0f;

    float frameWidth = baseWidth * fFrameScaleX;
    float frameHeight = baseHeight * fFrameScaleY;

    fRight = rightDir * frameWidth;
    fUp = upDir * frameHeight;

    XMStoreFloat4((_float4*)&frameNative.m[0][0], fRight);
    XMStoreFloat4((_float4*)&frameNative.m[1][0], fUp);
    XMStoreFloat4((_float4*)&frameNative.m[3][0], fPos);

    _float4x4 worldFrameTP;
    XMStoreFloat4x4(&worldFrameTP, XMMatrixTranspose(XMLoadFloat4x4(&frameNative)));

    float fFrameAlpha = 1.0f;
    m_pShaderCom->Set_RawValue("g_fAlpha", &fFrameAlpha, sizeof(float));
    m_pShaderCom->Set_RawValue("g_WorldMatrix", &worldFrameTP, sizeof(worldFrameTP));
    m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture", m_pFrameTextureCom->Get_SRV(0));

    m_pShaderCom->Begin(1);
    m_pVIBufferCom->Render();

    // ================== 2) HP Bar ==================
    float fBarAlpha = 1.0f;
    m_pShaderCom->Set_RawValue("g_fAlpha", &fBarAlpha, sizeof(float));

    _float4x4 barNative = worldNative;

    _vector vRightOrig = XMLoadFloat4((_float4*)&barNative.m[0][0]);
    _vector vUpOrig = XMLoadFloat4((_float4*)&barNative.m[1][0]);
    _vector vPosOrig = XMLoadFloat4((_float4*)&barNative.m[3][0]);

    float barBaseWidth = XMVectorGetX(XMVector3Length(vRightOrig));
    float barBaseHeight = XMVectorGetX(XMVector3Length(vUpOrig));

    _vector barRightDir = XMVector3Normalize(vRightOrig);
    _vector barUpDir = XMVector3Normalize(vUpOrig);

    const float innerScaleX = 0.9f;
    const float innerScaleY = 0.8f;

    const float offsetX = 0.1f * barBaseWidth;
    const float offsetY = 0.2f * barBaseHeight;

    float innerWidth = barBaseWidth * innerScaleX;
    float innerHeight = barBaseHeight * innerScaleY;

    float barWidth = innerWidth * m_fHpRatio;
    float barHeight = innerHeight;

    float fullLeft = -barBaseWidth * 0.5f;
    float innerLeft = fullLeft + (barBaseWidth - innerWidth) * 0.5f;
    float barCenterLocal = innerLeft + barWidth * 0.5f;

    _vector vRightNew = barRightDir * barWidth;
    _vector vUpNew = barUpDir * barHeight;

    _vector vPosNew = vPosOrig
        + barRightDir * barCenterLocal
        + barRightDir * offsetX
        + barUpDir * offsetY;

    XMStoreFloat4((_float4*)&barNative.m[0][0], vRightNew);
    XMStoreFloat4((_float4*)&barNative.m[1][0], vUpNew);
    XMStoreFloat4((_float4*)&barNative.m[3][0], vPosNew);

    _float4x4 barTP;
    XMStoreFloat4x4(&barTP, XMMatrixTranspose(XMLoadFloat4x4(&barNative)));

    m_pShaderCom->Set_RawValue("g_WorldMatrix", &barTP, sizeof(barTP));
    m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture", m_pTextureCom->Get_SRV(0));

    m_pShaderCom->Begin(1);
    m_pVIBufferCom->Render();

    return S_OK;
}

void CPlayer_HpBar::Set_Owner(CChampion* pOwner)
{
    m_pOwner = pOwner;

    if (!m_pOwner)
        return;       // 끊을 때는 여기서 바로 종료

    //m_pOwner->Set_HpBar(this);

    // HPBar가 기준으로 사용할 MaxHp 스냅샷
    m_fMaxHp = m_pOwner->Get_Stat().MaxHp;
    if (m_fMaxHp <= 0.f)
        m_fMaxHp = 1.f;
}



CPlayer_HpBar* CPlayer_HpBar::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CPlayer_HpBar* pInstance = new CPlayer_HpBar(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed To Created : CPlayer_HpBar");
        Safe_Release(pInstance);
    }
    return pInstance;
}

CGameObject* CPlayer_HpBar::Clone(void* pArg)
{
    CPlayer_HpBar* pInstance = new CPlayer_HpBar(*this);
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed To Cloned : CPlayer_HpBar");
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CPlayer_HpBar::Free()
{
    __super::Free();

}