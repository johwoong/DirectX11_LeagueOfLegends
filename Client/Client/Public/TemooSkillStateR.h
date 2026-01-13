#pragma once
#include "PlayerState.h"
#include "Temoo.h"

NS_BEGIN(Client)

NS_BEGIN(Player)
class CTemooSkillStateR final : public CPlayerState
{
public:
    explicit CTemooSkillStateR(CPlayer* pPlayer);
    virtual ~CTemooSkillStateR() = default;

public:
    virtual void Enter() override;
    virtual void Exit() override;

    virtual CPlayerState* HandleInput() override;
    virtual CPlayerState* Update(_float fTimeDelta) override;
    virtual CPlayerState* Late_Update(_float fTimeDelta) override;

private:
    bool Get_MouseGroundPickPos(_float3& outPos);
    void ComputeDirFaceAndAnim();     // ? 이름 통일 (cpp도 이걸로)
    void Spawn_MushRoom();

private:
    bool         m_bSpawned = false;
    _float3      m_vCastDir = { 0,0,1 };      // 마우스 방향(XZ)
    _vector      m_vPlacePos = XMVectorZero(); // 설치 위치
    CTemoo::ANIM m_eAnim = CTemoo::R_0;
};
NS_END
NS_END