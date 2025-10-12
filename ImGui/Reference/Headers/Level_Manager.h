#pragma once

#include "Base.h"

/* 현재 구동 해야 할 레벨을 보관한다. */
/* 보관하고 있는 레벨의 반복적인 업데이트 렌더를 수행한다. */
/* 레벨의 교체 (이전레벨은 삭제한다. + 기존 레벨용 자원을 정리해준다 ) */

NS_BEGIN(Engine)

class CLevel_Manager final : public CBase
{
	DECLARE_SINGLETON(CLevel_Manager)
public:
	CLevel_Manager();
	virtual ~CLevel_Manager() = default;

public:
	HRESULT Open_Level(unsigned int iLevelIndex, class CLevel* pNewLevel);
	void Update(_float fTimeDelta);
	void Late_Update(_float fTimeDelta);


public:
	class CLevel* Get_CurrentLevel() { return m_pCurrentLevel; }
	_uint Get_CurrentLevelIndex() { return m_iLevelIndex; }
	_uint Get_DestinationLevelIndex() { return m_iDestinationIndex; }
	_uint Get_PastLevelIndex() { return m_iPastLevelIndex; }
	void  Set_DestinationLevel(_uint LevelIndex) { m_iDestinationIndex = LevelIndex; }

private:
	class CLevel*				m_pCurrentLevel = { nullptr };
	_uint						m_iLevelIndex;
	_uint						m_iDestinationIndex;
	_uint						m_iPastLevelIndex;

public:
	virtual void Free() override;
};

NS_END