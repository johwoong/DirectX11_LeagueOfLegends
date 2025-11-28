#include "Minion.h"
#include "MinionManager.h"
#include "PoolManager.h"
#include "PickingManager.h"
#include "CursorManager.h"
#include "Champion.h"
#include "PlayerManager.h"

CMinion::CMinion(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CActor(pDevice, pContext)
{
}

CMinion::CMinion(const CMinion& rhs)
    : CActor(rhs)
    , m_tStats(rhs.m_tStats)
    , m_eState(rhs.m_eState)
    , m_eMinionType(rhs.m_eMinionType)
    , m_eTeam(rhs.m_eTeam)
    , m_eLane(rhs.m_eLane)
    , m_fAttackCoolTime(rhs.m_fAttackCoolTime)
    , m_fAttackTimer(rhs.m_fAttackTimer)
    , m_iWaveIndex(rhs.m_iWaveIndex)
    , m_iLaneIndex(rhs.m_iLaneIndex)
    , m_iGoldReward(rhs.m_iGoldReward)
    , m_iExpReward(rhs.m_iExpReward)
    , m_pModelCom(rhs.m_pModelCom)
    , m_pNavigationCom(rhs.m_pNavigationCom)
    , m_pTarget(rhs.m_pTarget)
    , m_pOrginDamageCauser(rhs.m_pOrginDamageCauser)
{
    Safe_AddRef(m_pModelCom);
    Safe_AddRef(m_pNavigationCom);
}

// ======================= Initialize =======================

HRESULT CMinion::Initialize_Prototype()
{
    return __super::Initialize_Prototype();
}

HRESULT CMinion::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (pArg)
    {
        MINION_SPAWNDESC* pDesc = (MINION_SPAWNDESC*)pArg;
        m_eTeam = pDesc->eTeam;
        m_eLane = pDesc->eLane;
        m_eMinionType = pDesc->eMinionType;
        m_iLaneIndex = pDesc->iLaneIndex;
        m_iWaveIndex = pDesc->iWaveIndex;
        m_vTargetPos = pDesc->vTargetPos;
    }

    Compute_LaneOffset();

    if (CMinionManager* mgr = CMinionManager::GetInstance())
    {
        const auto& path = mgr->GetLanePath(m_eLane);
        m_iLaneNodeIndex = mgr->GetLaneStartIndex(m_eTeam, path);
    }

    if (FAILED(Ready_Components(pArg)))
        return E_FAIL;

    CNavigation::NAVDESC NaviDesc{};
    if (FAILED(__super::Add_Components(
        TEXT("Com_Navigation"), LEVEL_STATIC,
        TEXT("Prototype_Component_ChampionNavigation"),
        (CComponent**)&m_pNavigationCom, &NaviDesc)))
        return E_FAIL;

    m_tStats.m_fCurrentHp = m_tStats.m_fMaxHp;

    m_fAttackCoolTime = (m_tStats.m_fAttackSpeed > 0.f)
        ? 1.f / m_tStats.m_fAttackSpeed : 1.f;

    m_fAttackTimer = 0.f;
    m_eState = STATE_MOVE;

    Compute_CurrentIndex();
    return S_OK;
}

// ======================= Update =======================

int CMinion::Update(_float fTimeDelta)
{
    if (m_bDead)
        return OBJ_POOL;

    if (FAILED(__super::Update(fTimeDelta)))
        return -1;

    m_fAttackTimer += fTimeDelta;
    Compute_CurrentIndex();

    if (m_tStats.m_fCurrentHp <= 0.f && m_eState != STATE_DIE)
    {
        m_tStats.m_fCurrentHp = 0.f;
        m_bDying = true;
        m_eState = STATE_DIE;
        On_Death();
    }
    //CPickingManager::GetInstance()->Out_PickingGroup(this);
    return 0;
}

