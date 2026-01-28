// ===============================
// PlayerRecallState.h (전체 수정본)
// ===============================
#pragma once
#include "PlayerState.h"

NS_BEGIN(Client)
NS_BEGIN(Player)

class CPlayerRecallState final : public CPlayerState
{
public:
    CPlayerRecallState(CPlayer* pPlayer);

public:
    virtual CPlayerState* HandleInput() override;
    virtual CPlayerState* Update(_float fTimeDelta) override;
    virtual CPlayerState* Late_Update(_float fTimeDelta) override;

    virtual void Enter() override;
    virtual void Exit() override;

private:
    enum RECALL_PHASE
    {
        PHASE_RECALL,
        PHASE_WINDDOWN,
    };

private:
    void CancelRecall();     // 우클릭 캔슬
    void FinishRecall();     // 완료 처리(Recall2 재생 포함)
    void StopRecallFX();     // FX 정지/삭제

private:
    RECALL_PHASE m_ePhase = PHASE_RECALL;

    bool m_bIsRecall = false;

    // 상태 내부 플래그
    bool m_bRecallStartFx = false;
    bool m_bRecallFinishSfx = false;   // Recall2 1회 보장
    bool m_bFinishing = false;         // ★ 완료로 빠져나가는 중인지(Exit에서 StopSound 제어)

    vector<class CEffect*> m_RecallFX; // 리콜 FX 핸들
};

NS_END
NS_END

