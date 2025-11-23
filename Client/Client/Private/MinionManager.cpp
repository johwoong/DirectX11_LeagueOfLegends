#include "MinionManager.h"
#include "PoolManager.h"
#include "GameInstance.h"

USING(Client);

IMPLEMENT_SINGLETON(CMinionManager)

CMinionManager::CMinionManager()
{
}

// 레벨 지정
HRESULT CMinionManager::Initialize(LEVEL eOwnerLevel)
{
    m_eLevel = eOwnerLevel;
    m_fWaveTimer = 0.f;
    m_iWaveCount = 0;

    // 1) 스폰 위치 로드
    if (FAILED(LoadSpawnPositions()))
        return E_FAIL;

    // 2) 라인별 웨이포인트 로드
    //    파일 이름은 네가 만든대로 맞춰줘야 함
    LoadLanePoints(TEXT("../../../Bin/Data/PositionData/MinionTopDestnation.dat"), m_TopLanePoints);
    LoadLanePoints(TEXT("../../../Bin/Data/PositionData/MinionMidDestnation.dat"), m_MidLanePoints);
    LoadLanePoints(TEXT("../../../Bin/Data/PositionData/MinionBottomDestnation.dat"), m_BotLanePoints);

    return S_OK;
}

void CMinionManager::Update(_float fTimeDelta)
{
    if (m_eLevel == LEVEL_END)
        return;

    CGameInstance* pGI = CGameInstance::GetInstance();
    pGI->AddRef();

    // 디버그: 1번 키로 바로 다음 웨이브 시작
    if (pGI->Key_Down(DIK_1))
    {
        ForceSpawnWave();
    }

    // 2번 키 : 레드 미니언 전부 즉사
    if (pGI->Key_Down(DIK_2))
    {
        if (CMinionManager* pMgr = CMinionManager::GetInstance())
            pMgr->Debug_KillTeamMinions(TEAM_RED);
    }

    // 3번 키 : 블루 미니언 전부 즉사
    if (pGI->Key_Down(DIK_3))
    {
        if (CMinionManager* pMgr = CMinionManager::GetInstance())
            pMgr->Debug_KillTeamMinions(TEAM_BLUE);
    }

    pGI->Release();

    // ====== 1) 현재 웨이브에서 아직 안 나온 미니언 처리 ======
    if (m_bWaveInProgress)
    {
        m_fWaveElapsed += fTimeDelta;

        _bool bAllSpawned = true;

        for (auto& ps : m_PendingSpawns)
        {
            if (!ps.bSpawned && m_fWaveElapsed >= ps.fDelay)
            {
                SpawnMinion(ps.desc);
                ps.bSpawned = true;
            }

            if (!ps.bSpawned)
                bAllSpawned = false;
        }

        // 이번 웨이브의 미니언을 전부 소환 완료
        if (bAllSpawned)
        {
            m_bWaveInProgress = false;
            m_PendingSpawns.clear();
            m_fWaveElapsed = 0.f;
        }
    }
    else
    {
        m_fWaveTimer += fTimeDelta;
        if (m_fWaveTimer >= m_fWaveInterval)
        {
            m_fWaveTimer = 0.f;
            SpawnWave();        // 이제는 "웨이브 세팅만" 하고, 실제 소환은 위에서 순차적으로
        }
    }
}

void CMinionManager::SpawnSingle(const MINION_SPAWNDESC& desc)
{
    SpawnMinion(desc);
}

void CMinionManager::ForceSpawnWave()
{
    // 이전 웨이브 진행 중이어도 그냥 초기화하고 새 웨이브 시작
    m_fWaveTimer = 0.f;

    m_bWaveInProgress = false;
    m_PendingSpawns.clear();
    m_fWaveElapsed = 0.f;

    SpawnWave();
}

void CMinionManager::Debug_KillTeamMinions(TEAMTYPE eTeam)
{
    CGameInstance* pGI = CGameInstance::GetInstance();
    Safe_AddRef(pGI);

    const _tchar* szMeleeLayer = nullptr;
    const _tchar* szRangeLayer = nullptr;

    if (eTeam == TEAM_RED)
    {
        szMeleeLayer = TEXT("Layer_Minion_Red_Melee");
        szRangeLayer = TEXT("Layer_Minion_Red_Range");
    }
    else // TEAM_BLUE
    {
        szMeleeLayer = TEXT("Layer_Minion_Blue_Melee");
        szRangeLayer = TEXT("Layer_Minion_Blue_Range");
    }

    auto KillList = [&](const _tchar* szLayerTag)
        {
            if (!szLayerTag) return;

            list<CGameObject*>* pList =
                pGI->Get_ObjectList(LEVEL_GAMEPLAY, szLayerTag);
            if (!pList) return;

            for (CGameObject* pObj : *pList)
            {
                CMinion* pMinion = dynamic_cast<CMinion*>(pObj);
                if (!pMinion || pMinion->Get_Dead())
                    continue;

                // 디버그용 즉사 데미지
                pMinion->Take_Damage(999999, nullptr);
            }
        };

    KillList(szMeleeLayer);
    KillList(szRangeLayer);

    Safe_Release(pGI);
}

