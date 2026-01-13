#include "UI_Enemy_HpBar.h"
#include "GameInstance.h"
#include "PoolManager.h"
#include "Enemy.h"

CEnemy_HpBar::CEnemy_HpBar(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CActor(pDevice, pContext)
{
}
CEnemy_HpBar::CEnemy_HpBar(const CEnemy_HpBar& rhs)

    : CActor(rhs)
    , m_pVIBufferCom(rhs.m_pVIBufferCom)
    , m_pTextureCom(rhs.m_pTextureCom)
{
    Safe_AddRef(m_pVIBufferCom);
    Safe_AddRef(m_pTextureCom);
}

HRESULT CEnemy_HpBar::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CEnemy_HpBar::Initialize(void* pArg)
{
    m_pOwner = static_cast<CEnemy*>(pArg);

    __super::Initialize(pArg);
    Ready_Components(pArg);

    return S_OK;
}

HRESULT CEnemy_HpBar::Ready_Components(void* pArg)
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
        TEXT("Prototype_Component_Texture_Enemy_HpBar"),      // 바 색
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

int CEnemy_HpBar::Update(_float fDelta)
{
    // owner 자체가 없어지면 그때만 진짜 제거
    if (!m_pOwner)
        return OBJ_DEAD;

    // owner가 죽음상태면 "숨김"만 (절대 OBJ_DEAD로 보내지마)
    if (m_pOwner->IsDeadForTargeting())
    {
        m_bVisible = false;
        return OBJ_NOEVENT;
    }

    m_bVisible = true;

    Update_Position_And_Scale();
    return __super::Update(fDelta);
}

void CEnemy_HpBar::Update_Position_And_Scale()
{
    if (!m_pOwner)
        return;


    // 1) 위치
    _vector pos = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
    pos = XMVectorSetY(pos, XMVectorGetY(pos) + m_fOffsetY);

    // 2) HP 비율 (현재 HP / MaxHp)
    CEnemy::STATS stat = m_pOwner->Get_Stats();
    float cur = stat.Hp;
    float max = stat.MaxHp;

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


void CEnemy_HpBar::Late_Update(_float fDelta)
{
    if (!m_bVisible) return;
    if (!m_pOwner) return;

    if (!m_pOwner->Is_Frustum(1.f))
        return;

    if (!Is_Frustum(0.1f))
        return;

    m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_ALPHABLEND, this);
}

HRESULT CEnemy_HpBar::SetUp_ShaderResources()
{
    _float4x4 view = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_VIEW);
    _float4x4 proj = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_PROJ);

    m_pShaderCom->Set_RawValue("g_ViewMatrix", &view, sizeof(view));
    m_pShaderCom->Set_RawValue("g_ProjMatrix", &proj, sizeof(proj));

    return S_OK;
}

