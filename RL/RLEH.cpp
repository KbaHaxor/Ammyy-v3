#include "stdafx.h"
#include "RLEH.h"
#include "RLStream.h"
#include "RLLog.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


CStringA RLEH::m_strAppName = "Non-def App";
bool     RLEH::m_msgbox   = false;
RLMutex  RLEH::m_lockOnErrorV;


RLEH::RLEH(const char* pFileName, DWORD dwLineNum):
m_pFileName(pFileName),m_dwLineNum(dwLineNum) 
{
	#ifdef RLEH_SOURCE_BASE_FOLDER
		int base_folder_len = sizeof(RLEH_SOURCE_BASE_FOLDER)-1;
		if (base_folder_len>0) {
			if (memcmp(RLEH_SOURCE_BASE_FOLDER, m_pFileName, base_folder_len)==0) {
				m_pFileName += base_folder_len;
			}
		}
	#endif
}


void RLEH::OnErrorV(const char* pFileName, DWORD dwLineNum, const char* pFrmError, va_list arg)
{
	RLMutexLock l(m_lockOnErrorV);

	CStringA descr;

	if      (pFrmError)	descr.FormatV(pFrmError, arg);
	else if (pFileName) descr.Format("Error in '%s' #%d", pFileName, dwLineNum);
	else				descr = "Not defineded error!!!";

	_log.WriteError(descr);

#ifdef _WIN32
	if (m_msgbox) ::MessageBoxA(NULL, (LPCSTR)descr, (LPCSTR)m_strAppName, MB_ICONERROR);
#endif
}


 
// old function: not use it, better to use macro RL_ERROR , because it fills m_pFileName & m_dwLineNum
//
void RLEH::ShowError(const char* pFrmError, ...)
{
	va_list arg;
	va_start(arg, pFrmError);
	OnErrorV(NULL, 0, pFrmError, arg);	
	va_end(arg);
}


void RLEH::operator() () const 
{
	OnErrorV(m_pFileName, m_dwLineNum, NULL, NULL);
}

void RLEH::operator() (const char* pFrmError, ...) const
{
	va_list arg;
	va_start(arg, pFrmError);
	OnErrorV(m_pFileName, m_dwLineNum, pFrmError, arg);
	va_end(arg);
}

void RLEH::operator() (RLException* ex) const
{
	OnErrorV(m_pFileName, m_dwLineNum, ex->GetDescription(), NULL);
}


#ifdef _WIN32

void RLEH::Init(bool unhandled)
{
	if (unhandled) {
		::SetUnhandledExceptionFilter(RLEH::UnhandledExceptionFilter);
	}
}

LONG WINAPI RLEH::UnhandledExceptionFilter(struct _EXCEPTION_POINTERS* ExceptionInfo)
{
	DWORD Eip = 0;
	DWORD Esp = 0;
	DWORD Ebp = 0;
	
	if (ExceptionInfo!=NULL) {
		if (ExceptionInfo->ContextRecord!=NULL) {
			Eip = ExceptionInfo->ContextRecord->Eip;
			Esp = ExceptionInfo->ContextRecord->Esp;
			Ebp = ExceptionInfo->ContextRecord->Ebp;
		}
	}

	ShowError("ERROR: Unhandled Exception eip=%.8X esp=%.8X ebp=%.8X", Eip, Esp, Ebp);

	::ExitProcess(0);

	return EXCEPTION_CONTINUE_EXECUTION;
}
#endif
