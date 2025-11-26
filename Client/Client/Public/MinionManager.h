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
    HRESULT Initialize(LEVEL eOwnerLevel);
    void    Update(_float fTimeDelta);

    void    SpawnSingle(const MINION_SPAWNDESC& desc);
    void    ForceSpawnWave();
    void    Debug_KillTeamMinions(TEAMTYPE eTeam);

public:
    const vector<_float3>& GetLanePath(LANE_TYPE eLane) const;
    _int  GetLaneStartIndex(TEAMTYPE eTeam, const vector<_float3>& path) const;
    _int  GetLaneStep(TEAMTYPE eTeam) const;

private:
    void    SpawnWave();
    void    SpawnMinion(const MINION_SPAWNDESC& desc);

    HRESULT LoadSpawnPositions();
    HRESULT LoadLanePoints(const _tchar* pFileName, vector<_float3>& outPoints);

    const _tchar* Get_PoolLayerTag(const MINION_SPAWNDESC& desc);

private:
    LEVEL m_eLevel = LEVEL_GAMEPLAY;

    _float m_fWaveInterval = 30.f;
    _float m_fWaveTimer = 0.f;
    _int   m_iWaveCount = 0;

    _float3 m_vBlueTopSpawn = {};
    _float3 m_vBlueMidSpawn = {};
    _float3 m_vBlueBotSpawn = {};

    _float3 m_vRedTopSpawn = {};
    _float3 m_vRedMidSpawn = {};
    _float3 m_vRedBotSpawn = {};

private:
    vector<_float3> m_TopLanePoints;
    vector<_float3> m_MidLanePoints;
    vector<_float3> m_BotLanePoints;

    _int m_iMeleePerWave = 3;
    _int m_iCasterPerWave = 3;

private:
    struct PENDING_SPAWN
    {
        MINION_SPAWNDESC desc;
        _float           fDelay = 0.f;
        _bool            bSpawned = false;
    };

    vector<PENDING_SPAWN> m_PendingSpawns;
    _bool  m_bWaveInProgress = false;
    _float m_fWaveElapsed = 0.f;
    _float m_fPerMinionDelay = 0.5f;

private:
    virtual void Free() override;
};

