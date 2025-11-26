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

void CMinion_Range::ReUse_Setting(void* pArg)
{
    // 1) 공통 리셋 (팀/라인/HP/위치/네비 인덱스 등)
    __super::ReUse_Setting(pArg);

    // 2) 이전 상태 정리 (DeadState 포함)
    if (m_pState)
    {
        m_pState->Exit();
        Safe_Delete(m_pState);
        m_pState = nullptr;
    }

    // 3) 다시 처음처럼 IdleState로 시작
    m_pState = new Minion::CIdleState(this);
    m_pState->Enter();

    // 4) 논리 상태도 살아있는 상태로
    m_eState = STATE_MOVE;     // 또는 STATE_IDLE, 너 스타일대로

    // 5) 애니메이션도 초기 상태로 (Idle 또는 Run)
    if (m_pModelCom)
    {
        // Idle에서 걷기로 넘어가는 구조면 Idle로
        m_pModelCom->Set_CurrentAnimIndex(IDLE);
    }

    // 공격 관련도 한 번 더 클리어하고 싶으면
    m_fAttackTimer = 0.f;
}

CMinion_Range::CMinion_Range(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CMinion(pDevice, pContext)
{
}

CMinion_Range::CMinion_Range(const CMinion_Range& rhs)
    : CMinion(rhs)
{
    // 상태는 복사 시 새로 생성하거나, nullptr로 두고 Initialize에서 다시 세팅해도 됨
    m_pState = nullptr;
}

HRESULT CMinion_Range::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CMinion_Range::Initialize(void* pArg)
{
    // CMinion::Initialize 에서 스탯 / 네비 / 컴포넌트 기본 세팅
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    m_tStats.m_fMaxHp = 280.f;
    m_tStats.m_fCurrentHp = m_tStats.m_fMaxHp;
    m_tStats.m_fAttackPower = 24.f;
    m_tStats.m_fWalkSpeed = 2.8f;

    m_tStats.m_fAttackRange = 5.5f;   // ★ 근거리보다 훨씬 큼
    m_tStats.m_fAggroRadius = 7.5f;   // 시야는 좀 더 넓게

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


    // 초기 상태 : Idle → 바로 Move로 넘기는 구조
    Safe_Delete(m_pState);
    m_pState = new Minion::CIdleState(this);
    m_pState->Enter();
    Set_Scale(_float3(0.007f, 0.007f, 0.007f));
    // ★ 초기에 Idle 애니메이션으로 설정
    if (m_pModelCom)
        m_pModelCom->Set_CurrentAnimIndex(IDLE);

    return S_OK;
}

int CMinion_Range::Update(_float fTimeDelta)
{
    // 1) 먼저 CMinion::Update 호출 + 리턴값 확인
    int iExit = __super::Update(fTimeDelta);

    // 2) 풀로 보내거나(OBJ_POOL) 완전 삭제(OBJ_DEAD)해야 하면
    //    더 이상 상태 머신 업데이트 하지 말고 그대로 리턴
    if (iExit == OBJ_POOL || iExit == OBJ_DEAD)
        return iExit;

    // 3) 여기서부터는 살아있는 동안의 상태 머신 처리
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
    // ★ 기존 상태 정리
    if (m_pState)
    {
        m_pState->Exit();
        Safe_Delete(m_pState);
        m_pState = nullptr;
    }

    // ★ DeadState 진입
    m_pState = new Minion::CDeadState(this);
    m_pState->Enter();

    // 여기서는 m_bDead 를 건드리지 않는다.
    // 죽는 애니 끝날 때 DeadState에서 Mark_Dead() 호출.
}

_int CMinion_Range::Take_Damage(int fDamage, CActor* DamageCauser)
{
    // 기본 체력/어그로 처리는 CMinion 에서
    _int iResult = __super::Take_Damage(fDamage, DamageCauser);

    // 죽었으면 죽는 애니 지정
    if (m_eState == STATE_DIE && m_pModelCom)
    {
        // 여러 DEATH 애니가 있으면 랜덤으로 골라도 됨
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




    return S_OK;
}

// =========================
// Create / Clone / Free
// =========================

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