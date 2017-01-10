#if !defined(RL_RLEH_H_INCLUDED_)
#define RL_RLEH_H_INCLUDED_

#include "RLException.h"
#include "RLLock.h"

class RLEH  
{
public:
	RLEH(const char* pFileName, DWORD dwLineNum);
#ifdef _WIN32
	static void Init(bool unhandled);
#endif

	void operator() () const;	
	void operator() (const char *pszFmt, ...) const;
	void operator() (RLException* ex) const;

	static CStringA m_strAppName;
	static bool     m_msgbox;

private:
	static void ShowError(const char* pFrmError, ...);
	static void OnErrorV(const char* pFileName, DWORD dwLineNum, const char* pFrmError, va_list arg);
#ifdef _WIN32
	static LONG WINAPI UnhandledExceptionFilter(struct _EXCEPTION_POINTERS* ExceptionInfo);
#endif
	static RLMutex m_lockOnErrorV;

	const char* m_pFileName;
	const DWORD m_dwLineNum;
};

#define RL_ERROR RLEH(__FILE__, __LINE__)

#define RL_ASSERT(f) \
	if (!(f)) { \
		RL_ERROR(); \
	} \

#endif // !defined(RL_RLEH_H_INCLUDED_)
