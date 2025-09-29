#pragma once

#include "Base.h"

NS_BEGIN(Engine)


class CTimer_Manager : public CBase
{
private:
	explicit CTimer_Manager();
	~CTimer_Manager() = default;

public:
	_float			Get_TimeDelta(const _tchar* pTimerTag);

public:
	HRESULT			Add_Timer(const _tchar* pTimerTag);
	void			Compute_TimeDelta(const _tchar* pTimerTag);
private:
	map<const _wstring, class CTimer*>		m_mapTimer;	
private:
	class CTimer* Find_Timer(const _tchar* pTimerTag);
public:
	static CTimer_Manager* Create();
	virtual void		Free();
};

NS_END