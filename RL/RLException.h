#if !defined(RL_EXCEPTION_H__INCLUDED_)
#define RL_EXCEPTION_H__INCLUDED_

#include "StringA.h"

class RLException 
{
public:
	virtual LPCSTR GetDescription();	
	RLException();
	RLException(LPCSTR templ, ...);
	virtual ~RLException();

protected:
	CStringA m_description;
};

#endif // !defined(RL_EXCEPTION_H__INCLUDED_)