void CMinionManager::SpawnWave()
{
    ++m_iWaveCount;

    // 이번 웨이브 세팅
    m_bWaveInProgress = true;
    m_fWaveElapsed = 0.f;
    m_PendingSpawns.clear();

    // === desc를 받아서, "몇 번째 그룹"인지에 따라 시간 지연 부여 ===
    auto QueueSpawnWithGroup = [&](const MINION_SPAWNDESC& desc, int groupIndex)
        {
            PENDING_SPAWN ps{};
            ps.desc = desc;
            ps.fDelay = (_float)groupIndex * m_fPerMinionDelay; // 그룹 단위 시간 지연
            ps.bSpawned = false;

            m_PendingSpawns.push_back(ps);
        };

    // 한 라인에 대해 desc 하나를 만드는 헬퍼
    auto MakeDesc = [&](TEAMTYPE team, LANE_TYPE lane, MINION_TYPE type,
        int laneIdx,
        const _float3& vSpawnBase, const _float3& vTargetBase) -> MINION_SPAWNDESC
        {
            MINION_SPAWNDESC desc{};
            desc.eTeam = team;
            desc.eLane = lane;
            desc.eMinionType = type;
            desc.iLaneIndex = laneIdx;
            desc.iWaveIndex = m_iWaveCount;
            desc.vTargetPos = vTargetBase;
            desc.vSpawnPos = vSpawnBase;   // ★ 전부 같은 위치에서 생성

            return desc;
        };

    // === 그룹 인덱스: 0~(근거리-1), 그 다음은 캐스터 ===
    int groupIndex = 0;

    // 1) 근거리 3마리 * 3라인 * 2팀
    for (int i = 0; i < m_iMeleePerWave; ++i)
    {
        // BLUE
        QueueSpawnWithGroup(MakeDesc(TEAM_BLUE, LANE_TOP, MINION_MELEE, i, m_vBlueTopSpawn, m_vRedTopSpawn), groupIndex);
        QueueSpawnWithGroup(MakeDesc(TEAM_BLUE, LANE_MID, MINION_MELEE, i, m_vBlueMidSpawn, m_vRedMidSpawn), groupIndex);
        QueueSpawnWithGroup(MakeDesc(TEAM_BLUE, LANE_BOT, MINION_MELEE, i, m_vBlueBotSpawn, m_vRedBotSpawn), groupIndex);

        // RED
        QueueSpawnWithGroup(MakeDesc(TEAM_RED, LANE_TOP, MINION_MELEE, i, m_vRedTopSpawn, m_vBlueTopSpawn), groupIndex);
        QueueSpawnWithGroup(MakeDesc(TEAM_RED, LANE_MID, MINION_MELEE, i, m_vRedMidSpawn, m_vBlueMidSpawn), groupIndex);
        QueueSpawnWithGroup(MakeDesc(TEAM_RED, LANE_BOT, MINION_MELEE, i, m_vRedBotSpawn, m_vBlueBotSpawn), groupIndex);

        ++groupIndex; // 다음 근거리 그룹 시간으로
    }

    // 2) 원거리 3마리 * 3라인 * 2팀
    for (int i = 0; i < m_iCasterPerWave; ++i)
    {
        // BLUE
        QueueSpawnWithGroup(MakeDesc(TEAM_BLUE, LANE_TOP, MINION_CASTER, i, m_vBlueTopSpawn, m_vRedTopSpawn), groupIndex);
        QueueSpawnWithGroup(MakeDesc(TEAM_BLUE, LANE_MID, MINION_CASTER, i, m_vBlueMidSpawn, m_vRedMidSpawn), groupIndex);
        QueueSpawnWithGroup(MakeDesc(TEAM_BLUE, LANE_BOT, MINION_CASTER, i, m_vBlueBotSpawn, m_vRedBotSpawn), groupIndex);

        // RED
        QueueSpawnWithGroup(MakeDesc(TEAM_RED, LANE_TOP, MINION_CASTER, i, m_vRedTopSpawn, m_vBlueTopSpawn), groupIndex);
        QueueSpawnWithGroup(MakeDesc(TEAM_RED, LANE_MID, MINION_CASTER, i, m_vRedMidSpawn, m_vBlueMidSpawn), groupIndex);
        QueueSpawnWithGroup(MakeDesc(TEAM_RED, LANE_BOT, MINION_CASTER, i, m_vRedBotSpawn, m_vBlueBotSpawn), groupIndex);

        ++groupIndex;
    }
}

