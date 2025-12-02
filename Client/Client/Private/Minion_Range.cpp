#include "Minion_Range.h"
#include "Minion_IdleState.h"    // 
#include "Minion_DeadState.h"
#include "GameInstance.h"
#include "Model.h"
#include "Transform.h"
#include "Navigation.h"

USING(Client);


_bool CMinion_Range::Is_AnimationLoop(_uint eAnimId)
{
    switch ((ANIM)eAnimId)
    {
    case IDLE:
    case RUN:
        return true;
    }
    return false;
}

void CMinion_Range::Ready_Animation_Event()
{
    CAnimation* pAnim = m_pModelCom->Get_Animations()[ATTACK1];
    if (!pAnim) return;

    ANIMEVENT evt{};
    evt.fStartTime = 14.5f;
    evt.fEndTime = 15.f;
    evt.eType = ANIMEVENT::EVENT_PROJECTILE;
    pAnim->Add_Event(evt);

    pAnim = m_pModelCom->Get_Animations()[ATTACK2];
    evt.fStartTime = 13.5f;
    evt.fEndTime = 14.f;
    evt.eType = ANIMEVENT::EVENT_PROJECTILE;
    pAnim->Add_Event(evt);
}

void CMinion_Range::ReUse_Setting(void* pArg)
{
    __super::ReUse_Setting(pArg);

    if (m_pState)
    {
        m_pState->Exit();
        Safe_Delete(m_pState);
        m_pState = nullptr;
    }

    m_pState = new Minion::CIdleState(this);
    m_pState->Enter();

    m_eState = STATE_MOVE;

    if (m_pModelCom)
    {
        m_pModelCom->Set_CurrentAnimIndex(IDLE);
    }

    m_fAttackTimer = 0.f;
}

CMinion_Range::CMinion_Range(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CMinion(pDevice, pContext)
{
}

CMinion_Range::CMinion_Range(const CMinion_Range& rhs)
    : CMinion(rhs)
{
    m_pState = nullptr;
}

HRESULT CMinion_Range::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CMinion_Range::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    m_tStats.m_fMaxHp = 280.f;
    m_tStats.m_fCurrentHp = m_tStats.m_fMaxHp;
    m_tStats.m_fAttackPower = 24.f;
    m_tStats.m_fWalkSpeed = 2.8f;

    m_tStats.m_fAttackRange = 5.5f; 
    m_tStats.m_fAggroRadius = 7.5f;  

    m_tStats.m_fAttackSpeed = 0.83f;
    m_fAttackCoolTime = 1.f / m_tStats.m_fAttackSpeed;
    m_fAttackTimer = 0.f;

    if (m_tStats.m_fAttackSpeed > 0.f)
        m_fAttackCoolTime = 1.f / m_tStats.m_fAttackSpeed;
    else
        m_fAttackCoolTime = 1.f;

    m_fAttackTimer = 0.f;

    m_iIdleAnimIndex = CMinion_Range::ANIM::IDLE;
    m_iRunAnimIndex = CMinion_Range::ANIM::RUN;


    Safe_Delete(m_pState);
    m_pState = new Minion::CIdleState(this);
    m_pState->Enter();
    Set_Scale(_float3(0.007f, 0.007f, 0.007f));
    if (m_pModelCom)
        m_pModelCom->Set_CurrentAnimIndex(IDLE);

    return S_OK;
}

int CMinion_Range::Update(_float fTimeDelta)
{
    int iExit = __super::Update(fTimeDelta);

    if (iExit == OBJ_POOL || iExit == OBJ_DEAD)
        return iExit;

    if (m_pState)
    {
        if (CMinionState* pNew = m_pState->AI_Behaviour(fTimeDelta))
        {
            Safe_Delete(m_pState);
            m_pState = pNew;
            m_pState->Enter();
        }

        if (CMinionState* pNew = m_pState->Update(fTimeDelta))
        {
            Safe_Delete(m_pState);
            m_pState = pNew;
            m_pState->Enter();
        }
    }

    return 0;
}

