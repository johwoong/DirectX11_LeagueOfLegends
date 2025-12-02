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

    CTransform* pTr = m_pOwner->Get_Transform();

    // 공통: 항상 같은 방향으로 바라보게 유지
    _vector vLook = XMVector3Normalize(XMLoadFloat3(&m_vDir));
    _vector vUp = XMVectorSet(0, 1, 0, 0);
    _vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vLook));
    vUp = XMVector3Cross(vLook, vRight);

    pTr->Set_State(CTransform::STATE_RIGHT, vRight);
    pTr->Set_State(CTransform::STATE_UP, vUp);
    pTr->Set_State(CTransform::STATE_LOOK, vLook);

    // ===== 1) 방향 E (E_-180, E_-90, E90, E180) =====
    if (curAnim == CEzreal::ANIM::E_MINUS_180 ||
        curAnim == CEzreal::ANIM::E_MINUS_90 ||
        curAnim == CEzreal::ANIM::E90 ||
        curAnim == CEzreal::ANIM::E180)
    {
        bool finished = model->Play_Animation(fDeltaTime * 1.4F, false, nullptr, 0.2f);

        // 방향 E 끝날 때 텔레포트 + EXIT로 전환
        if (!m_bTeleported && finished)
        {
            m_bTeleported = true;
            pTr->Set_State(CTransform::STATE_TRANSLATION, m_vDashTarget);
            // 필요하면 네비 인덱스 재계산
            m_pOwner->Get_Navigation()->Compute_CurrentIndex_byXZ(m_vDashTarget);
        }

        if (finished)
        {
            model->Set_CurrentAnimIndex(CEzreal::ANIM::E_EXIT);
        }

        return nullptr;
    }

    // ===== 2) EXIT =====
    if (curAnim == CEzreal::ANIM::E_EXIT)
    {
        bool exitFinished =
            model->Play_Animation(fDeltaTime * 1.3f, false, nullptr, 0.0f);

        // EXIT 끝 → IDLE
        if (exitFinished)
            return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);

        return nullptr;
    }

    // 혹시 다른 E 관련 애니로 들어가 있으면 그냥 EXIT로 넘겨버림
    model->Set_CurrentAnimIndex(CEzreal::ANIM::E_EXIT);
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
    m_bTeleported = false;

    CCursorManager* CM = CCursorManager::GetInstance();

    _vector rayPos = CM->Get_RayPos();
    _vector rayDir = CM->Get_RayDir();

    CTransform* pTr = m_pOwner->Get_Transform();
    _vector myPos = pTr->Get_State(CTransform::STATE_TRANSLATION);
    float y = XMVectorGetY(myPos);

    // ===== 1) 바닥 평면과의 교점 구하기 =====
    float rayDirY = XMVectorGetY(rayDir);
    if (fabsf(rayDirY) < 0.0001f)
        rayDirY = (rayDirY >= 0.f ? 0.0001f : -0.0001f);

    float t = (y - XMVectorGetY(rayPos)) / rayDirY;
    _vector hit = rayPos + rayDir * t;

    // ===== 2) 대쉬 방향 (XZ) =====
    _vector dir = XMVector3Normalize(
        XMVectorSetY(hit - myPos, 0.f)
    );
    XMStoreFloat3(&m_vDir, dir);

    // ===== 3) 도착 위치 (사거리 클램프) =====
    const float maxRange = 200.f;
    float dist = XMVectorGetX(XMVector3Length(hit - myPos));

    if (dist > maxRange)
        m_vDashTarget = myPos + dir * maxRange;
    else
        m_vDashTarget = hit;

    // ===== 4) 현재 바라보는 방향과의 각도로 애니 선택 =====
    _vector look = XMVector3Normalize(
        XMVectorSetY(pTr->Get_State(CTransform::STATE_LOOK), 0.f)
    );

    float dot = XMVectorGetX(XMVector3Dot(look, dir));
    dot = std::clamp(dot, -1.f, 1.f);

    float angle = XMConvertToDegrees(acosf(dot)); // 0~180
    float crossY = XMVectorGetY(XMVector3Cross(look, dir));
    if (crossY < 0) angle = -angle;              // -180 ~ 180

    int anim = CEzreal::ANIM::E90;

    if (angle < -135.f)
        anim = CEzreal::ANIM::E_MINUS_180;  // 거의 정반대 (뒤)
    else if (angle < -45.f)
        anim = CEzreal::ANIM::E_MINUS_90;   // 왼쪽
    else if (angle < 45.f)
        anim = CEzreal::ANIM::E180;         // 정면
    else if (angle < 135.f)
        anim = CEzreal::ANIM::E90;          // 오른쪽
    else
        anim = CEzreal::ANIM::E_MINUS_180;  // 거의 정반대 (뒤)

    m_pOwner->Get_Model()->Set_CurrentAnimIndex(anim);

    // ===== 5) 현재 프레임부터 바로 대쉬 방향으로 바라보게 회전 =====
    _vector vLook = dir;
    _vector vUp = XMVectorSet(0, 1, 0, 0);
    _vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vLook));
    vUp = XMVector3Cross(vLook, vRight);

    pTr->Set_State(CTransform::STATE_RIGHT, vRight);
    pTr->Set_State(CTransform::STATE_UP, vUp);
    pTr->Set_State(CTransform::STATE_LOOK, vLook);
}


void CEzrealSkillStateE::Exit()
{
    m_pOwner->SetUsingSkill(false);
    __super::Exit();
}