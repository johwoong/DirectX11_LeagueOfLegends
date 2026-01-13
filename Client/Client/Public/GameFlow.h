#pragma once
#include "Base.h"

NS_BEGIN(Client)

class CGameFlow final : public CBase
{
    DECLARE_SINGLETON(CGameFlow)

private:
    CGameFlow() = default;
    virtual ~CGameFlow() = default;

public:
    void  Set_GameOver(bool b) { m_bGameOver = b; }
    bool  Is_GameOver() const { return m_bGameOver; }

    void  Set_UnscaledDT(float dt) { m_fUnscaledDT = dt; }
    float Get_UnscaledDT() const { return m_fUnscaledDT; }

private:
    bool  m_bGameOver = false;
    float m_fUnscaledDT = 0.f;

public:
    virtual void Free() override {}
};

NS_END