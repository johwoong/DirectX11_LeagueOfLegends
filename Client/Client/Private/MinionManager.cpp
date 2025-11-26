#include "MinionManager.h"
#include "PoolManager.h"
#include "GameInstance.h"

IMPLEMENT_SINGLETON(CMinionManager)

CMinionManager::CMinionManager()
{
}

HRESULT CMinionManager::Initialize(LEVEL eOwnerLevel)
{
    m_eLevel = eOwnerLevel;
    m_fWaveTimer = 0.f;
    m_iWaveCount = 0;

    if (FAILED(LoadSpawnPositions()))
        return E_FAIL;

    LoadLanePoints(TEXT("../../../Bin/Data/PositionData/MinionTopDestnation.dat"), m_TopLanePoints);
    LoadLanePoints(TEXT("../../../Bin/Data/PositionData/MinionMidDestnation.dat"), m_MidLanePoints);
    LoadLanePoints(TEXT("../../../Bin/Data/PositionData/MinionBottomDestnation.dat"), m_BotLanePoints);

    return S_OK;
}

void CMinionManager::Update(_float fTimeDelta)
{
    if (m_eLevel == LEVEL_END)
        return;

    CGameInstance* GI = CGameInstance::GetInstance();
    GI->AddRef();

    if (GI->Key_Down(DIK_1))
        ForceSpawnWave();
    if (GI->Key_Down(DIK_2)) Debug_KillTeamMinions(TEAM_RED);
    if (GI->Key_Down(DIK_3)) Debug_KillTeamMinions(TEAM_BLUE);

    GI->Release();

    // ------ Existing wave pending ------
    if (m_bWaveInProgress)
    {
        m_fWaveElapsed += fTimeDelta;
        _bool allSpawned = true;

        for (auto& ps : m_PendingSpawns)
        {
            if (!ps.bSpawned && m_fWaveElapsed >= ps.fDelay)
            {
                SpawnMinion(ps.desc);
                ps.bSpawned = true;
            }
            if (!ps.bSpawned)
                allSpawned = false;
        }

        if (allSpawned)
        {
            m_bWaveInProgress = false;
            m_PendingSpawns.clear();
            m_fWaveElapsed = 0.f;
        }
    }
    // ------ Wait for next wave ------
    else
    {
        m_fWaveTimer += fTimeDelta;
        if (m_fWaveTimer >= m_fWaveInterval)
        {
            m_fWaveTimer = 0.f;
            SpawnWave();
        }
    }
}

void CMinionManager::SpawnSingle(const MINION_SPAWNDESC& desc)
{
    SpawnMinion(desc);
}

void CMinionManager::ForceSpawnWave()
{
    m_fWaveTimer = 0.f;
    m_bWaveInProgress = false;
    m_PendingSpawns.clear();
    m_fWaveElapsed = 0.f;

    SpawnWave();
}

void CMinionManager::Debug_KillTeamMinions(TEAMTYPE eTeam)
{
    CGameInstance* GI = CGameInstance::GetInstance();
    GI->AddRef();

    const _tchar* meleeLayer = nullptr;
    const _tchar* rangeLayer = nullptr;

    if (eTeam == TEAM_RED)
    {
        meleeLayer = TEXT("Layer_Minion_Red_Melee");
        rangeLayer = TEXT("Layer_Minion_Red_Range");
    }
    else
    {
        meleeLayer = TEXT("Layer_Minion_Blue_Melee");
        rangeLayer = TEXT("Layer_Minion_Blue_Range");
    }

    auto KillList = [&](const _tchar* layer)
        {
            if (!layer) return;

            auto* list = GI->Get_ObjectList(LEVEL_GAMEPLAY, layer);
            if (!list) return;

            for (auto* obj : *list)
            {
                CMinion* m = dynamic_cast<CMinion*>(obj);
                if (!m || m->Get_Dead())
                    continue;

                m->Take_Damage(999999, nullptr);
            }
        };

    KillList(meleeLayer);
    KillList(rangeLayer);

    GI->Release();
}

