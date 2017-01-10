#include "stdafx.h"
#include "TSSessions.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

CTSSessions TSSessions;


// Windows XP (and later) functions used to handle session Ids
typedef DWORD (WINAPI *__WTSGetActiveConsoleSessionId) ();
typedef BOOL  (WINAPI *__ProcessIdToSessionId) (DWORD, DWORD*);



CTSSessions::CTSSessions()
{
	m_session = 0xFFFFFFFF;
	_WTSGetActiveConsoleSessionId = NULL;

	LPCSTR dllName = "kernel32.dll";

	HMODULE dllHandle = ::GetModuleHandle(dllName);
	if (!dllHandle) {
		return;
	}
	_WTSGetActiveConsoleSessionId	= ::GetProcAddress(dllHandle, "WTSGetActiveConsoleSessionId");
	__ProcessIdToSessionId _ProcessIdToSessionId = (__ProcessIdToSessionId)
									  ::GetProcAddress(dllHandle, "ProcessIdToSessionId");

	if (_ProcessIdToSessionId!=NULL) {
		if ((_ProcessIdToSessionId)(::GetCurrentProcessId(), &m_session)==0) {
			m_session = 0xFFFFFFFF;
		}
	}
}

DWORD CTSSessions::GetSessionId()
{
	return m_session;
}

DWORD CTSSessions::WTSGetActiveConsoleSessionId()
{
	if (_WTSGetActiveConsoleSessionId==NULL) return 0xFFFFFFFE;

	return ((__WTSGetActiveConsoleSessionId)_WTSGetActiveConsoleSessionId)();
}

bool CTSSessions::IsOutConsole()
{
	if (m_session==0xFFFFFFFF) {
		return false; // ProcessIdToSessionId() not supported
	}

	DWORD session = WTSGetActiveConsoleSessionId();

	if (session==0xFFFFFFFE) return false; // WTSGetActiveConsoleSessionId not supported
	if (session==0xFFFFFFFF) return false; // no active session

	return (session!=m_session);
}

/*
typedef BOOLEAN (WINAPI *__WinStationConnect) (HANDLE,ULONG,ULONG,PCWSTR,ULONG);
BOOL CTSSessions::SetConsoleSession(DWORD sessionId) 
{
	HMODULE dllHandle = ::LoadLibrary("winsta.dll");
	if (dllHandle==NULL) {
		_log.WriteError("ERROR: SetConsoleSession()#1");
		return FALSE;
	}

	__WinStationConnect _WinStationConnect = (__WinStationConnect)
									  ::GetProcAddress(dllHandle, "WinStationConnectW");

	if (_WinStationConnect==NULL) {
		_log.WriteError("ERROR: SetConsoleSession()#2");
		::FreeLibrary(dllHandle);
		return FALSE;
	}

	// Try to reconnect our session to the console session
	BOOL b = (*_WinStationConnect)(0, sessionId, m_session, L"", 0);
	if (!b) {
		_log.WriteError("ERROR: WinStationConnec() error=%d", ::GetLastError());
	}

	::FreeLibrary(dllHandle);

	if (b) {
		m_session = sessionId;
		return TRUE;
	}
	else
		return FALSE;

  // Lock the newly connected session, for security
  //if (_LockWorkStation.isValid())
  //  (*_LockWorkStation)();
}
*/

