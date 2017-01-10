#include "stdafx.h"
#include "RLEvent.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


RLHandleWin::RLHandleWin()
{
	m_handle = NULL;
}

RLHandleWin::~RLHandleWin()
{
	Close();
}

void RLHandleWin::Close()
{
	if (m_handle!=NULL) {
		::CloseHandle(m_handle);
		m_handle = NULL;
	}
}



void RLEvent::Create(LPCTSTR lpName, SECURITY_ATTRIBUTES* sa)
{
	m_handle = ::CreateEvent(sa, FALSE, FALSE, lpName);	
	if (m_handle==NULL)
		throw RLException("ERROR %u CRLEvent::Create('%s')", ::GetLastError(), (lpName==NULL) ? "" : lpName);
}

void RLEvent::CreateOnly(LPCTSTR lpName)
{
	this->Create(lpName);

	if (::GetLastError()==ERROR_ALREADY_EXISTS) {
		this->Close();
		throw RLException("ERROR_ALREADY_EXISTS CRLEvent::CreateOnly('%s')", (lpName==NULL) ? "" : lpName);
	}
}

bool RLEvent::IsEventExist(LPCTSTR lpName)
{
	HANDLE hEvent = ::OpenEvent(EVENT_MODIFY_STATE, FALSE, lpName);

	if (hEvent==NULL) {
		DWORD dwError = ::GetLastError();
		if (dwError!=ERROR_FILE_NOT_FOUND) {
			throw RLException("ERROR: %d CRLEvent::IsEventExist()", dwError);
		}
		return false;
	}

	::CloseHandle(hEvent);
	return true;
}


void RLEvent::Set()
{
	BOOL v = ::SetEvent(m_handle);
}

void RLEvent::Reset()
{
	::ResetEvent(m_handle);
}

bool RLEvent::IsSet()
{
	DWORD v = ::WaitForSingleObject(m_handle, 0);

	if (v==WAIT_OBJECT_0) return true;
	if (v==WAIT_TIMEOUT)  return false;

	throw RLException("ERROR: CRLEvent::IsSet() %d", ::GetLastError());
}

HANDLE RLMutexWin::CreateAndOwn(LPSECURITY_ATTRIBUTES lpMutexAttributes, LPCSTR lpName)
{
	m_handle = ::CreateMutexA(lpMutexAttributes, FALSE, lpName);

	if (m_handle==NULL) return NULL;

	if (::WaitForSingleObject(m_handle, 0)!=WAIT_OBJECT_0) {
		::CloseHandle(m_handle);
		m_handle = NULL;
	}

	return m_handle;
}

bool RLMutexWin::IsExist(LPCTSTR lpName)
{
	HANDLE h = ::OpenMutex(MUTEX_MODIFY_STATE, FALSE, lpName);

	if (h==NULL) {
		DWORD dwError = ::GetLastError();
		if (dwError!=ERROR_FILE_NOT_FOUND) {
			throw RLException("ERROR: %d RLMutexWin::IsExist()", dwError);
		}
		return false;
	}

	::CloseHandle(h);
	return true;
}