void CMinionManager::SpawnMinion(const MINION_SPAWNDESC& desc)
{
    const _tchar* pLayerTag = Get_PoolLayerTag(desc);
    if (!pLayerTag)
        return;

    CPoolManager* pPoolMgr = CPoolManager::GetInstance();
    if (!pPoolMgr)
        return;

    // 풀에서 하나 꺼내서 ReUse_Setting(desc) 호출 후
    // GameInstance에 다시 Add 하는 것까지 PoolManager가 처리함.
    if (!pPoolMgr->Reuse_Pooling_Object(m_eLevel, pLayerTag, (void*)&desc))
    {

    }
}

HRESULT CMinionManager::LoadSpawnPositions()
{
    HANDLE hFile = CreateFile(
        TEXT("../../../Bin/Data/PositionData/MinionSpawnData.dat"),
        GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (hFile == INVALID_HANDLE_VALUE)
        return E_FAIL;

    _ulong      dwByte = 0;
    _uint       iNum = 0;
    NONANIMDESC ModelDesc{};

    // 저장된 개수
    ReadFile(hFile, &iNum, sizeof(_uint), &dwByte, nullptr);

    for (_uint i = 0; i < iNum; ++i)
    {
        ReadFile(hFile, &ModelDesc, sizeof(NONANIMDESC), &dwByte, nullptr);
        if (dwByte == 0)
            break;

        switch (i)
        {
        case 0: m_vBlueTopSpawn = ModelDesc.vPosition; break;
        case 1: m_vBlueMidSpawn = ModelDesc.vPosition; break;
        case 2: m_vBlueBotSpawn = ModelDesc.vPosition; break;
        case 3: m_vRedTopSpawn = ModelDesc.vPosition; break;
        case 4: m_vRedMidSpawn = ModelDesc.vPosition; break;
        case 5: m_vRedBotSpawn = ModelDesc.vPosition; break;
        default:
            // 여분은 무시
            break;
        }
    }

    CloseHandle(hFile);
    return S_OK;
}

HRESULT CMinionManager::LoadLanePoints(const _tchar* pFileName, vector<_float3>& outPoints)
{
    HANDLE hFile = CreateFile(
        pFileName,
        GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (hFile == INVALID_HANDLE_VALUE)
        return E_FAIL;   // 없는 라인은 그냥 실패 반환(원하면 S_OK로 두고 무시해도 됨)

    _ulong      dwByte = 0;
    _uint       iNum = 0;
    NONANIMDESC Desc{};

    ReadFile(hFile, &iNum, sizeof(_uint), &dwByte, nullptr);

    outPoints.clear();
    outPoints.reserve(iNum);

    for (_uint i = 0; i < iNum; ++i)
    {
        ReadFile(hFile, &Desc, sizeof(NONANIMDESC), &dwByte, nullptr);
        if (dwByte == 0)
            break;

        outPoints.push_back(Desc.vPosition);
    }

    CloseHandle(hFile);
    return S_OK;
}

const _tchar* CMinionManager::Get_PoolLayerTag(const MINION_SPAWNDESC& desc)
{

    if (desc.eTeam == TEAM_BLUE)
    {
        switch (desc.eMinionType)
        {
        case MINION_MELEE:   return TEXT("Layer_Minion_Blue_Melee");
        case MINION_CASTER:  return TEXT("Layer_Minion_Blue_Range");
        default:             return nullptr;
        }
    }
    else if (desc.eTeam == TEAM_RED)
    {
        switch (desc.eMinionType)
        {
        case MINION_MELEE:   return TEXT("Layer_Minion_Red_Melee");
        case MINION_CASTER:  return TEXT("Layer_Minion_Red_Range");
        default:             return nullptr;
        }
    }

    return nullptr;
}

void CMinionManager::Free()
{
    __super::Free();
}