#pragma once
#include "Minion_State.h"

NS_BEGIN(Client)
NS_BEGIN(Minion)
class CAttackState :
    public CMinionState
{
public:
    CAttackState(class CMinion* pMinion);

public:
    virtual CMinionState* AI_Behaviour(_float fTimeDelta) override;
    virtual CMinionState* Update(_float fTimeDelta)       override;
    virtual CMinionState* Late_Update(_float fTimeDelta)   override;

    virtual void Enter() override;
    virtual void Exit()  override;

private:
    CColider* m_pAttackColider = nullptr;

    _matrix m_ColliderMatrix = XMMatrixIdentity();

private:
    CColider* Get_Collider(CColider::TYPE eType, _float3 vScale, _float3 vRotation, _float3 vPosition);

private:
    _float m_fAttackElapsed = 0.f;
    _bool  m_bDamageApplied = false;

};
NS_END
NS_END
