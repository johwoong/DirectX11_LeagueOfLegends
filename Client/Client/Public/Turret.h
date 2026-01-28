#pragma once
#include "Building.h"

// 억제기 
NS_BEGIN(Client)
class CTurret final : public CBuilding
{
public:
	enum TEAMTYPE{RED, BLUE};
	enum STATE { DEATH, DESTORY, IDLE };

private:
	struct TURRET_STATS
	{
		float fAttackRange = 11.f;   // 사거리
		float fAggroRange = 11.f;   // 어그로 범위(그냥 사거리랑 같게)
		float fAttackDamage = 150.f;
		float fAttackSpeed = 0.8f;   // 0.8타 / sec
	}m_tStats;

private:
	CTurret(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CTurret(const CTurret& rhs);
	virtual ~CTurret() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual int		Update(_float fTimeDelta);
	virtual void	Late_Update(_float fTimeDelta);

public: /*For.State*/
	virtual _bool	Is_AnimationLoop(_uint eAnimId);
	void			Update_State(_float fTimeDelta);
	void			LateUpdate_State(_float fTimeDelta);


protected:
	virtual _int Take_Damage(int iDamage, CActor* DamageCauser) override;  // ★ 추가

public:
	virtual HPBARDESC Get_HpBarDesc() const override;
private:
	virtual HRESULT Ready_Components(void* pArg);

private:
	_uint m_eState = IDLE;
	class CAttackRange* m_pAttackRange = nullptr; // UI 링
	float   m_fAttackTimer = 0.f;
	float   m_fAttackCoolTime = 0.f;
	CActor* m_pTarget = nullptr;
private:
	bool m_bDeathSfxPlayed = false;
public:
	static CTurret* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free() override;
};
NS_END

