#include "UI_Champion_HpBar.h"
#include "GameInstance.h"
#include "PoolManager.h"

CChampion_HpBar::CChampion_HpBar(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CActor(pDevice, pContext)
{
}

CChampion_HpBar::CChampion_HpBar(const CChampion_HpBar& rhs)
    : CActor(rhs)
    , m_pVIBufferCom(rhs.m_pVIBufferCom)
    , m_pTextureCom(rhs.m_pTextureCom)
{
    Safe_AddRef(m_pVIBufferCom);
    Safe_AddRef(m_pTextureCom);
}

HRESULT CChampion_HpBar::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CChampion_HpBar::Initialize(void* pArg)
{
    m_pOwner = static_cast<CActor*>(pArg);

    __super::Initialize(pArg);

    Ready_Components(pArg);


    return S_OK;
}

HRESULT CChampion_HpBar::Ready_Components(void* pArg)
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
    const _tchar* szFrameTexProto = TEXT("Prototype_Component_Texture_Minion_Minion_HpBar_Frame"); // 프레임


    if (FAILED(__super::Add_Components(
        TEXT("Com_Texture"), LEVEL_GAMEPLAY,
        szTexProto,
        (CComponent**)&m_pTextureCom)))
        return E_FAIL;

    if (FAILED(__super::Add_Components(
        TEXT("Com_FrameTexture"), LEVEL_GAMEPLAY,
        szFrameTexProto,
        (CComponent**)&m_pFrameTextureCom)))
        return E_FAIL;

    // ===================== 5) VIBuffer (Rect) =====================
    if (FAILED(__super::Add_Components(
        TEXT("Com_VIBuffer"), LEVEL_STATIC,
        TEXT("Prototype_Component_VIBuffer_Rect"),
        (CComponent**)&m_pVIBufferCom)))
        return E_FAIL;

    return S_OK;
}

int CChampion_HpBar::Update(_float fDelta)
{
    if (m_bDead)
        return OBJ_POOL;   // 풀에 들어가 있는 상태

    // ★ 주인이 없거나, 주인이 완전히 Dead 상태면 나도 풀로 돌려보냄
    if (!m_pOwner || m_pOwner->Get_Dead())
    {
        ReturnToPool();
        return OBJ_POOL;
    }

    Update_Position_And_Scale();
    return __super::Update(fDelta);
}

void CChampion_HpBar::Update_Position_And_Scale()
{
    if (!m_pOwner)
        return;

    // 1) 기본 위치 : 미니언 머리 위
    _vector pos = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
    pos = XMVectorSetY(pos, XMVectorGetY(pos) + m_fOffsetY);

    const float fullHeight = 0.1f;


    // 4) 살짝 오른쪽 오프셋
    _vector vRight = XMVector3Normalize(
        m_pTransformCom->Get_State(CTransform::STATE_RIGHT));

    const float xOffset = 0.15f;
    pos += vRight * xOffset;

    // 5) 최종 위치
    m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, pos);
}

void CChampion_HpBar::Set_Owner(CActor* pOwner)
{
    m_pOwner = pOwner;
}

void CChampion_HpBar::Late_Update(_float fDelta)
{
    if (m_bDead) return;
    if (!m_pOwner) return;
    if (!m_pOwner->Is_Frustum(1.f))
        return;

    if (!Is_Frustum(0.1f))
        return;

    m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_ALPHABLEND, this);
}

HRESULT CChampion_HpBar::SetUp_ShaderResources()
{
    _float4x4 view = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_VIEW);
    _float4x4 proj = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_PROJ);

    m_pShaderCom->Set_RawValue("g_ViewMatrix", &view, sizeof(view));
    m_pShaderCom->Set_RawValue("g_ProjMatrix", &proj, sizeof(proj));

    return S_OK;
}

