#pragma once
#include "Base.h"
#include "Client_Defines.h"
#include "UI_ExpBar.h"

NS_BEGIN(Engine)
class CGameObject;
NS_END

NS_BEGIN(Client)
class CPlayer;
class CChampion;
class CUI_SkillMenu;
class CAttackRange;
class CPlayer_HpBar;
class CPlayerManager : public CBase
{
	DECLARE_SINGLETON(CPlayerManager)

public:
	enum TYPE { PLAYER, ALLY, ENEMY, PLAYER_END };

private:
	explicit CPlayerManager();
	virtual ~CPlayerManager() = default;

public:
	CChampion* Get_ActivePlayer() const { return m_pActivePlayer; }
	void        Set_ActivePlayer(CChampion* pPlayer);
	void Set_SkillMenu(CUI_SkillMenu* pMenu);
	
	void Set_ExpBar(CUI_ExpBar* pBar);
public:
	HRESULT Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	void    ChangeChampion(); // F4로 호출 시 활성 플레이어 교체

private:
	ID3D11Device*			m_pDevice	=	{ nullptr };
	ID3D11DeviceContext*	m_pContext	=	{ nullptr };

private:
	vector<CChampion*> m_vChampions;     // 생성된 챔피언 목록
	_int                    m_iCurrentChampionIndex = 0; // 현재 조종 중인 챔피언 인덱스
	CChampion* m_pActivePlayer = nullptr; 	// 현재 조작 중인 플레이어
	CUI_SkillMenu* m_pSkillMenu = nullptr;
private:
	CAttackRange* m_pAttackRange = nullptr;
private:
	CUI_ExpBar* m_pExpBar = nullptr;
private:
	CPlayer_HpBar* m_pPlayerHpBar = nullptr; // ★ 1개만 들고감
public:
	virtual void Free() override;

};
NS_END

