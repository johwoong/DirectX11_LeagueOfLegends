#include "EzrealSkillStateE.h"
#include "Ezreal.h"
#include "GameInstance.h"
#include "PlayerIdleState.h"
#include "PlayerRunState.h"
#include "CursorManager.h"

using namespace Player;

CEzrealSkillStateE::CEzrealSkillStateE(CPlayer* pPlayer)
{
    m_pOwner = pPlayer;
    m_eState = STATE_SKILL_ATTACK_E;
}


CPlayerState* CEzrealSkillStateE::HandleInput()
{
    return nullptr;
}

CPlayerState* CEzrealSkillStateE::Update(_float fDeltaTime)
{
    CModel* model = m_pOwner->Get_Model();
    int curAnim = model->Get_CurrentAnimIndex();

    // ===== 1) 방향 애니메이션 재생 =====
    bool finished =
        model->Play_Animation(fDeltaTime, false, nullptr, 0.0f);

    if (curAnim == CEzreal::ANIM::E_MINUS_180 ||
        curAnim == CEzreal::ANIM::E_MINUS_90 ||
        curAnim == CEzreal::ANIM::E90 ||
        curAnim == CEzreal::ANIM::E180)
    {
        // 방향을 항상 유지 (중간에 돌아가는 문제 방지)
        m_pOwner->Get_Transform()->Set_State(
            CTransform::STATE_LOOK,
            XMVector3Normalize(XMLoadFloat3(&m_vDir))
        );

        if (finished)
        {
            model->Set_CurrentAnimIndex(CEzreal::ANIM::E_EXIT);
            return nullptr;
        }
        return nullptr;
    }

    // ===== 2) EXIT 단계 =====
    if (curAnim == CEzreal::ANIM::E_EXIT)
    {
        bool exitFinished =
            model->Play_Animation(fDeltaTime * 1.7f, false, nullptr, 0.0f);

        // Look 고정
        m_pOwner->Get_Transform()->Set_State(
            CTransform::STATE_LOOK,
            XMVector3Normalize(XMLoadFloat3(&m_vDir))
        );

        // 텔레포트는 한 번만
        if (!m_bTeleported)
        {
            m_bTeleported = true;
            m_pOwner->Get_Transform()->Set_State(
                CTransform::STATE_TRANSLATION,
                m_vDashTarget
            );

            // === 이동 리셋 ===
            m_pOwner->Set_TargetPosition(
                *(_float3*)&m_vDashTarget
            );
        }

        // EXIT 끝 → IDLE
        if (exitFinished)
        {
            return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
        }
    }

    return nullptr;
}

CPlayerState* CEzrealSkillStateE::Late_Update(_float fDeltaTime)
{
    return nullptr;
}

void CEzrealSkillStateE::Enter()
{
    __super::Enter();
    m_pOwner->SetUsingSkill(true);


    CCursorManager* CM = CCursorManager::GetInstance();

    _vector rayPos = CM->Get_RayPos();
    _vector rayDir = CM->Get_RayDir();

    _vector myPos = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
    float y = XMVectorGetY(myPos);

    float t = (y - XMVectorGetY(rayPos)) / XMVectorGetY(rayDir);
    _vector hit = rayPos + rayDir * t;

    _vector dir = XMVector3Normalize(
        XMVectorSetY(hit - myPos, 0.f)
    );
    XMStoreFloat3(&m_vDir, dir);



    float maxRange = 200.f;
    float dist = XMVectorGetX(XMVector3Length(hit - myPos));

    if (dist > maxRange)
        m_vDashTarget = myPos + dir * maxRange;
    else
        m_vDashTarget = hit;


    _vector look = XMVector3Normalize(
        XMVectorSetY(m_pOwner->Get_Transform()->Get_State(CTransform::STATE_LOOK), 0.f)
    );

    float dot = XMVectorGetX(XMVector3Dot(look, dir));
    dot = std::clamp(dot, -1.f, 1.f);

    float angle = XMConvertToDegrees(acosf(dot));

    float crossY = XMVectorGetY(XMVector3Cross(look, dir));
    if (crossY < 0) angle = -angle;

    int anim = CEzreal::ANIM::E90;

    if (angle < -135.f)
        anim = CEzreal::ANIM::E_MINUS_180;
    else if (angle < 0)
        anim = CEzreal::ANIM::E_MINUS_90;
    else if (angle < 135.f)
        anim = CEzreal::ANIM::E90;
    else
        anim = CEzreal::ANIM::E180;

    m_pOwner->Get_Model()->Set_CurrentAnimIndex(anim);


    _vector vLook = dir;
    _vector vUp = XMVectorSet(0, 1, 0, 0);
    _vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vLook));
    vUp = XMVector3Cross(vLook, vRight);

    m_pOwner->Get_Transform()->Set_State(CTransform::STATE_RIGHT, vRight);
    m_pOwner->Get_Transform()->Set_State(CTransform::STATE_UP, vUp);
    m_pOwner->Get_Transform()->Set_State(CTransform::STATE_LOOK, vLook);
}


void CEzrealSkillStateE::Exit()
{
    m_pOwner->SetUsingSkill(false);
    __super::Exit();
}