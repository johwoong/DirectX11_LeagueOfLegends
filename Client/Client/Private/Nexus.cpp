// =======================
// Nexus.cpp
// =======================
#include "Nexus.h"
#include "GameInstance.h"
#include "Effect.h"

USING(Client)

CNexus::CNexus(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CBuilding(pDevice, pContext)
{
}

CNexus::CNexus(const CNexus& rhs)
    : CBuilding(rhs)
{
}

HRESULT CNexus::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CNexus::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    m_eState = IDLE;
    if (m_pModelCom)
        m_pModelCom->Set_CurrentAnimIndex(m_eState);

    Set_Scale(_float3(0.01f, 0.01f, 0.01f));

    // ★ 게임오버/죽음 연출 상태 리셋
    m_bDestroyed = false;
    m_bDeathFxPlayed = false;
    m_vecDeathFx.clear();

    return S_OK;
}

bool CNexus::IsRedTeam() const
{
    // Level에서 BuildDesc.eTeamType을 CBuilding::RED/BLUE로 넣고 있음
    return (m_BuildDesc.eTeamType == CBuilding::RED);
}

_int CNexus::Take_Damage(int iDamage, CActor* DamageCauser)
{
    // ★ 블루 넥서스는 절대 안 터지게
    if (!IsRedTeam())
        return 0;

    return __super::Take_Damage(iDamage, DamageCauser);
}

_int CNexus::Update(_float fTimeDelta)
{
    __super::Update(fTimeDelta);

    // ============================
    // ★ 레드 넥서스가 죽으면 삭제되기 전에 "죽음 연출"로 전환
    // ============================
    if (IsRedTeam() && !m_bDestroyed && m_bDead)
    {
        // 베이스에서 OBJ_DEAD 처리되기 전에 가로채기
        m_bDead = false;
        Begin_Destroy();
    }

    // (안전장치) 혹시라도 블루가 죽는 플래그가 찍히면 강제 복구
    if (!IsRedTeam() && m_bDead)
        m_bDead = false;

    Update_State(fTimeDelta);
    return OBJ_NOEVENT;
}

void CNexus::Late_Update(_float fTimeDelta)
{
    __super::Late_Update(fTimeDelta);
    LateUpdate_State(fTimeDelta);
}

_bool CNexus::Is_AnimationLoop(_uint eAnimId)
{
    switch ((STATE)eAnimId)
    {
    case DEATH: return false; // ★ 죽음은 루프 X
    default:    return true;
    }
}

void CNexus::Update_State(_float fTimeDelta)
{
    if (!m_pModelCom)
        return;

    float fSpeed = 1.f;

    switch (m_eState)
    {
    case IDLE:
        fSpeed = 4.f;
        m_pModelCom->Play_Animation(fTimeDelta * fSpeed, true);
        break;

    case DEATH:
        fSpeed = 2.5f; // ★ 죽음 애니 속도
        m_pModelCom->Play_Animation(fTimeDelta * fSpeed, false);
        break;

    default:
        m_pModelCom->Play_Animation(fTimeDelta, true);
        break;
    }
}

void CNexus::LateUpdate_State(_float fTimeDelta)
{
}

CBuilding::HPBARDESC CNexus::Get_HpBarDesc() const
{
    HPBARDESC desc;
    desc.fWidth = 4.f;
    desc.fHeight = 0.5f;
    desc.fOffsetY = 5.f;
    return desc;
}

void CNexus::Begin_Destroy()
{
    m_bDestroyed = true;
    m_eState = DEATH;

    if (m_pModelCom)
        m_pModelCom->Set_CurrentAnimIndex(DEATH);

    // ★ 폭발 FX 1회
    if (!m_bDeathFxPlayed && m_pTransformCom)
    {
        m_bDeathFxPlayed = true;

        _matrix world = m_pTransformCom->Get_WorldMatrix();
        m_vecDeathFx = CEffect::PlayEffectAtLocation(TEXT("Nexus_Explosion.dat"), world);

        for (auto& fx : m_vecDeathFx)
        {
            if (!fx) continue;
            fx->Set_WorldPosition(world);
        }
    }
}

void CNexus::Tick_GameOverSequence(_float fUnscaledDelta)
{
    if (!m_bDestroyed || !m_pModelCom)
        return;

    // ★ 월드DT=0이어도 죽음 애니는 계속
    const float fDeathSpeed = 2.5f; // Update_State(DEATH)랑 동일
    m_pModelCom->Play_Animation(fUnscaledDelta * fDeathSpeed, false);

    // FX도 계속 틱(원하면 * fDeathSpeed 가능)
    for (auto& fx : m_vecDeathFx)
    {
        if (!fx) continue;
        fx->Update(fUnscaledDelta);
    }
}

HRESULT CNexus::Ready_Components(void* pArg)
{
    if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC,
        TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRendererCom)))
        return E_FAIL;

    CTransform::TRANSFORMDESC TransformDesc;
    ZeroMemory(&TransformDesc, sizeof(CTransform::TRANSFORMDESC));
    TransformDesc.fSpeedPerSec = 2.5f;
    TransformDesc.fRotationPerSec = XMConvertToRadians(90.0f);

    if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC,
        TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom, &TransformDesc)))
        return E_FAIL;

    if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC,
        TEXT("Prototype_Component_Shader_VtxAnimModel"), (CComponent**)&m_pShaderCom)))
        return E_FAIL;

    _tchar szModeltag[MAX_PATH] = TEXT("");
    MultiByteToWideChar(CP_ACP, 0, m_BuildDesc.ModelDesc.pModeltag,
        (int)strlen(m_BuildDesc.ModelDesc.pModeltag), szModeltag, MAX_PATH);

    if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY,
        szModeltag, (CComponent**)&m_pModelCom)))
        return E_FAIL;

    CColider::COLLIDERDESC ColliderDesc;
    ZeroMemory(&ColliderDesc, sizeof(CColider::COLLIDERDESC));
    ColliderDesc.vScale = _float3(7.f, 7.f, 7.f);
    ColliderDesc.vRotation = _float3(0.f, 0.f, 0.f);
    ColliderDesc.vPosition = _float3(0.f, 14.f, 0.f);

    if (FAILED(__super::Add_Components(TEXT("Com_SPHERE"), LEVEL_STATIC,
        TEXT("Prototype_Component_Colider_Sphere"), (CComponent**)&m_pSphereCom, &ColliderDesc)))
        return E_FAIL;

    return S_OK;
}

CNexus* CNexus::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CNexus* pInstance = new CNexus(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CNexus");
        Safe_Release(pInstance);
    }
    return pInstance;
}

CGameObject* CNexus::Clone(void* pArg)
{
    CNexus* pInstance = new CNexus(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CNexus");
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CNexus::Free()
{
    __super::Free();
}