HRESULT CEnemy_HpBar::Render()
{
    if (!m_bVisible)
        return S_OK;
    if (m_bDead)
        return S_OK;

    if (!m_pShaderCom || !m_pVIBufferCom || !m_pTextureCom || !m_pFrameTextureCom)
        return E_FAIL;

    if (FAILED(SetUp_ShaderResources()))
        return E_FAIL;

    // -----------------------------
    // 0) 공통: 월드 행렬 (HPBar 기준 크기)
    // -----------------------------
    _float4x4 worldNative = m_pTransformCom->Get_World();

    // ================== 1) Frame ==================
    // → HPBar보다 가로는 약간 크고, 세로는 많이 두껍게
    _float4x4 frameNative = worldNative;

    _vector fRight = XMLoadFloat4((_float4*)&frameNative.m[0][0]);
    _vector fUp = XMLoadFloat4((_float4*)&frameNative.m[1][0]);
    _vector fPos = XMLoadFloat4((_float4*)&frameNative.m[3][0]); // 위치는 그대로

    // 기준 길이
    float baseWidth = XMVectorGetX(XMVector3Length(fRight));
    float baseHeight = XMVectorGetX(XMVector3Length(fUp));

    _vector rightDir = XMVector3Normalize(fRight);
    _vector upDir = XMVector3Normalize(fUp);

    // ★ 프레임 비율 조절 (LoL 느낌)
    // 가로는 살짝만 크게, 세로는 많이 두껍게
    const float fFrameScaleX = 1.15f;   // 가로 약간 크게
    const float fFrameScaleY = 2.0f;    // 세로는 2배

    float frameWidth = baseWidth * fFrameScaleX;
    float frameHeight = baseHeight * fFrameScaleY;

    fRight = rightDir * frameWidth;
    fUp = upDir * frameHeight;

    XMStoreFloat4((_float4*)&frameNative.m[0][0], fRight);
    XMStoreFloat4((_float4*)&frameNative.m[1][0], fUp);
    XMStoreFloat4((_float4*)&frameNative.m[3][0], fPos);

    _float4x4 worldFrameTP;
    XMStoreFloat4x4(&worldFrameTP,
        XMMatrixTranspose(XMLoadFloat4x4(&frameNative)));

    float fFrameAlpha = 1.0f;
    m_pShaderCom->Set_RawValue("g_fAlpha", &fFrameAlpha, sizeof(float));
    m_pShaderCom->Set_RawValue("g_WorldMatrix", &worldFrameTP, sizeof(worldFrameTP));
    m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture", m_pFrameTextureCom->Get_SRV(0));

    // 프레임 렌더
    m_pShaderCom->Begin(1);
    m_pVIBufferCom->Render();


    // ================== 2) HP Bar ==================
    float fBarAlpha = 1.0f;
    m_pShaderCom->Set_RawValue("g_fAlpha", &fBarAlpha, sizeof(float));

    _float4x4 barNative = worldNative;   // HPBar 기준은 원래 스케일

    // 바 기준 벡터 / 길이
    _vector vRightOrig = XMLoadFloat4((_float4*)&barNative.m[0][0]);
    _vector vUpOrig = XMLoadFloat4((_float4*)&barNative.m[1][0]);
    _vector vPosOrig = XMLoadFloat4((_float4*)&barNative.m[3][0]);

    float barBaseWidth = XMVectorGetX(XMVector3Length(vRightOrig));
    float barBaseHeight = XMVectorGetX(XMVector3Length(vUpOrig));

    _vector barRightDir = XMVector3Normalize(vRightOrig);
    _vector barUpDir = XMVector3Normalize(vUpOrig);

    // ---------------- 튜닝 포인트 ----------------
    // 1) 프레임 안에서 HPBar “최대 영역” 비율
    const float innerScaleX = 0.9f;   // 가로: 1.0에 가까울수록 프레임 꽉참
    const float innerScaleY = 0.8f;   // 세로: 값 키우면 바가 두꺼워짐

    // 2) HPBar 위치 오프셋 (프레임 안에서 위/아래/좌우 이동)
    const float offsetX = 0.1f * barBaseWidth;   // +면 오른쪽, -면 왼쪽
    const float offsetY = 0.2f * barBaseHeight; // +면 위로, -면 아래로
    // --------------------------------------------

    // 프레임 안에서 풀체력 기준 가로/세로
    float innerWidth = barBaseWidth * innerScaleX;
    float innerHeight = barBaseHeight * innerScaleY;

    // HP 비율 적용된 실제 바 폭
    float barWidth = innerWidth * m_fHpRatio;
    float barHeight = innerHeight;

    // 좌우 위치: 프레임 안쪽 왼쪽에서 시작해서 오른쪽으로 차오르게
    float fullLeft = -barBaseWidth * 0.5f;                 // 전체 바 기준 왼쪽
    float innerLeft = fullLeft + (barBaseWidth - innerWidth) * 0.5f;
    float barCenterLocal = innerLeft + barWidth * 0.5f;

    // 새 Right/Up/Pos 설정
    _vector vRightNew = barRightDir * barWidth;
    _vector vUpNew = barUpDir * barHeight;

    // offsetX, offsetY 적용
    _vector vPosNew = vPosOrig
        + barRightDir * barCenterLocal   // 좌우 위치
        + barRightDir * offsetX          // 추가 좌우 이동
        + barUpDir * offsetY;         // 위/아래 이동

    XMStoreFloat4((_float4*)&barNative.m[0][0], vRightNew);
    XMStoreFloat4((_float4*)&barNative.m[1][0], vUpNew);
    XMStoreFloat4((_float4*)&barNative.m[3][0], vPosNew);

    _float4x4 barTP;
    XMStoreFloat4x4(&barTP,
        XMMatrixTranspose(XMLoadFloat4x4(&barNative)));

    m_pShaderCom->Set_RawValue("g_WorldMatrix", &barTP, sizeof(barTP));
    m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture", m_pTextureCom->Get_SRV(0));

    m_pShaderCom->Begin(1);
    m_pVIBufferCom->Render();

    return S_OK;
}

void CEnemy_HpBar::Set_Owner(CEnemy* pOwner)
{
    m_pOwner = pOwner;
    if (!m_pOwner) return;

    m_bVisible = !m_pOwner->IsDeadForTargeting();

    m_fMaxHp = m_pOwner->Get_Stats().MaxHp;
    if (m_fMaxHp <= 0.f)
        m_fMaxHp = 1.f;
}



CEnemy_HpBar* CEnemy_HpBar::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CEnemy_HpBar* pInstance = new CEnemy_HpBar(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed To Created : CChampion_HpBar");
        Safe_Release(pInstance);
    }
    return pInstance;
}

CGameObject* CEnemy_HpBar::Clone(void* pArg)
{
    CEnemy_HpBar* pInstance = new CEnemy_HpBar(*this);
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed To Cloned : CChampion_HpBar");
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CEnemy_HpBar::Free()
{
    __super::Free();
}