HRESULT CChampion_HpBar::Render()
{
    if (!m_pShaderCom || !m_pVIBufferCom || !m_pTextureCom || !m_pFrameTextureCom)
        return E_FAIL;

    if (FAILED(SetUp_ShaderResources()))
        return E_FAIL;

    _float4x4 worldNative = m_pTransformCom->Get_World();

    // ================== 1) Frame ==================
    _float4x4 worldFrameTP;
    XMStoreFloat4x4(&worldFrameTP,
        XMMatrixTranspose(XMLoadFloat4x4(&worldNative)));

    float fFrameAlpha = 0.3f;
    m_pShaderCom->Set_RawValue("g_fAlpha", &fFrameAlpha, sizeof(float));
    m_pShaderCom->Set_RawValue("g_WorldMatrix", &worldFrameTP, sizeof(worldFrameTP));
    m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture", m_pFrameTextureCom->Get_SRV(0));

    // ?? 여기 중요: pass 1 사용 (PS_HPBar)
    m_pShaderCom->Begin(1);
    m_pVIBufferCom->Render();

    // ================== 2) HP Bar ==================
    float fBarAlpha = 1.0f;    // HP바는 완전 불투명
    m_pShaderCom->Set_RawValue("g_fAlpha", &fBarAlpha, sizeof(float));

    _float4x4 barNative = worldNative;

    _vector vRightOrig = XMLoadFloat4((_float4*)&barNative.m[0][0]);
    _vector vUpOrig = XMLoadFloat4((_float4*)&barNative.m[1][0]);
    _vector vPosOrig = XMLoadFloat4((_float4*)&barNative.m[3][0]);

    float   fullWidth = XMVectorGetX(XMVector3Length(vRightOrig));
    _vector rightDir = XMVector3Normalize(vRightOrig);

    const float innerScale = 0.97f;
    float innerWidth = fullWidth * innerScale;
    float barWidth = innerWidth * m_fHpRatio;
    float marginSide = (fullWidth - innerWidth) * 0.5f;

    float centerOffset =
        (-fullWidth * 0.5f) +
        marginSide +
        barWidth * 0.5f;

    _vector vRightNew = rightDir * barWidth;
    _vector vPosNew = vPosOrig + rightDir * centerOffset;

    XMStoreFloat4((_float4*)&barNative.m[0][0], vRightNew);
    XMStoreFloat4((_float4*)&barNative.m[1][0], vUpOrig);
    XMStoreFloat4((_float4*)&barNative.m[3][0], vPosNew);

    _float4x4 barTP;
    XMStoreFloat4x4(&barTP,
        XMMatrixTranspose(XMLoadFloat4x4(&barNative)));

    m_pShaderCom->Set_RawValue("g_WorldMatrix", &barTP, sizeof(barTP));
    m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture", m_pTextureCom->Get_SRV(0));

    // ?? HPBar도 pass 1
    m_pShaderCom->Begin(1);
    m_pVIBufferCom->Render();

    return S_OK;

}

void CChampion_HpBar::ReUse_Setting(void* pArg)
{
    // 풀에서 꺼낼 때 다시 세팅
    m_bDead = false;
    m_pOwner = static_cast<CActor*>(pArg);
    m_fHpRatio = 1.f;

}

// 풀로 되돌릴 때 호출
void CChampion_HpBar::ReturnToPool()
{
    if (m_bDead)
        return;


    m_bDead = true;
    m_pOwner = nullptr;

    if (CPoolManager* pPool = CPoolManager::GetInstance())
    {
        pPool->Add_Pooling_Object(
            LEVEL_GAMEPLAY,
            TEXT("Layer_HpBar_Champion"),
            this);
    }
}


CChampion_HpBar* CChampion_HpBar::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CChampion_HpBar* pInstance = new CChampion_HpBar(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed To Created : CChampion_HpBar");
        Safe_Release(pInstance);
    }
    return pInstance;
}

CGameObject* CChampion_HpBar::Clone(void* pArg)
{
    CChampion_HpBar* pInstance = new CChampion_HpBar(*this);
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed To Cloned : CChampion_HpBar");
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CChampion_HpBar::Free()
{
    __super::Free();

}
