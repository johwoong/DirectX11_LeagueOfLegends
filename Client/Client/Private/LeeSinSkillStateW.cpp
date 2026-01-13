#include "LeeSinSkillStateW.h"
#include "LeeSin.h"

#include "PickingManager.h"
#include "PlayerIdleState.h"
#include "Minion.h"

using namespace Player;

CLeeSinSkillStateW::CLeeSinSkillStateW(CPlayer* pPlayer, CActor* pTarget)
{
    m_pPlayer = pPlayer;
    m_pOwner = dynamic_cast<CLeeSin*>(pPlayer);
    m_pTarget = pTarget;
    m_eState = STATE_SKILL_ATTACK_W;
}

void CLeeSinSkillStateW::Enter()
{
    __super::Enter();

    if (!m_pOwner)
    {
        m_bFinished = true;
        return;
    }

    // ? W2인지 먼저 체크
    m_bRecast = m_pOwner->Can_WRecast();

    if (m_bRecast)
    {
        // ? W2: 타겟 무시(대시 없음)
        m_bSelfCast = true;
        m_pTarget = nullptr;

        // ? 핵심: W2는 Enter에서 "즉시" 실드(HP/타이머/FX) 제거
        // (너가 수정한 Consume_WRecast(true) 안에서 실드 변수/FX 다 지우게 되어있어야 함)
        m_pOwner->Consume_WRecast(true);

        // ? W2 강화 버프
        m_pOwner->Apply_WEmpower(2, 3.f);

        // Update에서 재처리 안 하도록
        m_bApplied = true;
    }
    else
    {
        // ===== 기존: 타겟 없으면 Self Cast =====
        if (m_pTarget == nullptr)
        {
            m_bSelfCast = true;
        }
        else
        {
            if (!IsValidAllyMinion(m_pTarget))
            {
                m_bFinished = true;
                return;
            }
        }

        m_bApplied = false; // W1은 Update에서 1회 적용
    }

    m_pOwner->SetUsingSkill(true);
    m_pOwner->Get_Model()->Set_CurrentAnimIndex(CLeeSin::ANIM::W);
    m_fSelfAcc = 0.f;
}

void CLeeSinSkillStateW::Exit()
{
    if (m_pOwner)
        m_pOwner->SetUsingSkill(false);

    __super::Exit();
}

bool CLeeSinSkillStateW::IsValidAllyMinion(CActor* p) const
{
    if (!p) return false;
    if (p->Get_Dead()) return false;

    CMinion* pMinion = dynamic_cast<CMinion*>(p);
    if (!pMinion) return false;

    return true;
}

CPlayerState* CLeeSinSkillStateW::Update(_float dt)
{
    if (!m_pPlayer)
        return nullptr;

    if (!m_pOwner)
        return new CIdleState(m_pPlayer, CIdleState::IDLE_MAIN);

    if (m_bFinished)
        return new CIdleState(m_pPlayer, CIdleState::IDLE_MAIN);

    // 애니 재생
    CModel* pModel = m_pOwner->Get_Model();
    pModel->Play_Animation(
        dt,
        m_pOwner->Is_AnimationLoop(pModel->Get_CurrentAnimIndex()),
        nullptr,
        0.1f);

    // ? W1만 여기서 적용 (W2는 Enter에서 이미 처리 끝)
    if (!m_bApplied && !m_bRecast)
    {
        m_bApplied = true;
        m_pOwner->Cast_W_Self(80.f, 2.f, 3.f, TEXT("LeeSin_W.dat"));
    }

    // ===== Self Cast면: 대시 없음, 잠깐 후 종료 =====
    if (m_bSelfCast)
    {
        m_fSelfAcc += dt;

        if (m_fSelfAcc >= 0.15f)
        {
            m_bFinished = true;
            m_pOwner->SetUsingSkill(false);
            return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
        }

        return nullptr;
    }

    // ===== 타겟 캐스트면: 기존 대시 =====
    DashToTarget(dt);

    if (m_bFinished)
    {
        m_pOwner->SetUsingSkill(false);
        return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
    }

    return nullptr;
}

void CLeeSinSkillStateW::DashToTarget(_float dt)
{
    if (!m_pTarget || m_pTarget->Get_Dead())
    {
        m_bFinished = true;
        return;
    }

    CTransform* pTr = m_pOwner->Get_Transform();

    _vector vPos = pTr->Get_State(CTransform::STATE_TRANSLATION);
    _vector vTar = m_pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

    _vector to = vTar - vPos;
    to = XMVectorSetY(to, 0.f);

    float dist = XMVectorGetX(XMVector3Length(to));
    if (dist <= m_fStopDist || dist < 0.0001f)
    {
        m_pOwner->Check_Navigation();
        m_bFinished = true;
        return;
    }

    _vector dir = XMVector3Normalize(to);
    dir = XMVectorSetW(dir, 0.f);

    // 스케일 보존
    _vector curR = pTr->Get_State(CTransform::STATE_RIGHT);
    _vector curU = pTr->Get_State(CTransform::STATE_UP);
    _vector curL = pTr->Get_State(CTransform::STATE_LOOK);

    float sx = XMVectorGetX(XMVector3Length(curR));
    float sy = XMVectorGetX(XMVector3Length(curU));
    float sz = XMVectorGetX(XMVector3Length(curL));
    if (sx < 0.0001f) sx = 1.f;
    if (sy < 0.0001f) sy = 1.f;
    if (sz < 0.0001f) sz = 1.f;

    const _vector vUpWorld = XMVectorSet(0.f, 1.f, 0.f, 0.f);

    _vector vLookN = -dir;
    vLookN = XMVectorSetW(vLookN, 0.f);

    _vector vRightN = XMVector3Normalize(XMVector3Cross(vUpWorld, vLookN));
    vRightN = XMVectorSetW(vRightN, 0.f);

    _vector vUpN = XMVector3Cross(vLookN, vRightN);
    vUpN = XMVectorSetW(vUpN, 0.f);

    pTr->Set_State(CTransform::STATE_RIGHT, vRightN * sx);
    pTr->Set_State(CTransform::STATE_UP, vUpN * sy);
    pTr->Set_State(CTransform::STATE_LOOK, vLookN * sz);

    float move = m_fDashSpeed * dt;
    if (move > dist) move = dist;

    _vector vNext = vPos + dir * move;
    vNext = XMVectorSetW(vNext, 1.f);
    pTr->Set_State(CTransform::STATE_TRANSLATION, vNext);
}