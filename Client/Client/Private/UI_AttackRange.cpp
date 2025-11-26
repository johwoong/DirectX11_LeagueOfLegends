#include "UI_AttackRange.h"
#include "Champion.h"
#include "GameInstance.h"
#include "VIBuffer_Rect.h"
#include "Shader.h"
#include "Texture.h"
#include "Renderer.h"

USING(Client);

CAttackRange::CAttackRange(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CActor(pDevice, pContext)
{
}

CAttackRange::CAttackRange(const CAttackRange& rhs)
    : CActor(rhs)
{
}

HRESULT CAttackRange::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CAttackRange::Initialize(void* pArg)
{
    if (pArg)
        m_pOwner = static_cast<CChampion*>(pArg);

    if (FAILED(Ready_Components(pArg)))
        return E_FAIL;

    // 평면을 위로 향하게 (정확한 회전)
    m_pTransformCom->Set_RotationX(XMConvertToRadians(90.f)); // XY → XZ

    // 기본 스케일 (나중에 Update에서 다시 적용됨)
    Set_Scale(_float3(1.f, 1.f, 1.f));
    m_bVisible = false;
    return S_OK;
}

int CAttackRange::Update(_float fTimeDelta)
{
    if (!m_pOwner || !m_bVisible)
        return OBJ_NOEVENT;

    _vector vPos = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
    vPos = XMVectorSetY(vPos, XMVectorGetY(vPos) + 0.1f);
    m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vPos);

    float fRange = m_pOwner->Get_AttackRange(); // 550 같은 값

    // *** 월드 변환용 스케일 팩터 (직접 튜닝) ***
    const float fWorldScale = 0.02f;   // 일단 0.02로 시작, 크면 줄이고 작으면 키우기
    float fWorldRange = fRange * fWorldScale;

    m_pTransformCom->Set_Scale(CTransform::STATE_RIGHT, fWorldRange * 2.f);
    m_pTransformCom->Set_Scale(CTransform::STATE_LOOK, fWorldRange * 2.f);
    m_pTransformCom->Set_Scale(CTransform::STATE_UP, 1.f);

    __super::Update(fTimeDelta);
    return OBJ_NOEVENT;
}

void CAttackRange::Late_Update(_float fTimeDelta)
{
    if (!m_bVisible)
        return;

    if (m_pRendererCom)
        m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_ALPHABLEND, this);
}

HRESULT CAttackRange::Render()
{
    if (!m_bVisible) return S_OK;
    if (!m_pShaderCom || !m_pVIBufferCom) return E_FAIL;

    if (FAILED(SetUp_ShaderResources()))
        return E_FAIL;

    m_pShaderCom->Begin(0);
    m_pVIBufferCom->Render();
    return S_OK;
}

HRESULT CAttackRange::Ready_Components(void* pArg)
{
    // Renderer
    if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC,
        TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRendererCom)))
        return E_FAIL;

    // Transform
    CTransform::TRANSFORMDESC desc{};
    desc.fSpeedPerSec = 0.f;
    desc.fRotationPerSec = 0.f;
    if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC,
        TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom, &desc)))
        return E_FAIL;

    // Shader (노말 모델 아니고, 그냥 평면용 셰이더 하나 쓰면 됨)
    if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC,
        TEXT("Prototype_Component_Shader_VtxTex"), (CComponent**)&m_pShaderCom)))
        return E_FAIL;

    // Texture (원형 사거리 dds)
    if (FAILED(__super::Add_Components(TEXT("Com_Texture"), LEVEL_GAMEPLAY,
        TEXT("Prototype_Component_Texture_AttackRange"), (CComponent**)&m_pTextureCom)))
        return E_FAIL;

    // VIBuffer : 바닥 평면
    if (FAILED(__super::Add_Components(TEXT("Com_VIBuffer"), LEVEL_STATIC,
        TEXT("Prototype_Component_VIBuffer_Quad"), (CComponent**)&m_pVIBufferCom)))
        return E_FAIL;

    m_eShaderID = SHADER_NOANIM_DEFAULT; // 셰이더에 맞게

    return S_OK;
}

HRESULT CAttackRange::SetUp_ShaderResources()
{
    if (!m_pShaderCom) return E_FAIL;

    _float4x4 World = m_pTransformCom->Get_World_TP();
    _float4x4 View = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_VIEW);
    _float4x4 Proj = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_PROJ);

    m_pShaderCom->Set_RawValue("g_WorldMatrix", &World, sizeof(_float4x4));
    m_pShaderCom->Set_RawValue("g_ViewMatrix", &View, sizeof(_float4x4));
    m_pShaderCom->Set_RawValue("g_ProjMatrix", &Proj, sizeof(_float4x4));

    m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture", m_pTextureCom->Get_SRV(0));

    return S_OK;
}

CAttackRange* CAttackRange::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CAttackRange* pInstance = new CAttackRange(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed To Created : CAttackRange");
        Safe_Release(pInstance);
    }
    return pInstance;
}

CGameObject* CAttackRange::Clone(void* pArg)
{
    CAttackRange* pInstance = new CAttackRange(*this);
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed To Cloned : CAttackRange");
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CAttackRange::Free()
{
    __super::Free();
}