void CMinion::Late_Update(_float fTimeDelta)
{
    __super::Late_Update(fTimeDelta);

    if (!Is_Frustum(Check_CullingRadius()))
        return;

    if (m_pRendererCom && Is_Frustum(Check_CullingRadius()))
        m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_NOALPHABLEND, this);

    //CPickingManager::GetInstance()->Add_PickingGroup(this);
}

// ======================= Render =======================

HRESULT CMinion::Render()
{
    if (!m_pShaderCom || !m_pModelCom)
        return E_FAIL;

    if (FAILED(SetUp_ShaderResources()))
        return E_FAIL;

    if (FAILED(SetUp_ShaderID()))
        return E_FAIL;

    _uint iNumMeshes = m_pModelCom->Get_NumMeshs();
    for (_uint i = 0; i < iNumMeshes; ++i)
    {
        if (FAILED(m_pModelCom->SetUp_Material(m_pShaderCom, "g_DiffuseTexture", i, aiTextureType_DIFFUSE)))
            return E_FAIL;

        if (FAILED(m_pModelCom->SetUp_Material(m_pShaderCom, "g_NormalTexture", i, aiTextureType_NORMALS)))
            return E_FAIL;

        if (FAILED(m_pModelCom->Render(m_pShaderCom, i, m_eShaderID)))
            return E_FAIL;
    }
    return S_OK;
}

HRESULT CMinion::Render_ShadowDepth()
{
    return __super::Render_ShadowDepth();
}

HRESULT CMinion::Render_EdgeDetection()
{
    return __super::Render_EdgeDetection();
}

// ======================= Navigation =======================

void CMinion::Compute_CurrentIndex(LEVEL eLevel)
{
    UNREFERENCED_PARAMETER(eLevel);
    Compute_CurrentIndex();
}

void CMinion::Compute_CurrentIndex()
{
    if (!m_pNavigationCom || !m_pTransformCom)
        return;

    _vector vPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    m_pNavigationCom->Compute_CurrentIndex_byXZ(vPos);
}

// ======================= Target / AI =======================

_float CMinion::Target_Distance(CActor* pTarget) const
{
    if (!pTarget || !m_pTransformCom)
        return FLT_MAX;

    _vector a = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    _vector b = pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

    a = XMVectorSetY(a, 0.f);
    b = XMVectorSetY(b, 0.f);

    return XMVectorGetX(XMVector3Length(b - a));
}

void CMinion::Find_Target()
{
    if (m_pTarget)
        return;

    CGameInstance* GI = CGameInstance::GetInstance();
    GI->AddRef();

    list<CGameObject*>* pMelee = nullptr;
    list<CGameObject*>* pRange = nullptr;

    if (m_eTeam == TEAM_BLUE)
    {
        pMelee = GI->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Minion_Red_Melee"));
        pRange = GI->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Minion_Red_Range"));
    }
    else
    {
        pMelee = GI->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Minion_Blue_Melee"));
        pRange = GI->Get_ObjectList(LEVEL_GAMEPLAY, TEXT("Layer_Minion_Blue_Range"));
    }

    CActor* best = nullptr;
    _float  bestDist = m_tStats.m_fAggroRadius;
    const _int mySlot = m_iLaneIndex;

    auto Scan = [&](list<CGameObject*>* list)
        {
            if (!list) return;

            for (auto* obj : *list)
            {
                CMinion* m = dynamic_cast<CMinion*>(obj);
                if (!m || m->IsDeadOrDying())
                    continue;

                if (m->Get_LaneIndex() != mySlot)
                    continue;

                float d = Target_Distance(m);
                if (d < bestDist)
                {
                    bestDist = d;
                    best = m;
                }
            }
        };

    Scan(pMelee);
    Scan(pRange);

    if (!best)
    {
        auto ScanAny = [&](list<CGameObject*>* list)
            {
                if (!list) return;

                for (auto* obj : *list)
                {
                    CMinion* m = dynamic_cast<CMinion*>(obj);
                    if (!m || m->IsDeadOrDying())
                        continue;

                    float d = Target_Distance(m);
                    if (d < bestDist)
                    {
                        bestDist = d;
                        best = m;
                    }
                }
            };
        ScanAny(pMelee);
        ScanAny(pRange);
    }

    m_pTarget = best;
    Safe_Release(GI);
}

