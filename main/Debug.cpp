#include "stdafx.h"
#include "Debug.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

CDebug::CDebug()
{

}

CDebug::~CDebug()
{

}

/*
DWORD CDebug::GetProcessIdToSessionId(DWORD processId)
{
	DWORD sessionId;
	if (!::ProcessIdToSessionId(processId, &sessionId)) {
		//_log.WriteError("ERROR: ProcessIdToSessionId(%X) error=%d", processId, ::GetLastError());
		sessionId = 99999999;
	}
	return sessionId;
}
*/

/*
CStringA CDebug::GetInputDesktopName()
{
	HDESK inputdesktop = ::OpenInputDesktop(0, FALSE, 0);

	if (inputdesktop == NULL) {
		return "<NULL>";
	}
	
	char inputname[256];
	DWORD dummy;

	if (!GetUserObjectInformation(inputdesktop, UOI_NAME, &inputname, 256, &dummy)) {
		strcpy(inputname, "<ERROR#1>");
	}

	if (!::CloseDesktop(inputdesktop)) {
		_log.WriteError("ERROR: CloseDesktop() error=%d", ::GetLastError());
	}

	return inputname;
}
*/



/*
BOOL CALLBACK EnumDesktopProc(LPTSTR lpszDesktop, LPARAM lParam)
{
	_log.WriteError("    EnumDesktopProc - '%s'", lpszDesktop);
	return TRUE;
}

BOOL CALLBACK EnumWindowStationProc(  LPTSTR lpszWindowStation,   LPARAM lParam)
{
	_log.WriteError("EnumWindowStationProc - '%s'", lpszWindowStation);

	HWINSTA hwinsta = ::OpenWindowStation(lpszWindowStation, FALSE, WINSTA_ENUMDESKTOPS | WINSTA_ENUMERATE);

	if (hwinsta==NULL) {
		_log.WriteError("ERROR: OpenWindowStation(), error=%d", ::GetLastError());
		return TRUE;
	}

	if (!::EnumDesktops(hwinsta, EnumDesktopProc, 0)) {
		_log.WriteError("ERROR: EnumDesktopProc() error=%d", ::GetLastError());
	}

	if (!::CloseWindowStation(hwinsta)) {
		_log.WriteError("ERROR: CloseWindowStation() error=%d", ::GetLastError());
	}

	return TRUE;
}

void CDebug::EnumWindowStations()
{

	if (!::EnumWindowStations(EnumWindowStationProc, 0)) {
		_log.WriteError("ERROR: EnumWindowStations() error=%d", ::GetLastError());
	}

	HWINSTA hwinsta = ::GetProcessWindowStation();

	char name[128];

	::GetUserObjectInformation(hwinsta, UOI_NAME, name, 128, NULL);

	_log.WriteError("Current Win Station %X '%s'", hwinsta, name);
}
*/


/*
void CDebug::OpenWinStations()
{
	LPCSTR winsta = "\\Global\\Sessions\\0\\Windows\\WindowStations\\WinSta0";

	HWINSTA hwinsta = ::OpenWindowStation(winsta, FALSE, WINSTA_ENUMDESKTOPS | WINSTA_ENUMERATE);

	if (hwinsta==NULL) {
		_log.WriteError("ERROR: OpenWindowStation('%s'), error=%d", winsta, ::GetLastError());
		return;
	}

	if (!::EnumDesktops(hwinsta, EnumDesktopProc, 0)) {
		_log.WriteError("ERROR: EnumDesktopProc() error=%d", ::GetLastError());
	}

	if (!::CloseWindowStation(hwinsta)) {
		_log.WriteError("ERROR: CloseWindowStation() error=%d", ::GetLastError());
	}

	return;
}
*/

