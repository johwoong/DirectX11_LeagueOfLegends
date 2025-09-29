#pragma once

#include "Base.h"

/* ���� ���� �ؾ� �� ������ �����Ѵ�. */
/* �����ϰ� �ִ� ������ �ݺ����� ������Ʈ ������ �����Ѵ�. */
/* ������ ��ü (���������� �����Ѵ�. + ���� ������ �ڿ��� �������ش� ) */

NS_BEGIN(Engine)

class CLevel_Manager final : public CBase
{
private:
	CLevel_Manager();
	virtual ~CLevel_Manager() = default;

public:
	HRESULT Change_Level(_uint iCurrentLevelID, class CLevel* pNewLevel);
	void Update(_float fTimeDelta);
	HRESULT Render();

private:	
	_uint			m_iCurrentLevelID = {};
	class CLevel*	m_pCurrentLevel = { nullptr };	

private:
	class CGameInstance* m_pGameInstance = { nullptr };

public:
	static CLevel_Manager* Create();
	virtual void Free() override;
};

NS_END