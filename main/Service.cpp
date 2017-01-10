#include "stdafx.h"
#include "AmmyyApp.h"
#include "Service.h"
#include "ServerInteract.h"
#include "Debug.h"
#include "Common.h"
#include "TSSessions.h"
#include "../RL/RLEvent.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#pragma comment(lib, "Advapi32.lib")



CService Service;


CService::CService()
{
	m_hstatus = 0;
	m_dwWorkThreadId = 0;
	memset(&m_srvstatus, 0, sizeof(m_srvstatus));	
}

CService::~CService()
{
}


void CService::WinMain()
{
	_log.WriteInfo("CService::WinMain()#begin");

	// Create a service entry table
	SERVICE_TABLE_ENTRY dispatchTable[] =
    {
		{AMMYYSERVICENAME, (LPSERVICE_MAIN_FUNCTION)CService::ServiceMain},
		{NULL, NULL}
	};

	// Call the service control dispatcher with our entry table
	if (!::StartServiceCtrlDispatcher(dispatchTable)) {
		_log.WriteError("ERROR: StartServiceCtrlDispatcher() %d", ::GetLastError());
		//LogErrorMsg("StartServiceCtrlDispatcher failed.");
	}
}


// SERVICE MAIN ROUTINE
void CService::ServiceMain(DWORD argc, char**argv)
{
	// Register the service control handler
    Service.m_hstatus = ::RegisterServiceCtrlHandlerEx(AMMYYSERVICENAME, ServiceHandlerProc, (void*)33);

    if (Service.m_hstatus == 0)
		return;

	// Set up some standard service state values
    Service.m_srvstatus.dwServiceType = SERVICE_WIN32;
    Service.m_srvstatus.dwServiceSpecificExitCode = 0;

	// Give this status to the SCM
    if (!Service.ReportStatus(SERVICE_START_PENDING, NO_ERROR, 15000))
	{
        Service.ReportStatus(SERVICE_STOPPED, 0, 0);
		return;
	}

	// TODO: just for testing
	//CService::CreateProcess1(1, L"WinSta0\\Default", "cmd.exe");

	if (TheApp.m_CmgArgs.lunch) {
		int session=-1;

		if (argc>=2) {
			if (argv[1][0] == 's') {
				session = atoi(argv[1]+1);
			}
		}

		if (session<0) {
			_log.WriteError("ServiceMain() Invalid session");
		}
		else {
			CStringW path = CCommon::WrapMarks(CCommon::GetModuleFileNameW(0));
			CService::CreateProcess1(session, L"WinSta0\\Default", (LPCWSTR)path);
		}
		Service.ReportStatus(SERVICE_STOPPED, 0, 0);
		return;
	}

	// Now start the service for real
	HANDLE hThread = ::CreateThread(NULL, 0, CService::ServiceWorkThread, 0, 0, NULL);
	::CloseHandle(hThread);
    return;
}


// Service control routine
DWORD CService::ServiceHandlerProc(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
    switch(dwControl)
    {
	case SERVICE_CONTROL_STOP:
		// STOP : The service must stop
		Service.m_srvstatus.dwCurrentState = SERVICE_STOP_PENDING;
        Service.ServiceStop();
        break;

    case SERVICE_CONTROL_INTERROGATE:
		// QUERY : Service control manager just wants to know our state
		break;

	default:
		// Control code not recognised
		break;

    }

	// Tell the control manager what we're up to.
	Service.ReportStatus(Service.m_srvstatus.dwCurrentState, NO_ERROR, 0);

	return NO_ERROR;
}


void CService::ServiceStop()
{
	// Post a quit message to the main service thread
	if (m_dwWorkThreadId != 0)
	{
		//_log.Print(LL_INF, VNCLOG("quitting from ServiceStop"));
		::PostThreadMessage(m_dwWorkThreadId, WM_QUIT, 0, 0);
	}
}


