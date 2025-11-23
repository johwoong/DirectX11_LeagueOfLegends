#pragma once
#include "Base.h"
#include "Client_Defines.h"
#include "Minion.h"
class CMinionManager :
    public CBase
{
    DECLARE_SINGLETON(CMinionManager)

private:
    CMinionManager();
    virtual ~CMinionManager() = default;

public:
    // 어떤 레벨에서 미니언을 운용할지 지정 
    HRESULT Initialize(LEVEL eOwnerLevel);
    void    Update(_float fTimeDelta);

    // 디버그용: 원하는 스폰Desc로 한 마리 강제 소환
    void    SpawnSingle(const MINION_SPAWNDESC& desc);

public:
    const vector<_float3>& GetLanePath(LANE_TYPE eLane) const
    {
        switch (eLane)
        {
        case LANE_TOP: return m_TopLanePoints;
        case LANE_MID: return m_MidLanePoints;
        case LANE_BOT: return m_BotLanePoints;
        }
        // 안전용, 빈 static 벡터 하나 만들어서 리턴해도 되고 assert 걸어도 되고
        static vector<_float3> dummy;
        return dummy;
    }

    _int GetLaneStartIndex(TEAMTYPE eTeam, const vector<_float3>& path) const
    {
        if (path.empty()) return -1;
        return (eTeam == TEAM_BLUE) ? 0 : (_int)path.size() - 1;
    }

    _int GetLaneStep(TEAMTYPE eTeam) const
    {
        return (eTeam == TEAM_BLUE) ? +1 : -1;
    }

    // 웨이브 즉시 생성(디버그용)
    void    ForceSpawnWave();
public:
    // 디버그용 : 특정 팀 미니언 전부 즉사
    void Debug_KillTeamMinions(TEAMTYPE eTeam);
private:
    void    SpawnWave();                      // 내부에서 한 웨이브 생성
    void    SpawnMinion(const MINION_SPAWNDESC& desc);
    HRESULT LoadSpawnPositions();
    HRESULT LoadLanePoints(const _tchar* pFileName, vector<_float3>& outPoints);


    const _tchar* Get_PoolLayerTag(const MINION_SPAWNDESC& desc);

private:
    LEVEL   m_eLevel = LEVEL_END;

    // 웨이브 타이머
    _float  m_fWaveInterval = 30.f;    // 30초마다 웨이브 (원하는 값으로 조정)
    _float  m_fWaveTimer = 0.f;
    _int    m_iWaveCount = 0;       // 몇 번째 웨이브인지

    _float3 m_vBlueTopSpawn = {};
    _float3 m_vBlueMidSpawn = {};
    _float3 m_vBlueBotSpawn = {};

    _float3 m_vRedTopSpawn = {};
    _float3 m_vRedMidSpawn = {};
    _float3 m_vRedBotSpawn = {};

private:
    // 라인별 웨이포인트 (블루 넥서스 → 레드 넥서스 방향으로 찍음)
    vector<_float3> m_TopLanePoints;
    vector<_float3> m_MidLanePoints;
    vector<_float3> m_BotLanePoints;

private:
    // 한 웨이브 당 미니언 수
    _int m_iMeleePerWave = 3;   // 근거리 3
    _int m_iCasterPerWave = 3;  // 원거리 3

    // ====== 웨이브 내부 “한 마리씩” 생성용 ======
    struct PENDING_SPAWN
    {
        MINION_SPAWNDESC desc; // 어디, 어떤 미니언인지
        _float            fDelay;   // 웨이브 시작 후 몇 초 뒤에 소환할지
        _bool             bSpawned; // 이미 소환됐는지
    };

    vector<PENDING_SPAWN> m_PendingSpawns;
    _bool  m_bWaveInProgress = false; // 현재 웨이브에서 아직 안 나온 미니언이 있는지
    _float m_fWaveElapsed = 0.f;   // 이번 웨이브 시작 후 경과 시간
    _float m_fPerMinionDelay = 0.5f;  // 미니언 한 마리마다 시간 간격 (롤 느낌으로 조절)

    void Free();
};

