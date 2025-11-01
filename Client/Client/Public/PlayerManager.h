#pragma once
#include "Base.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CGameObject;
NS_END

NS_BEGIN(Client)
class CPlayer;
class CChampion;

class CPlayerManager : public CBase
{
	DECLARE_SINGLETON(CPlayerManager)

public:
	enum TYPE { PLAYER, ALLY, ENEMY, PLAYER_END };

private:
	explicit CPlayerManager();
	virtual ~CPlayerManager() = default;

private:
	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;

	// 현재 조종 중인 플레이어
	CChampion* m_pActivePlayer = nullptr;

	// 전체 플레이어들 (자기 자신 포함)
	vector<CChampion*> m_AllPlayers;
public:
	HRESULT Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	void Update(_float fTimeDelta);
	void Late_Update(_float fTimeDelta);
	void Render();

	// 플레이어 생성 및 등록
	HRESULT Add_Player(TYPE eType, CChampion* pPlayer);
	void ChangeActivePlayer(TYPE eType); // F4 등으로 전환
	CChampion* Get_ActivePlayer() const { return m_pActivePlayer; }
	void ChangeChampion();

	void Free() override;

};
NS_END

