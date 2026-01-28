#include "TemooSkillStateW.h"

#include "GameInstance.h"
#include "PoolManager.h"
#include "Champion.h"
#include "PlayerIdleState.h"
#include "PlayerRunState.h"
#include "Effect.h" // 네 이펙트 dat 스폰 시스템용

USING(Client)
USING(Player)

CTemooSkillStateW::CTemooSkillStateW(CPlayer* pPlayer)
{
    m_pOwner = pPlayer;
    m_eState = STATE_SKILL_ATTACK_W;
    m_ePlayerID = (m_pOwner) ? m_pOwner->Get_PlayerID() : 0;

    // 튜닝값
    m_fDuration = 3.0f;
    m_fSpeedMul = 1.35f;
}

void CTemooSkillStateW::Enter()
{
    __super::Enter();

    m_bApplied = false;
    Apply_WBuff_And_SpawnFX();
}

void CTemooSkillStateW::Exit()
{
    __super::Exit();
}

CPlayerState* CTemooSkillStateW::HandleInput()
{
    // W는 즉발 버프라 입력 처리할 게 없음
    return nullptr;
}

CPlayerState* CTemooSkillStateW::Update(_float)
{
    // ★ 애니/이동 처리 안 함. (원하는 설계)
    //    버프 걸고 바로 Run/Idle로 복귀

    if (!m_pOwner)
        return nullptr;

    _vector cur = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

    _float3 target3 = m_pOwner->Get_TargetPosition();
    _vector tar = XMLoadFloat3(&target3);

    const float dist = GetDistanceXZ(cur, tar);

    // 이동 중이면 Run 유지, 아니면 Idle
    if (dist > 0.05f)
        return new CRunState(m_pOwner);

    return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
}

CPlayerState* CTemooSkillStateW::Late_Update(_float)
{
    return nullptr;
}

void CTemooSkillStateW::Apply_WBuff_And_SpawnFX()
{
    if (m_bApplied || !m_pOwner)
        return;

    m_bApplied = true;

    if (CChampion* pChamp = dynamic_cast<CChampion*>(m_pOwner))
    {
        pChamp->Apply_MoveSpeedBuff(m_fSpeedMul, m_fDuration, TEXT("Alphen_Boost_Wind_1.dat"));
        CGameInstance::GetInstance()->PlaySounds(const_cast<TCHAR*>(L"06_teemo_movequick_OBA_1.wav"), SOUND_PLAYER, 0.4f);
    }
}

void CTemooSkillStateW::Spawn_TemooW_Effect()
{

    struct EFFECT_SPAWN_DESC
    {
        const _tchar* pDatFile = nullptr;
        CGameObject* pFollow = nullptr;
        _float        fDeadTime = 0.f;
        _bool         bFollow = true;
        _float3       vOffset = { 0.f, 0.8f, 0.f };
    };

    EFFECT_SPAWN_DESC desc{};
    desc.pDatFile = TEXT("Temoo_W.dat");   // ★ 여기! (Temoo_Blind 아님)
    desc.pFollow = m_pOwner;             // ★ 티모에 붙임
    desc.fDeadTime = m_fDuration;          // ★ 버프 지속시간과 같이 꺼짐
    desc.bFollow = true;
    desc.vOffset = { 0.f, 0.f, 0.f };

    CPoolManager* PM = CPoolManager::GetInstance();
    CGameInstance* GI = CGameInstance::GetInstance();
    // ★ 너 프로젝트에서 Effect가 실제로 생성되는 태그랑 반드시 일치해야 함
    const _tchar* LAYER = TEXT("Layer_Effect");
    const _tchar* PROTO = TEXT("Prototype_GameObject_Effect");

    if (!PM->Reuse_Pooling_Object(LEVEL_GAMEPLAY, LAYER, &desc))
    {
        GI->Add_GameObject(PROTO, LEVEL_GAMEPLAY, LAYER, &desc);
    }
}
