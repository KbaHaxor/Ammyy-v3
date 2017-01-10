#include "stdafx.h"
#include "RLException.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

RLException::RLException()
{
}

RLException::RLException(LPCSTR templ, ...)
{
	va_list ap;
	va_start(ap, templ);
	m_description.FormatV(templ, ap);	
	va_end(ap);		
}


RLException::~RLException()
{	
}

LPCSTR RLException::GetDescription()
{
	return (LPCSTR)m_description;
}
