#pragma once

/* 레퍼런스카운트를 관리해준다. */
/* 관리 : 레퍼런스카운트를 증가시킨다*/
/*     : 레퍼런스카운트를 감소시키거나 삭제한다. */
#include "Engine_Define.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBase abstract
{
protected:
	CBase();
	virtual ~CBase() = default;

public:
	/* 증가시킨 이후의 값을 리턴한다. */
	unsigned int AddRef();
	/* 감소시키기전에 값을 리턴한다. */
	unsigned int Release();

private:
	unsigned int			m_iRefCnt = {};

public:
	virtual void Free();
};

NS_END