void CMinionManager::SpawnWave()
{
    ++m_iWaveCount;

    m_bWaveInProgress = true;
    m_fWaveElapsed = 0.f;
    m_PendingSpawns.clear();

    auto AddPending = [&](const MINION_SPAWNDESC& d, int group)
        {
            PENDING_SPAWN ps{};
            ps.desc = d;
            ps.fDelay = group * m_fPerMinionDelay;
            ps.bSpawned = false;
            m_PendingSpawns.push_back(ps);
        };

    auto MakeDesc = [&](TEAMTYPE team, LANE_TYPE lane, MINION_TYPE type,
        int laneIdx, const _float3& spawn, const _float3& target)
        {
            MINION_SPAWNDESC d{};
            d.eTeam = team;
            d.eLane = lane;
            d.eMinionType = type;
            d.iLaneIndex = laneIdx;
            d.iWaveIndex = m_iWaveCount;
            d.vSpawnPos = spawn;
            d.vTargetPos = target;
            return d;
        };

    int group = 0;

    // Melee
    for (int i = 0; i < m_iMeleePerWave; ++i)
    {
        AddPending(MakeDesc(TEAM_BLUE, LANE_TOP, MINION_MELEE, i, m_vBlueTopSpawn, m_vRedTopSpawn), group);
        AddPending(MakeDesc(TEAM_BLUE, LANE_MID, MINION_MELEE, i, m_vBlueMidSpawn, m_vRedMidSpawn), group);
        AddPending(MakeDesc(TEAM_BLUE, LANE_BOT, MINION_MELEE, i, m_vBlueBotSpawn, m_vRedBotSpawn), group);

        AddPending(MakeDesc(TEAM_RED, LANE_TOP, MINION_MELEE, i, m_vRedTopSpawn, m_vBlueTopSpawn), group);
        AddPending(MakeDesc(TEAM_RED, LANE_MID, MINION_MELEE, i, m_vRedMidSpawn, m_vBlueMidSpawn), group);
        AddPending(MakeDesc(TEAM_RED, LANE_BOT, MINION_MELEE, i, m_vRedBotSpawn, m_vBlueBotSpawn), group);

        ++group;
    }

    // Caster
    for (int i = 0; i < m_iCasterPerWave; ++i)
    {
        AddPending(MakeDesc(TEAM_BLUE, LANE_TOP, MINION_CASTER, i, m_vBlueTopSpawn, m_vRedTopSpawn), group);
        AddPending(MakeDesc(TEAM_BLUE, LANE_MID, MINION_CASTER, i, m_vBlueMidSpawn, m_vRedMidSpawn), group);
        AddPending(MakeDesc(TEAM_BLUE, LANE_BOT, MINION_CASTER, i, m_vBlueBotSpawn, m_vRedBotSpawn), group);

        AddPending(MakeDesc(TEAM_RED, LANE_TOP, MINION_CASTER, i, m_vRedTopSpawn, m_vBlueTopSpawn), group);
        AddPending(MakeDesc(TEAM_RED, LANE_MID, MINION_CASTER, i, m_vRedMidSpawn, m_vBlueMidSpawn), group);
        AddPending(MakeDesc(TEAM_RED, LANE_BOT, MINION_CASTER, i, m_vRedBotSpawn, m_vBlueBotSpawn), group);

        ++group;
    }
}

void CMinionManager::SpawnMinion(const MINION_SPAWNDESC& desc)
{
    const _tchar* tag = Get_PoolLayerTag(desc);
    if (!tag)
        return;

    if (CPoolManager* pool = CPoolManager::GetInstance())
    {
        pool->Reuse_Pooling_Object(m_eLevel, tag, (void*)&desc);
    }
}

HRESULT CMinionManager::LoadSpawnPositions()
{
    HANDLE hFile = CreateFile(
        TEXT("../../../Bin/Data/PositionData/MinionSpawnData.dat"),
        GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (hFile == INVALID_HANDLE_VALUE)
        return E_FAIL;

    _ulong bytes = 0;
    _uint count = 0;
    NONANIMDESC d{};

    ReadFile(hFile, &count, sizeof(_uint), &bytes, nullptr);

    for (_uint i = 0; i < count; ++i)
    {
        ReadFile(hFile, &d, sizeof(NONANIMDESC), &bytes, nullptr);
        if (bytes == 0)
            break;

        switch (i)
        {
        case 0: m_vBlueTopSpawn = d.vPosition; break;
        case 1: m_vBlueMidSpawn = d.vPosition; break;
        case 2: m_vBlueBotSpawn = d.vPosition; break;
        case 3: m_vRedTopSpawn = d.vPosition; break;
        case 4: m_vRedMidSpawn = d.vPosition; break;
        case 5: m_vRedBotSpawn = d.vPosition; break;
        default: break;
        }
    }

    CloseHandle(hFile);
    return S_OK;
}

HRESULT CMinionManager::LoadLanePoints(const _tchar* file, vector<_float3>& out)
{
    HANDLE hFile = CreateFile(
        file, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (hFile == INVALID_HANDLE_VALUE)
        return E_FAIL;

    _ulong bytes = 0;
    _uint count = 0;
    NONANIMDESC d{};

    ReadFile(hFile, &count, sizeof(_uint), &bytes, nullptr);

    out.clear();
    out.reserve(count);

    for (_uint i = 0; i < count; ++i)
    {
        ReadFile(hFile, &d, sizeof(NONANIMDESC), &bytes, nullptr);
        if (bytes == 0)
            break;

        out.push_back(d.vPosition);
    }

    CloseHandle(hFile);
    return S_OK;
}

const _tchar* CMinionManager::Get_PoolLayerTag(const MINION_SPAWNDESC& desc)
{
    if (desc.eTeam == TEAM_BLUE)
    {
        return (desc.eMinionType == MINION_MELEE)
            ? TEXT("Layer_Minion_Blue_Melee")
            : TEXT("Layer_Minion_Blue_Range");
    }
    else
    {
        return (desc.eMinionType == MINION_MELEE)
            ? TEXT("Layer_Minion_Red_Melee")
            : TEXT("Layer_Minion_Red_Range");
    }
}

const vector<_float3>& CMinionManager::GetLanePath(LANE_TYPE eLane) const
{
    switch (eLane)
    {
    case LANE_TOP: return m_TopLanePoints;
    case LANE_MID: return m_MidLanePoints;
    case LANE_BOT: return m_BotLanePoints;
    }
    static vector<_float3> dummy;
    return dummy;
}

_int CMinionManager::GetLaneStartIndex(TEAMTYPE eTeam, const vector<_float3>& path) const
{
    if (path.empty()) return -1;
    return (eTeam == TEAM_BLUE) ? 0 : (path.size() - 1);
}

_int CMinionManager::GetLaneStep(TEAMTYPE eTeam) const
{
    return (eTeam == TEAM_BLUE) ? +1 : -1;
}

void CMinionManager::Free()
{
    __super::Free();
}
