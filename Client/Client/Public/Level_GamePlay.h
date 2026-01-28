#pragma once

#include "Client_Defines.h"
#include "Level.h"

NS_BEGIN(Client)
class CNexus;
class CLevel_GamePlay final : public CLevel
{
private:
	CLevel_GamePlay(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CLevel_GamePlay() = default;


public:
	virtual HRESULT Initialize() override;
	virtual void Update(_float fTimeDelta) override;
	virtual void Late_Update(_float fTimeDelta) override;
public:
	HRESULT Ready_Layer_Terrian(const _tchar* pLayerTag);
	HRESULT Ready_Lights();
	HRESULT Ready_Layer_MapObject(const _tchar* pLayerTag);
	HRESULT Ready_Layer_Building(const _tchar* pLayerTag);
	HRESULT Ready_Layer_Player(const _tchar* pLayerTag);
	HRESULT Ready_Layer_Enemy(const _tchar* pLayerTag);
	HRESULT Ready_Layer_Camera(const _tchar* pLayerTag);
	HRESULT Ready_Layer_UI(const _tchar* pLayerTag);
	HRESULT Ready_Layer_Minion();
	HRESULT Ready_Layer_Projectile();

private:
	void Check_GameOver_RedNexus();
	void Start_GameOver_Victory();
	

private:
	ID3D11Device* m_pDevice = { nullptr };
	ID3D11DeviceContext* m_pContext = { nullptr };
	bool   m_bGameOver = false;
	bool   m_bVictoryUISpawned = false;
	CNexus* m_pRedNexus = nullptr; // ★ 레드 넥서스 포인터
private:
	float m_fWelcomeAcc = 0.f;
	bool  m_bWelcomePlayed = false;
public:
	static CLevel_GamePlay* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void Free() override;
};

NS_END