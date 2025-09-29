#pragma once

#include "Base.h"

/* 현재 구동 해야 할 레벨을 보관한다. */
/* 보관하고 있는 레벨의 반복적인 업데이트 렌더를 수행한다. */
/* 레벨의 교체 (이전레벨은 삭제한다. + 기존 레벨용 자원을 정리해준다 ) */

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