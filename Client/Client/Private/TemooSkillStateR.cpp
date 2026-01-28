#include "TemooSkillStateR.h"

#include <cmath>

#include "GameInstance.h"
#include "PlayerIdleState.h"
#include "CursorManager.h"
#include "MushRoom.h"

USING(Client)
USING(Player)

// Teemo 모델이 "전방 = -LOOK" 이면 -1, "전방 = +LOOK" 이면 +1
static constexpr float kPlayerLookSign = -1.f;

static void Rotate_LookOnly_PreserveScale(CTransform* tr, _vector desiredLookAxis)
{
    if (!tr) return;

    desiredLookAxis = XMVectorSetY(desiredLookAxis, 0.f);
    if (XMVectorGetX(XMVector3LengthSq(desiredLookAxis)) < 1e-6f)
        return;

    desiredLookAxis = XMVector3Normalize(desiredLookAxis);

    // 기존 축 길이(스케일) 보존
    float sx = XMVectorGetX(XMVector3Length(tr->Get_State(CTransform::STATE_RIGHT)));
    float sy = XMVectorGetX(XMVector3Length(tr->Get_State(CTransform::STATE_UP)));
    float sz = XMVectorGetX(XMVector3Length(tr->Get_State(CTransform::STATE_LOOK)));

    _vector upW = XMVectorSet(0.f, 1.f, 0.f, 0.f);

    _vector right = XMVector3Cross(upW, desiredLookAxis);
    if (XMVectorGetX(XMVector3LengthSq(right)) < 1e-6f)
        right = XMVectorSet(1.f, 0.f, 0.f, 0.f);
    else
        right = XMVector3Normalize(right);

    _vector up = XMVector3Normalize(XMVector3Cross(desiredLookAxis, right));

    tr->Set_State(CTransform::STATE_RIGHT, right * sx);
    tr->Set_State(CTransform::STATE_UP, up * sy);
    tr->Set_State(CTransform::STATE_LOOK, desiredLookAxis * sz);
}

CTemooSkillStateR::CTemooSkillStateR(CPlayer* pPlayer)
{
    m_pOwner = pPlayer;
    m_eState = STATE_SKILL_ATTACK_R;
}

void CTemooSkillStateR::Enter()
{
    __super::Enter();
    m_pOwner->SetUsingSkill(true);

    m_bSpawned = false;

    // ? 커서 위치 + 방향 + (플레이어 회전) + 설치위치(커서) 확정
    ComputeDirFaceAndAnim();

    auto* model = m_pOwner->Get_Model();
    model->Set_CurrentAnimIndex((_uint)m_eAnim);
    model->Set_TimeReset();
}

void CTemooSkillStateR::Exit()
{
    m_pOwner->SetUsingSkill(false);
    __super::Exit();
}

CPlayerState* CTemooSkillStateR::HandleInput()
{
    return nullptr;
}

CPlayerState* CTemooSkillStateR::Update(_float dt)
{
    auto* model = m_pOwner->Get_Model();
    const bool animEnd = model->Play_Animation(dt, false, nullptr, 0.1f);

    if (!m_bSpawned)
    {
        for (auto& e : model->Get_Events())
        {
            if (e.isPlay && e.eType == ANIMEVENT::EVENT_PROJECTILE)
            {
                Spawn_MushRoom();
                break;
            }
        }
    }

    if (animEnd)
        return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);

    return nullptr;
}

CPlayerState* CTemooSkillStateR::Late_Update(_float)
{
    return nullptr;
}

bool CTemooSkillStateR::Get_MouseGroundPickPos(_float3& outPos)
{
    CCursorManager* cm = CCursorManager::GetInstance();
    cm->Update_Ray();

    _vector rayPos = cm->Get_RayPos();
    _vector rayDir = cm->Get_RayDir();

    // 지금 너가 쓰는 방식 유지(플레이어 y 기준 평면)
    _vector myPos = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
    float y = XMVectorGetY(myPos);

    float rayDirY = XMVectorGetY(rayDir);
    if (fabsf(rayDirY) < 1e-6f)
        rayDirY = (rayDirY >= 0.f ? 1e-6f : -1e-6f);

    float t = (y - XMVectorGetY(rayPos)) / rayDirY;
    _vector hit = rayPos + rayDir * t;

    XMStoreFloat3(&outPos, hit);
    return true;
}

void CTemooSkillStateR::ComputeDirFaceAndAnim()
{
    _float3 pick{};
    if (!Get_MouseGroundPickPos(pick))
        return;

    CTransform* tr = m_pOwner->Get_Transform();
    _vector myPos = tr->Get_State(CTransform::STATE_TRANSLATION);

    _vector hit = XMLoadFloat3(&pick);
    hit = XMVectorSetW(hit, 1.f);

    // dir : player -> cursor (XZ)
    _vector toHit = XMVectorSetY(hit - myPos, 0.f);
    float len = XMVectorGetX(XMVector3Length(toHit));

    _vector dir = XMVectorZero();
    if (len < 1e-6f)
        dir = XMVectorSet(0.f, 0.f, 1.f, 0.f);
    else
        dir = toHit / len;

    XMStoreFloat3(&m_vCastDir, dir);

    // 플레이어 회전
    _vector desiredLookAxis = dir * kPlayerLookSign;
    Rotate_LookOnly_PreserveScale(tr, desiredLookAxis);

    m_eAnim = CTemoo::R_0;

    // ? 사거리 클램프
    static constexpr float kMushCastRange = 6.0f; // 너 원하는 R 사거리로
    _vector place = hit;
    if (len > kMushCastRange)
        place = myPos + dir * kMushCastRange;

    // y는 플레이어 y로 고정
    place = XMVectorSetY(place, XMVectorGetY(myPos));
    place = XMVectorSetW(place, 1.f);


    m_vPlacePos = place;
}

void CTemooSkillStateR::Spawn_MushRoom()
{
    m_bSpawned = true;

    CMushRoom::MUSHROOM_DESC desc{};
    XMStoreFloat3(&desc.vPos, m_vPlacePos);
    desc.vDir = m_vCastDir;
    desc.pOwner = dynamic_cast<CActor*>(m_pOwner);
    
    CGameInstance::GetInstance()->PlaySounds(const_cast<TCHAR*>(L"TemooRSound.wav"), SOUND_PLAYERTALK, 0.4f);
    CGameObject* pObj = nullptr;
    if (FAILED(CGameInstance::GetInstance()->Add_GameObject_Out(
        TEXT("Prototype_GameObject_TemooMushroom"),
        LEVEL_GAMEPLAY,
        TEXT("Layer_MushRoom"),
        pObj,
        &desc)))
    {
        m_bSpawned = false;
    }
}
