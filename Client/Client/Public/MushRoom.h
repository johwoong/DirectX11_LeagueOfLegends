#pragma once
#include "Actor.h"
#include "Client_Defines.h"

#include <vector>
#include <algorithm>

NS_BEGIN(Engine)
class CModel;
NS_END

NS_BEGIN(Client)

class CMushRoom final : public CActor
{
public:
    struct MUSHROOM_DESC
    {
        _float3  vPos = { 0,0,0 };
        _float3  vDir = { 0,0,1 };
        CActor* pOwner = nullptr;

        _float   fLifeTime = 300.f;
        _float   fTriggerRadius = 0.9f;

        int      iExplodeDamage = 40;
        _float   fPoisonDuration = 4.f;
        _float   fPoisonTickInterval = 1.f;
        int      iPoisonDamagePerTick = 20;

        const _tchar* pExplodeDat = TEXT("Temoo_R_Explode.dat");
    };

public:
    enum ANIM { IDLE, SPAWN, IDLE_END };

public:
    // ? ¼³Ä¡ À§Ä¡ °ãÄ§ ÇØ°á: ±âÁ¸ ¹ö¼¸°ú °ãÄ¡¸é -dir·Î ¹Ð±â
    static void ResolvePlacePos_Backward(_vector& ioPos, _vector dirXZ, float minDist = 1.8f, int maxIter = 8);

private:
    CMushRoom(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CMushRoom(const CMushRoom& rhs);
    virtual ~CMushRoom() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* pArg) override;
    virtual int Update(_float fTimeDelta) override;
    virtual void Late_Update(_float fTimeDelta) override;
    virtual HRESULT Render() override;
    virtual HRESULT Render_Glow() override;
    virtual HRESULT Render_ShadowDepth() override;

public:
    static CMushRoom* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr) override;
    virtual void Free() override;

private:
    virtual HRESULT Ready_Components(void* pArg = nullptr) override;
    virtual HRESULT SetUp_ShaderResources() override;

private:
    void Apply_DirToTransform();
    void Trigger_Explode(CActor* pVictim);

    void Update_Sphere_NoScale();

private:
    void RegisterSelf();
    void UnregisterSelf();

private:
    static vector<CMushRoom*> s_Alive;
    bool m_bRegistered = false;
private:
    _vector m_vPlaceStart = XMVectorZero();
    _vector m_vPlaceFinal = XMVectorZero();
    _float  m_fPlaceAcc = 0.f;
    _float  m_fPlaceTime = 0.12f;   // ? Æ¨±è ¿¬Ãâ ½Ã°£
    bool    m_bPlaceSlide = false;
private:
    CModel* m_pModelCom = nullptr;
    MUSHROOM_DESC m_Desc{};

    _float  m_fLifeAcc = 0.f;
    bool    m_bTriggered = false;
    bool    m_bArmed = false;
};

NS_END