// Service manager status reporting
// waithint - hint as to how long Ammyy should have hung before you assume error
//
BOOL CService::ReportStatus(DWORD state, DWORD exitcode, DWORD waithint)
{
	static DWORD checkpoint = 1;
	BOOL result = TRUE;

	// If we're in the start state then we don't want the control manager
	// sending us control messages because they'll confuse us.
    if (state == SERVICE_START_PENDING)
		m_srvstatus.dwControlsAccepted = 0;
	else
		m_srvstatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	//m_srvstatus.dwControlsAccepted += SERVICE_ACCEPT_SESSIONCHANGE; // support OS v5.1 or higher

	// Save the new status we've been given
	m_srvstatus.dwCurrentState = state;
	m_srvstatus.dwWin32ExitCode = exitcode;
	m_srvstatus.dwWaitHint = waithint;

	// Update the checkpoint variable to let the SCM know that we
	// haven't died if requests take a long time
	if ((state == SERVICE_RUNNING) || (state == SERVICE_STOPPED))
		m_srvstatus.dwCheckPoint = 0;
	else
        m_srvstatus.dwCheckPoint = checkpoint++;

	// Tell the SCM our new status
	if (!(result = ::SetServiceStatus(m_hstatus, &m_srvstatus))) {
		_log.WriteError("ERROR %d SetServiceStatus()", ::GetLastError());
		//LogErrorMsg("SetServiceStatus failed");
	}

    return result;
}


DWORD CService::ServiceWorkThread(LPVOID)
{
	try 
	{
		_log.WriteInfo("CService::ServiceWorkThread()#1");

		// Save the current thread identifier
		Service.m_dwWorkThreadId = ::GetCurrentThreadId();

		// report the status to the service control manager.
		if (!Service.ReportStatus(SERVICE_RUNNING, NO_ERROR, 0))
			return 0;


		RLEvent event;
		event.Create(AMMYY_SERVICE_FINISH_EVENT);

		//_log.WriteInfo("CService::ServiceWorkThread() %d", TSSessions.WTSGetActiveConsoleSessionId());
	
		::SetTimer(NULL, 1234, 1000, NULL);

		CStringW commandLine = CCommon::WrapMarks(CCommon::GetModuleFileNameW(NULL)) + L" -nogui";

		if (TheApp.m_CmgArgs.log)			commandLine += L" -log";
		if (TheApp.m_CmgArgs.debug)			commandLine += L" -debug";

		HANDLE hProcess = INVALID_HANDLE_VALUE;

		MSG msg;
		while (::GetMessage(&msg, NULL, 0,0) )
		{
			if (msg.message == WM_TIMER) 
			{
				//DWORD activeSessionId = TSSessions.WTSGetActiveConsoleSessionId();
				//CStringA inputDesktop = CDebug::GetInputDesktopName();			
				//_log.WriteInfo("Desktop '%s' %d", (LPCSTR)inputDesktop, activeSessionId);

				{
					if (hProcess!=INVALID_HANDLE_VALUE) {
						if (IsProcessRunning(hProcess)) continue; // will wait until it's finished
						if (::CloseHandle(hProcess)==0) {
							_log.WriteError("ERROR: CloseHandle()#533 %d", ::GetLastError());
						}
						hProcess = INVALID_HANDLE_VALUE;
					}

					try {
						DWORD activeSessionId = TSSessions.WTSGetActiveConsoleSessionId();
						if (activeSessionId==0xFFFFFFFF) continue; //no active session

						::Sleep(100); // may be it help to prevent sometimes error 2, 183 or 233

						_log.WriteInfo("ServiceWorkThread() creating new process, session=%d", (int)activeSessionId);
												
						// if desktop is empty LoadLibrary('target.dll') error=998
						hProcess = Service.CreateProcess1(activeSessionId, L"WinSta0\\Winlogon", commandLine);
					}
					catch(RLException& ex) {
						_log.WriteError(ex.GetDescription());
					}
				}
			}
		}

		_log.WriteInfo("CService::ServiceWorkThread()#10");

		// before exit terminate child process
		//
		if (hProcess!=INVALID_HANDLE_VALUE) 
		{
			// child process close self if this event is exist
			if (!RLEvent::IsEventExist(AMMYY_SERVICE_CLOSE_SELF_EVENT)) {
				event.Set();
				_log.WriteInfo("CService::ServiceWorkThread()#11");
				::WaitForSingleObject(hProcess, INFINITE);
				_log.WriteInfo("CService::ServiceWorkThread()#12");
			}
			if (::CloseHandle(hProcess)==0) {
				_log.WriteError("ERROR: CloseHandle()#533 %d", ::GetLastError());
			}
			hProcess = INVALID_HANDLE_VALUE;
		}
	}
	catch(RLException& ex) {
		_log.WriteError(ex.GetDescription());
	}

	_log.WriteInfo("CService::ServiceWorkThread()#13");

	// Mark that we're no longer running
	Service.m_dwWorkThreadId = 0;

	// Tell the service manager that we've stopped.
    Service.ReportStatus(SERVICE_STOPPED, 0, 0);

	return 0;
}


