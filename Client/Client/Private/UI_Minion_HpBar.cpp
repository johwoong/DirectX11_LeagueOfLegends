#include "UI_Minion_HpBar.h"
#include "Minion.h"
#include "GameInstance.h"
#include "PoolManager.h"

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
    const _tchar* szFrameTexProto = TEXT("Prototype_Component_Texture_Minion_Minion_HpBar_Frame"); // 프레임

    if (m_pOwner && m_pOwner->Get_Team() == TEAM_BLUE)
    {
        szTexProto = TEXT("Prototype_Component_Texture_Minion_Minion_HpBar_Blue");
    }

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

int CMinion_HpBar::Update(_float fDelta)
{
    if (m_bDead)
        return OBJ_POOL;   // 풀에 들어가 있는 상태

    // ★ 주인이 없거나, 주인이 완전히 Dead 상태면 나도 풀로 돌려보냄
    if (!m_pOwner || m_pOwner->Get_Dead())
    {
        ReturnToPool();
        return OBJ_POOL;
    }

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

    m_fHpRatio = cur / max;
    m_fHpRatio = max(0.f, min(1.f, m_fHpRatio));

    // 2) 빌보드
    SetUp_BillBoard();

    // 3) Transform 스케일은 "프레임 기준" 고정값
    const float fullWidth = 0.7f;
    const float fullHeight = 0.1f;

    m_pTransformCom->Set_Scale(CTransform::STATE_RIGHT, fullWidth);
    m_pTransformCom->Set_Scale(CTransform::STATE_UP, fullHeight);

    // 4) 살짝 오른쪽 오프셋
    _vector vRight = XMVector3Normalize(
        m_pTransformCom->Get_State(CTransform::STATE_RIGHT));

    const float xOffset = 0.15f;
    pos += vRight * xOffset;

    // 5) 최종 위치
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
    if (!m_pOwner) return;
    if (m_pOwner->IsDeadOrDying()) return;

    // 1) 미니언이 카메라 프러스텀 밖이면 HpBar도 스킵
    if (!m_pOwner->Is_Frustum(m_pOwner->Check_CullingRadius()))
        return;

    // 2) 혹시 HpBar 자체도 한번 더 체크 (원하면)
    if (!Is_Frustum(0.1f))
        return;

    m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_ALPHABLEND, this);
}

HRESULT CMinion_HpBar::SetUp_ShaderResources()
{
    _float4x4 view = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_VIEW);
    _float4x4 proj = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_PROJ);

    m_pShaderCom->Set_RawValue("g_ViewMatrix", &view, sizeof(view));
    m_pShaderCom->Set_RawValue("g_ProjMatrix", &proj, sizeof(proj));

    return S_OK;
}

HRESULT CMinion_HpBar::Render()
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

void CMinion_HpBar::ReUse_Setting(void* pArg)
{
    // 풀에서 꺼낼 때 다시 세팅
    m_bDead = false;
    m_pOwner = static_cast<CMinion*>(pArg);
    m_fHpRatio = 1.f;

    if (m_pOwner)
        m_pOwner->Set_HpBar(this);
}

// 풀로 되돌릴 때 호출
void CMinion_HpBar::ReturnToPool()
{
    if (m_bDead)
        return;

    // 소유자 기준으로 팀 결정
    TEAMTYPE eTeam = TEAM_BLUE;
    if (m_pOwner)
        eTeam = m_pOwner->Get_Team();

    // 팀별 PoolTag 분리 (렌더 레이어가 아님!)
    const _tchar* szPoolTag = nullptr;
    if (eTeam == TEAM_BLUE)
        szPoolTag = TEXT("Pool_UI_MinionHp_Blue");
    else
        szPoolTag = TEXT("Pool_UI_MinionHp_Red");

    m_bDead = true;
    m_pOwner = nullptr;

    if (CPoolManager* pPool = CPoolManager::GetInstance())
    {
        pPool->Add_Pooling_Object(
            LEVEL_GAMEPLAY,
            szPoolTag,
            this);
    }
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