CActor* CMinion::Find_MinDistance_Target()
{
    return m_pTarget;
}

CActor* CMinion::Check_FiledTarget()
{
    if (!m_pTarget)
        return nullptr;

    if (CMinion* pMin = dynamic_cast<CMinion*>(m_pTarget))
    {
        if (pMin->IsDeadOrDying())
        {
            m_pTarget = nullptr;
            return nullptr;
        }
    }
    else
    {
        if (m_pTarget->Get_Dead())
        {
            m_pTarget = nullptr;
            return nullptr;
        }
    }

    if (Target_Distance(m_pTarget) > m_tStats.m_fAggroRadius * 1.5f)
    {
        m_pTarget = nullptr;
        return nullptr;
    }

    return m_pTarget;
}

_bool CMinion::Is_TargetInAttackRange() const
{
    if (!m_pTarget)
        return false;

    float base = m_tStats.m_fAttackRange;
    float extra = fabsf(m_fLaneOffset) * 0.35f;
    return Target_Distance(m_pTarget) <= base + extra + 0.1f;
}

_bool CMinion::Is_TargetInAggroRange() const
{
    if (!m_pTarget)
        return false;

    return Target_Distance(m_pTarget) <= m_tStats.m_fAggroRadius;
}

void CMinion::Follow_Target(_float fTimeDelta)
{
    if (!m_pTarget || !m_pTransformCom || !m_bMove)
        return;

    _vector pos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    _vector tpos = m_pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

    _vector dir = XMVectorSetY(pos - tpos, 0.f);
    if (XMVectorGetX(XMVector3LengthSq(dir)) < 1e-6f)
        dir = XMVectorSet(0, 0, 1, 0);
    else
        dir = XMVector3Normalize(dir);

    float base = m_tStats.m_fAttackRange;
    float extra = fabsf(m_fLaneOffset) * 0.35f;
    float range = base + extra;

    _vector fromTarget = dir * range;

    if (fabsf(m_fLaneOffset) > 0.01f)
    {
        _vector right = XMVector3Normalize(XMVector3Cross(
            XMVectorSet(0, 1, 0, 0), dir));

        fromTarget += right * m_fLaneOffset;

        _vector flat = XMVectorSetY(fromTarget, 0.f);
        float lenSq = XMVectorGetX(XMVector3LengthSq(flat));

        if (lenSq > 1e-6f)
        {
            flat = XMVector3Normalize(flat) * range;
            fromTarget = XMVectorSetY(flat, 0.f);
        }
    }

    _vector desired = tpos + fromTarget;
    _vector moveDir = XMVectorSetY(desired - pos, 0.f);

    float distSq = XMVectorGetX(XMVector3LengthSq(moveDir));
    if (distSq < 1e-6f)
        return;

    float dist = sqrtf(distSq);
    float maxMove = m_tStats.m_fWalkSpeed * fTimeDelta;

    if (dist < 0.05f)
        return;

    if (maxMove > dist)
        maxMove = dist;

    moveDir = XMVector3Normalize(moveDir);
    pos += moveDir * maxMove;

    m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, pos);
    Check_Navigation();
}

// ======================= Combat / Damage =======================

_int CMinion::Take_Damage(int dmg, CActor* causer)
{
    if (m_eState == STATE_DIE)
        return 0;

    m_pOrginDamageCauser = causer;
    m_tStats.m_fCurrentHp -= dmg;

    if (m_tStats.m_fCurrentHp <= 0.f)
    {
        m_tStats.m_fCurrentHp = 0.f;
        m_bDying = true;

        if (causer)
            m_pTarget = causer;

        return 1;
    }

    if (causer)
        m_pTarget = causer;

    return 0;
}