bool CService::IsProcessRunning(HANDLE hProcess)
{
	DWORD exitCode = 0;
	if(::GetExitCodeProcess(hProcess, &exitCode) == 0) {
		throw RLException("ERROR: GetExitCodeProcess() %d", ::GetLastError());
	}

	return (exitCode == STILL_ACTIVE);
}

DWORD CService::MarshallString(LPCWSTR pszText, RLStream& stream)
{         
	if(!pszText) return 0;
	DWORD dwOffset = stream.GetLen();
	stream.AddRaw(pszText, (wcslen(pszText)+1)*sizeof(WCHAR));
	return dwOffset;
} 


BOOL CService::CreateRemoteSessionProcessW(
        DWORD        dwSessionId, 
        BOOL         bUseDefaultToken, 
        HANDLE       hToken, 
        LPCWSTR      lpApplicationName, 
        LPWSTR       lpCommandLine, 
        LPSECURITY_ATTRIBUTES lpProcessAttributes, 
        LPSECURITY_ATTRIBUTES lpThreadAttributes, 
        BOOL bInheritHandles, 
        DWORD dwCreationFlags, 
        LPVOID lpEnvironment, 
        LPCWSTR lpCurrentDirectory, 
        LPSTARTUPINFOW lpStartupInfo, 
        LPPROCESS_INFORMATION lpProcessInformation)
{ 
	typedef BOOLEAN (WINAPI* __WinStationQueryInformationW)( 
		IN   HANDLE hServer, 
		IN   ULONG LogonId, 
		IN   DWORD /*WINSTATIONINFOCLASS*/ WinStationInformationClass, 
		OUT  PVOID pWinStationInformation, 
		IN   ULONG WinStationInformationLength, 
		OUT  PULONG pReturnLength 
	);

	struct CPAU_RET_PARAM {
        DWORD   cbSize; 
        BOOL    bRetValue; 
        DWORD   dwLastErr; 
        PROCESS_INFORMATION     ProcInfo; 
	} cpauRetData;

	struct CPAU_PARAM{ 
        DWORD   cbSize;
        DWORD   dwProcessId;
        BOOL    bUseDefaultToken;
        HANDLE  hToken;
        LPWSTR  lpApplicationName;
        LPWSTR  lpCommandLine;
        SECURITY_ATTRIBUTES ProcessAttributes;
        SECURITY_ATTRIBUTES ThreadAttributes;
        BOOL bInheritHandles;
        DWORD dwCreationFlags;
        LPVOID lpEnvironment;
        LPWSTR lpCurrentDirectory;
        STARTUPINFOW StartupInfo;
        PROCESS_INFORMATION     ProcessInformation;
	};
	
    WCHAR           szNamedPipeName[MAX_PATH]=L"";
    HANDLE          hNamedPipe;
    BOOL            bRet = FALSE;

	// get szNamedPipeName
	{
		WCHAR	szWinStaPath[MAX_PATH];
		::GetSystemDirectoryW(szWinStaPath, MAX_PATH);
		lstrcatW(szWinStaPath,L"\\winsta.dll");
    
		HINSTANCE hInstWinSta = ::LoadLibraryW(szWinStaPath);

		if(hInstWinSta)
		{ 
			__WinStationQueryInformationW f = (__WinStationQueryInformationW)::GetProcAddress(hInstWinSta, "WinStationQueryInformationW");
			if(f) {
				DWORD dwNameLen;
				if (!f(0, dwSessionId, 0x21, szNamedPipeName, sizeof(szNamedPipeName), &dwNameLen)) szNamedPipeName[0]='\0';
			} 
			::FreeLibrary(hInstWinSta);
		}
	}
    
	if(szNamedPipeName[0] == '\0') { 
		swprintf(szNamedPipeName, L"\\\\.\\Pipe\\TerminalServer\\SystemExecSrvr\\%d", dwSessionId);
    } 

	while(true) { 
		hNamedPipe = ::CreateFileW(szNamedPipeName, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
		if(hNamedPipe != INVALID_HANDLE_VALUE) break;

		if(::GetLastError() == ERROR_PIPE_BUSY) { 
			if(!::WaitNamedPipeW(szNamedPipeName, 30000)) return FALSE; 
        }
        else { 
			return FALSE;
        }		
    }

    CPAU_PARAM cpauData;

	RLStream data(1024);

	data.AddRaw(NULL, sizeof(cpauData));

	memset(&cpauData, 0, sizeof(cpauData));
	cpauData.bInheritHandles        = bInheritHandles; 
	cpauData.bUseDefaultToken       = bUseDefaultToken;
	cpauData.dwCreationFlags        = dwCreationFlags;
	cpauData.dwProcessId            = ::GetCurrentProcessId();
	cpauData.hToken                 = hToken; 
	cpauData.lpApplicationName      = (LPWSTR)MarshallString(lpApplicationName, data);
	cpauData.lpCommandLine          = (LPWSTR)MarshallString(lpCommandLine, data);
	cpauData.StartupInfo            = *lpStartupInfo;
	cpauData.StartupInfo.lpDesktop  = (LPWSTR)MarshallString(cpauData.StartupInfo.lpDesktop, data);
	cpauData.StartupInfo.lpTitle    = (LPWSTR)MarshallString(cpauData.StartupInfo.lpTitle, data);	

	/*
    if(lpEnvironment) 
    {
		DWORD dwEnvLen = 0;
		
		if(dwCreationFlags & CREATE_UNICODE_ENVIRONMENT) 
        { 
			while(true)
			{ 
				if(((LPWSTR)lpEnvironment)[dwEnvLen/2]=='\0' && ((LPWSTR)lpEnvironment)[dwEnvLen/2+1] == '\0') { 
					dwEnvLen+=2*sizeof(WCHAR);
					break;
				} 
				dwEnvLen+=sizeof(WCHAR); 
			} 
		} 
		else 
		{ 
			while(true)
			{ 
				if(((LPSTR)lpEnvironment)[dwEnvLen]=='\0' && ((LPSTR)lpEnvironment) [dwEnvLen+1]=='\0') {
					dwEnvLen+=2;
                    break;
                } 
                dwEnvLen++; 
            } 
		} 

		DWORD offset = data.GetLen();
		data.AddRaw(lpEnvironment, dwEnvLen);        
		cpauData.lpEnvironment = (LPVOID)offset;
	} 
	else
	*/
	{ 
		cpauData.lpEnvironment  = NULL;
	} 
	
	cpauData.cbSize  = data.GetLen();

	memcpy(data.GetBuffer(), &cpauData, sizeof(cpauData));

	DWORD cbBytes;

	if(::WriteFile(hNamedPipe, data.GetBuffer(), data.GetLen(), &cbBytes, NULL) && 
		::ReadFile(hNamedPipe, &cpauRetData, sizeof(cpauRetData), &cbBytes, NULL))
	{ 
		bRet = cpauRetData.bRetValue;
		if(bRet) {
			*lpProcessInformation = cpauRetData.ProcInfo; 
		} 
		else {
			::SetLastError(cpauRetData.dwLastErr); 
		}
	} 
	else 
		bRet = FALSE; 


	::CloseHandle(hNamedPipe);
	return bRet;
} 



HANDLE CService::GetToken(ULONG sessionId)
{
	HANDLE hProcessToken;

	if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ALL_ACCESS|TOKEN_ADJUST_SESSIONID, &hProcessToken)) {
		throw RLException("ERROR: OpenProcessToken() error=%d", ::GetLastError());
	}

	HANDLE hNewToken=NULL;
	
	if(!::DuplicateTokenEx(hProcessToken, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, &hNewToken)) {
		DWORD dwError = ::GetLastError();
		::CloseHandle(hProcessToken);
		throw RLException("ERROR: DuplicateToken() error=%d", dwError);
	}

	::CloseHandle(hProcessToken);

	if (hNewToken==NULL || hNewToken==INVALID_HANDLE_VALUE)
		throw RLException("ERROR: GetToken()#2");

	if (sessionId!=0xFFFFFFFE)	// if supported sessions on this OS
	{		
		if (!::SetTokenInformation(hNewToken, TokenSessionId, &sessionId, sizeof(sessionId))) {
			DWORD dwError = ::GetLastError();
			::CloseHandle(hNewToken);
			throw RLException("ERROR: SetTokenInformation() error=%d", dwError);
		}		
	}

	return hNewToken;
}


