#include "../public/Base.h"

CBase::CBase()
{
}

unsigned int CBase::AddRef()
{
	return ++m_iRefCnt;
}

unsigned int CBase::Release()
{ 
	if (0 != m_iRefCnt)
	{
		return m_iRefCnt--;
	}
	else
	{
		Free();

		delete this;

		return 0;
	}
}

void CBase::Free()
{
}