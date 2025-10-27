#pragma once
#include "Player.h"


// 챔피언의 스탯을 관리한다.
NS_BEGIN(Client)
class CChampion abstract : public CPlayer
{
	struct ChampionStat
	{
		wstring Name; // 챔피언 이름
		int iLevel; // 현재 레벨

		// 기본 능력치
		float Hp; // 체력
		float HpPerLevel; // 레벨당 체력 증가량
		
		float Mp; // 마나
		float MpPerLevel; // 레벨당 마나 증가량

		float MoveSpeed; // 이동속도
		float AttackRange; // 공격 사거리
		
		// ---------- 공격 ------------
		float AttackDamage; // 기본공격력
		float AttackDamagePerLevel; // 레벨당 공격력 증가량

		float AttackSpeed; // 기본 공격속도
		float AttackSpeedPerLevel;

		float Critical;	// 치명타 확률
		float CritDamage; // 치명타 피해량
		
		// -----------방어 ------------
		float Armor; // 방어력
		float ArmorPerLevel;
		
		// ------- 회복 관련 ---------
		float HPRegen; // 초당 체력 재생
		float HpRegenPerLevel; // 초당 체력 재생 증가량

		float MPRegen; // 초당 마나 재생
		float MpRegenPerLevel; // 초당 마나 재생 증가량
	};

protected:
	CChampion(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CChampion(const CChampion& rhs);
	virtual ~CChampion() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual int Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);
	virtual HRESULT Render();

protected:
	void UpdateLevelStats();

protected:
	virtual HRESULT Ready_Components(void* pArg) = 0;
	virtual HRESULT SetUp_ShaderResources();

public:
	virtual void Free() override;
};
NS_END