HANDLE CService::CreateProcess1(DWORD sessionId, LPWSTR lpDesktop, LPCWSTR lpCommandLine)
{
	CStringW str = lpCommandLine; // function can change memory

	PROCESS_INFORMATION pi;
	STARTUPINFOW        si;

	ZeroMemory(&pi, sizeof(pi)); // not nesassary
	ZeroMemory(&si, sizeof(si));
	si.cb        = sizeof(si);
	si.lpDesktop = lpDesktop;

	BOOL bResult = FALSE;
	DWORD dwError=0;

	bool customCreate = (TheApp.m_dwOSVersion==0x50001 && sessionId>0); // if XP

	_log.WriteInfo("CreateProcess1()#1 session=%d custom=%d", sessionId, (int)customCreate);

	if (customCreate) 
	{
		bResult = CreateRemoteSessionProcessW(sessionId, TRUE, NULL, NULL, (LPWSTR)(LPCWSTR)str, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
		
		if (!bResult) dwError=::GetLastError();

		if (bResult && pi.hProcess==0) {
			pi.hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pi.dwProcessId);
			if (!pi.hProcess)
				throw RLException("ERROR: OpenProcess(%u) error=%d", pi.dwProcessId, ::GetLastError());
		}
	}	
	else {
		HANDLE hToken = GetToken(sessionId);

		bResult = ::CreateProcessAsUserW(hToken, NULL, str.GetBuffer(0), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
		if (!bResult) dwError=::GetLastError();

		if (::CloseHandle(hToken)==0)
			_log.WriteError("ERROR: CloseHandle(hToken) %d", ::GetLastError());
	}	
	
	_log.WriteInfo("CreateProcess1()#3 %d error=%d", (int)bResult, dwError);

	if (!bResult)
		throw RLException("ERROR: CreateProcessAsUser() error=%d, session=%d", dwError, sessionId);

	// no need it
	if (pi.hThread!=INVALID_HANDLE_VALUE && pi.hThread!=NULL) {
		if (::CloseHandle(pi.hThread)==0)
			_log.WriteError("ERROR: CloseHandle(pi.hThread) %d", ::GetLastError());
	}

	return pi.hProcess;
}