void CMinion::ReUse_Setting(void* pArg)
{
    if (!pArg)
        return;

    MINION_SPAWNDESC* d = (MINION_SPAWNDESC*)pArg;

    m_eTeam = d->eTeam;
    m_eLane = d->eLane;
    m_eMinionType = d->eMinionType;
    m_iLaneIndex = d->iLaneIndex;
    m_iWaveIndex = d->iWaveIndex;
    m_vTargetPos = d->vTargetPos;

    Compute_LaneOffset();

    m_tStats.m_fCurrentHp = m_tStats.m_fMaxHp;
    m_eState = STATE_MOVE;
    m_bDead = false;
    m_pTarget = nullptr;
    m_pOrginDamageCauser = nullptr;
    m_bDying = false;
    m_fAttackTimer = 0.f;

    if (m_pTransformCom)
    {
        _vector p = XMLoadFloat3(&d->vSpawnPos);
        p = XMVectorSetW(p, 1.f);
        m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, p);
    }

    if (CMinionManager* mgr = CMinionManager::GetInstance())
    {
        const auto& path = mgr->GetLanePath(m_eLane);
        m_iLaneNodeIndex = mgr->GetLaneStartIndex(m_eTeam, path);
    }

    Compute_CurrentIndex();
}

_bool CMinion::Picking(_float3* PickingPoint)
{
    if (m_pModelCom->Picking(m_pTransformCom, PickingPoint) == true )
    {
        // 마우스가 적 위에 있음 → 공격 커서
        CCursorManager::GetInstance()->SetState(CURSOR_ATTACK);
        CChampion* pPlayer = CPlayerManager::GetInstance()->Get_ActivePlayer();
        pPlayer->Set_AttackTarget(this);
        return true;
    }
    else
    {
        ZeroMemory(&m_vMousePickedPos, sizeof(_float3));
    }
    return false;
}

void CMinion::PickingTrue()
{
}

void CMinion::Set_Picked()
{

}

void CMinion::Mark_Dead()
{
    if (m_bDead)
        return;

    CPoolManager* pool = CPoolManager::GetInstance();
    if (pool)
    {
        const _tchar* tag = nullptr;

        if (m_eTeam == TEAM_BLUE)
            tag = (m_eMinionType == MINION_MELEE) ? TEXT("Layer_Minion_Blue_Melee") : TEXT("Layer_Minion_Blue_Range");
        else
            tag = (m_eMinionType == MINION_MELEE) ? TEXT("Layer_Minion_Red_Melee") : TEXT("Layer_Minion_Red_Range");

        if (tag)
            pool->Add_Pooling_Object(LEVEL_GAMEPLAY, tag, this);
    }

    m_bDead = true;
    m_bDying = false;
}

// ======================= Lane Offset =======================

void CMinion::Compute_LaneOffset()
{
    const float width = 1.2f;

    switch (m_iLaneIndex)
    {
    case 0: m_fLaneOffset = 0.f; break;
    case 1: m_fLaneOffset = -width; break;
    case 2: m_fLaneOffset = +width; break;
    default: m_fLaneOffset = 0.f; break;
    }

    if (m_eTeam == TEAM_RED)
        m_fLaneOffset = -m_fLaneOffset;
}

// ======================= Shader =======================

HRESULT CMinion::SetUp_ShaderResources()
{
    if (!m_pShaderCom || !m_pTransformCom)
        return E_FAIL;

    _float4x4 w = m_pTransformCom->Get_World_TP();
    _float4x4 v = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_VIEW);
    _float4x4 p = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_PROJ);

    if (FAILED(m_pShaderCom->Set_RawValue("g_WorldMatrix", &w, sizeof(_float4x4)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Set_RawValue("g_ViewMatrix", &v, sizeof(_float4x4)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Set_RawValue("g_ProjMatrix", &p, sizeof(_float4x4)))) return E_FAIL;

    return S_OK;
}

HRESULT CMinion::SetUp_ShaderID()
{
    return S_OK;
}

_float CMinion::Check_CullingRadius()
{
    return 0.01f;
}


void CMinion::Free()
{
    __super::Free();
}