void CMinion_Range::Late_Update(_float fTimeDelta)
{
    __super::Late_Update(fTimeDelta);

    if (m_eState == STATE_DIE)
        return;

    LateUpdate_State(fTimeDelta);
}

void CMinion_Range::AI_Behavior(_float fTimeDelta)
{
    CMinionState* pNewState = m_pState->AI_Behaviour(fTimeDelta);
    if (pNewState)
        m_pState = m_pState->ChangeState(m_pState, pNewState);
}

void CMinion_Range::Update_State(_float fTimeDelta)
{
    CMinionState* pNewState = m_pState->Update(fTimeDelta);
    if (pNewState)
        m_pState = m_pState->ChangeState(m_pState, pNewState);
}

void CMinion_Range::LateUpdate_State(_float fTimeDelta)
{
    CMinionState* pNewState = m_pState->Late_Update(fTimeDelta);
    if (pNewState)
        m_pState = m_pState->ChangeState(m_pState, pNewState);
}

// =========================
// Navigation
// =========================

void CMinion_Range::Check_Navigation()
{
    if (!m_pNavigationCom || !m_pTransformCom)
        return;

    _vector vPos = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
    _float  fHeight = m_pNavigationCom->Compute_Height(vPos, 0.f);

    vPos = XMVectorSetY(vPos, fHeight);
    m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vPos);
}

void CMinion_Range::On_Death()
{
    if (m_pState)
    {
        m_pState->Exit();
        Safe_Delete(m_pState);
        m_pState = nullptr;
    }
    m_pState = new Minion::CDeadState(this);
    m_pState->Enter();

}



_int CMinion_Range::Take_Damage(int fDamage, CActor* DamageCauser)
{
    _int iResult = __super::Take_Damage(fDamage, DamageCauser);

    if (m_eState == STATE_DIE && m_pModelCom)
    {
        m_pModelCom->Set_CurrentAnimIndex(DEATH1);
    }

    return iResult;
}

HRESULT CMinion_Range::SetUp_ShaderID()
{
    m_eShaderID = SHADER_ANIMDEFAULT;
    return S_OK;
}

HRESULT CMinion_Range::Ready_Components(void* pArg)
{
    /* For.Com_Transform */
    CTransform::TRANSFORMDESC TransformDesc;
    ZeroMemory(&TransformDesc, sizeof(CTransform::TRANSFORMDESC));
    TransformDesc.fSpeedPerSec = 3.f;
    TransformDesc.fRotationPerSec = XMConvertToRadians(90.0f);

    if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC, TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom, &TransformDesc)))
        return E_FAIL;
    if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRendererCom)))
        return E_FAIL;

    /* For.Com_Shader */
    if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxAnimModel"), (CComponent**)&m_pShaderCom)))
        return E_FAIL;
    if (m_eTeam == TEAM_BLUE)
    {
        /* For.Com_Model*/
        if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Minion_Range_Blue"), (CComponent**)&m_pModelCom)))
            return E_FAIL;
    }
    else
    {
        /* For.Com_Model*/
        if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Minion_Range_Red"), (CComponent**)&m_pModelCom)))
            return E_FAIL;
    }

    CColider::COLLIDERDESC ColliderDesc;
    ZeroMemory(&ColliderDesc, sizeof(CColider::COLLIDERDESC));
    ColliderDesc.vScale = _float3(2.0f, 2.0f, 2.0f);
    ColliderDesc.vRotation = _float3(0.f, 0.f, 0.f);
    ColliderDesc.vPosition = _float3(0.f, 60.f, 0.f);
    if (FAILED(__super::Add_Components(TEXT("Com_SPHERE"), LEVEL_STATIC, TEXT("Prototype_Component_Colider_Sphere"), (CComponent**)&m_pSphereCom, &ColliderDesc)))
        return E_FAIL;


    return S_OK;
}


CMinion_Range* CMinion_Range::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CMinion_Range* pInstance = new CMinion_Range(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CMinion_Melee");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CMinion_Range::Clone(void* pArg)
{
    CMinion_Range* pInstance = new CMinion_Range(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CMinion_Melee");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CMinion_Range::Free()
{
    __super::Free();

    Safe_Delete(m_pState);
}