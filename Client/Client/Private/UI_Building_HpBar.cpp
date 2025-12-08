#include "UI_Building_HpBar.h"
#include "GameInstance.h"
#include "PoolManager.h"
#include "Building.h"

CBuilding_HpBar::CBuilding_HpBar(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CActor(pDevice, pContext)
{
}
CBuilding_HpBar::CBuilding_HpBar(const CBuilding_HpBar& rhs)

    : CActor(rhs)
    , m_pVIBufferCom(rhs.m_pVIBufferCom)
    , m_pTextureCom(rhs.m_pTextureCom)
{
    Safe_AddRef(m_pVIBufferCom);
    Safe_AddRef(m_pTextureCom);
}

HRESULT CBuilding_HpBar::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CBuilding_HpBar::Initialize(void* pArg)
{
    m_pOwner = static_cast<CBuilding*>(pArg);

    __super::Initialize(pArg);
    Ready_Components(pArg);

    return S_OK;
}

HRESULT CBuilding_HpBar::Ready_Components(void* pArg)
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
        TEXT("Prototype_Component_Texture_Building_HpBar"),      // 바 색
        (CComponent**)&m_pTextureCom)))
        return E_FAIL;

    if (FAILED(__super::Add_Components(
        TEXT("Com_FrameTexture"), LEVEL_GAMEPLAY,
        TEXT("Prototype_Component_Texture_Building_HpBar_Frame"), // 프레임
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

int CBuilding_HpBar::Update(_float fDelta)
{
    if (m_bDead)
        return OBJ_DEAD;   // 풀에 들어가 있는 상태

    // ★ 주인이 없거나, 주인이 완전히 Dead 상태면 나도 풀로 돌려보냄
    if (!m_pOwner || m_pOwner->Get_Dead())
    {
        return OBJ_DEAD;
    }

    Update_Position_And_Scale();
    return __super::Update(fDelta);
}

void CBuilding_HpBar::Update_Position_And_Scale()
{
    if (!m_pOwner)
        return;

    // 1) 위치
    _vector pos = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
    pos = XMVectorSetY(pos, XMVectorGetY(pos) + m_fOffsetY);

    // 2) HP 비율 (현재 HP / MaxHp)
    float cur = m_pOwner->GetInfo().m_fCurrentHp;
    float max = m_fMaxHp;           

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


void CBuilding_HpBar::Late_Update(_float fDelta)
{
    if (m_bDead) return;
    if (!m_pOwner) return;
    if (m_pOwner->Get_Dead()) return;

    if (!m_pOwner->Is_Frustum(1.f))
        return;

    if (!Is_Frustum(0.1f))
        return;

    m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_ALPHABLEND, this);
}

HRESULT CBuilding_HpBar::SetUp_ShaderResources()
{
    _float4x4 view = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_VIEW);
    _float4x4 proj = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_PROJ);

    m_pShaderCom->Set_RawValue("g_ViewMatrix", &view, sizeof(view));
    m_pShaderCom->Set_RawValue("g_ProjMatrix", &proj, sizeof(proj));

    return S_OK;
}

HRESULT CBuilding_HpBar::Render()
{
    if (m_bDead)
        return S_OK;

    if (!m_pShaderCom || !m_pVIBufferCom || !m_pTextureCom || !m_pFrameTextureCom)
        return E_FAIL;

    if (FAILED(SetUp_ShaderResources()))
        return E_FAIL;

    _float4x4 worldNative = m_pTransformCom->Get_World();

    /* ================== Frame ================== */
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

    XMStoreFloat4((_float4*)&frameNative.m[0][0], rightDir * frameWidth);
    XMStoreFloat4((_float4*)&frameNative.m[1][0], upDir * frameHeight);
    XMStoreFloat4((_float4*)&frameNative.m[3][0], fPos);

    _float4x4 worldFrameTP;
    XMStoreFloat4x4(&worldFrameTP,
        XMMatrixTranspose(XMLoadFloat4x4(&frameNative)));

    float alpha = 1.f;
    m_pShaderCom->Set_RawValue("g_fAlpha", &alpha, sizeof(float));
    m_pShaderCom->Set_RawValue("g_WorldMatrix", &worldFrameTP, sizeof(worldFrameTP));
    m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture",
        m_pFrameTextureCom->Get_SRV(0));

    m_pShaderCom->Begin(1);
    m_pVIBufferCom->Render();


    /* ================== HP Bar ================== */
    _float4x4 barNative = frameNative;

    _vector vRightFrame = XMLoadFloat4((_float4*)&barNative.m[0][0]);
    _vector vUpFrame = XMLoadFloat4((_float4*)&barNative.m[1][0]);
    _vector vPosFrame = XMLoadFloat4((_float4*)&barNative.m[3][0]);

    frameWidth = XMVectorGetX(XMVector3Length(vRightFrame));
    frameHeight = XMVectorGetX(XMVector3Length(vUpFrame));

    rightDir = XMVector3Normalize(vRightFrame);
    upDir = XMVector3Normalize(vUpFrame);

    /* ?? 미세 튜닝 포인트 */
    const float marginLeft = 0.062f;
    const float marginRight = 0.062f;
    const float marginTop = 0.35f;
    const float marginBottom = 0.30f;

    float leftX = -frameWidth * 0.5f;
    float rightX = frameWidth * 0.5f;
    float bottomY = -frameHeight * 0.5f;
    float topY = frameHeight * 0.5f;

    float innerLeft = leftX + frameWidth * marginLeft;
    float innerRight = rightX - frameWidth * marginRight;
    float innerBottom = bottomY + frameHeight * marginBottom;
    float innerTop = topY - frameHeight * marginTop;

    float innerWidth = innerRight - innerLeft;
    float innerHeight = innerTop - innerBottom;

    float barWidth = innerWidth * m_fHpRatio;
    float barHeight = innerHeight;

    float barCenterX = innerLeft + barWidth * 0.5f;

    // ? “진짜 살짝” 위로
    float barCenterY = (innerBottom + innerTop) * 0.565f;
    barCenterY += 0.012f * frameHeight;

    _vector vRightNew = rightDir * barWidth;
    _vector vUpNew = upDir * barHeight;
    _vector vPosNew = vPosFrame
        + rightDir * barCenterX
        + upDir * barCenterY;

    XMStoreFloat4((_float4*)&barNative.m[0][0], vRightNew);
    XMStoreFloat4((_float4*)&barNative.m[1][0], vUpNew);
    XMStoreFloat4((_float4*)&barNative.m[3][0], vPosNew);

    _float4x4 barTP;
    XMStoreFloat4x4(&barTP,
        XMMatrixTranspose(XMLoadFloat4x4(&barNative)));

    m_pShaderCom->Set_RawValue("g_WorldMatrix", &barTP, sizeof(barTP));
    m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture",
        m_pTextureCom->Get_SRV(0));

    m_pShaderCom->Begin(1);
    m_pVIBufferCom->Render();

    return S_OK;
}

void CBuilding_HpBar::Set_Owner(CBuilding* pOwner)
{
    m_pOwner = pOwner;

    if (!m_pOwner)
        return;

    // ★ 1) Owner에게서 HpBar 설정값 받아오기
    CBuilding::HPBARDESC desc = m_pOwner->Get_HpBarDesc();
    m_fMaxWidth = desc.fWidth;
    m_fHeight = desc.fHeight;
    m_fOffsetY = desc.fOffsetY;

    // ★ 2) MaxHp 스냅샷
    m_fMaxHp = m_pOwner->GetInfo().m_fMaxHp;
    if (m_fMaxHp <= 0.f)
        m_fMaxHp = 1.f;
}



CBuilding_HpBar* CBuilding_HpBar::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CBuilding_HpBar* pInstance = new CBuilding_HpBar(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed To Created : CBuilding_HpBar");
        Safe_Release(pInstance);
    }
    return pInstance;
}

CGameObject* CBuilding_HpBar::Clone(void* pArg)
{
    CBuilding_HpBar* pInstance = new CBuilding_HpBar(*this);
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed To Cloned : CChampion_HpBar");
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CBuilding_HpBar::Free()
{
    __super::Free();

}
