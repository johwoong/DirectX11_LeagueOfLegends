#pragma once
#include "PlayerState.h"

NS_BEGIN(Client)
NS_BEGIN(Player)
class CEzrealSkillStateR final : public CPlayerState
{
public:
    CEzrealSkillStateR(CPlayer* pPlayer);

    virtual CPlayerState* HandleInput() override;
    virtual CPlayerState* Update(_float fTimeDelta) override;
    virtual CPlayerState* Late_Update(_float fTimeDelta) override;

    virtual void Enter() override;
    virtual void Exit() override;

private:
    bool   m_bProjectileSpawned = false;   // 이벤트 기반 발사
    _float3 m_vDir = { 0,0,0 };            // 마우스 방향

    // ? 추가: R 종료 후 R_IDLE로 자연스럽게 넘어가기용
    bool   m_bPlayRIdle = false;           // 지금 R_IDLE 재생중인가?
};
NS_END
NS_END

