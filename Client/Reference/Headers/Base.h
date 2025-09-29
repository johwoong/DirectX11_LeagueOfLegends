#pragma once

/* ���۷���ī��Ʈ�� �������ش�. */
/* ���� : ���۷���ī��Ʈ�� ������Ų��*/
/*     : ���۷���ī��Ʈ�� ���ҽ�Ű�ų� �����Ѵ�. */
#include "Engine_Define.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBase abstract
{
protected:
	CBase();
	virtual ~CBase() = default;

public:
	/* ������Ų ������ ���� �����Ѵ�. */
	unsigned int AddRef();
	/* ���ҽ�Ű������ ���� �����Ѵ�. */
	unsigned int Release();

private:
	unsigned int			m_iRefCnt = {};

public:
	virtual void Free();
};

NS_END