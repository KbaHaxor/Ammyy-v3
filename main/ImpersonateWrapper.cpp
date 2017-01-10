#include "stdafx.h"
#include "ImpersonateWrapper.h"
#include "AmmyyApp.h"
#include "TSSessions.h"
#include <Tlhelp32.h>
#include <Userenv.h>
#pragma comment(lib, "Userenv.lib")


typedef BOOL  (WINAPI *__WTSQueryUserToken) (ULONG, PHANDLE);

class Impersonate
{
public:
	Impersonate();
	~Impersonate();
	bool DoImpersonate();

	HANDLE QueryUserToken();

private:
	LPVOID GetProc();
	static DWORD FindProcessByName(LPCSTR processName);

	HMODULE	m_hWtsapi32;
	LPVOID	m_pWTSQueryUserToken;
};

Impersonate::Impersonate()
{
	m_pWTSQueryUserToken = (LPVOID)-1;
	m_hWtsapi32 = NULL;
}

Impersonate::~Impersonate()
{
	if (m_hWtsapi32!=NULL) {
		if (::FreeLibrary(m_hWtsapi32)==0) {
			//_log2.Print(LL_ERR, VTCLOG("ERROR: FreeLibrary('Wtsapi32.dll') error=%d"), ::GetLastError());
		}
	}
}

LPVOID Impersonate::GetProc()
{
	if (m_pWTSQueryUserToken==(LPVOID)-1) {
		m_pWTSQueryUserToken = NULL;
	
		m_hWtsapi32 = ::LoadLibraryA("Wtsapi32.dll");
		if (m_hWtsapi32==NULL) {			
			_log2.Print(LL_WRN, VTCLOG("WARN: LoadLibrary('Wtsapi32.dll') error=%d"), ::GetLastError());
		}
		else {
			// can be failed for Windows 2000
			m_pWTSQueryUserToken = ::GetProcAddress(m_hWtsapi32, "WTSQueryUserToken");
			if (m_pWTSQueryUserToken==NULL) {
				_log2.Print(LL_WRN, VTCLOG("WARN: GetProcAddress('WTSQueryUserToken') error=%d"), ::GetLastError());
			}
		}
	}
	return m_pWTSQueryUserToken;
}

DWORD Impersonate::FindProcessByName(LPCSTR processName)
{
	// Take a snapshot of all processes in the system.
	HANDLE hProcessSnap = ::CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if( hProcessSnap == INVALID_HANDLE_VALUE ) {
		return 0;
	}

	// Set the size of the structure before using it.
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof( PROCESSENTRY32 );

	// Retrieve information about the first process, and exit if unsuccessful
	if( !::Process32First( hProcessSnap, &pe32 ) ) {		
		::CloseHandle( hProcessSnap );          // clean the snapshot object
		return 0;
	}

	DWORD processId = 0;

	// Now walk the snapshot of processes
	do
	{
		if (stricmp(pe32.szExeFile, processName)==0) {
			processId = pe32.th32ProcessID;;
			break;
		}

	} while( ::Process32Next( hProcessSnap, &pe32 ) );

	::CloseHandle(hProcessSnap);
	return processId;
}

HANDLE Impersonate::QueryUserToken()
{
	DWORD sessionId = TSSessions.GetSessionId();

	//_log.WriteInfo("QueryUserToken()#1 %X", sessionId);

	HANDLE userToken = 0;
	void* pWTSQueryUserToken = GetProc();

	if (sessionId!=0xFFFFFFFF && pWTSQueryUserToken!=NULL)
	{
		BOOL b = ((__WTSQueryUserToken)pWTSQueryUserToken)(sessionId, &userToken);

		if (!b) {
			_log2.Print(LL_ERR, VTCLOG("ERROR: WTSQueryUserToken() error=%d"), ::GetLastError());
			return 0;
		}

		//_log.WriteInfo("QueryUserToken()#2");
	}
	else
	{
		DWORD processId = FindProcessByName("explorer.exe");

		if (processId==0) {
			_log2.Print(LL_ERR, VTCLOG("ERROR: FindProcessByName('explorer.exe')"));
			return 0;
		}

		HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);

		if (hProcess==NULL) {
			_log2.Print(LL_ERR, VTCLOG("ERROR %d OpenProcess()"), ::GetLastError());
			return 0;
		}

		if (::OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &userToken)==0) {
			_log2.Print(LL_ERR, VTCLOG("ERROR %d OpenProcessToken()"), ::GetLastError());
			::CloseHandle(hProcess);
			return 0;
		}

		::CloseHandle(hProcess); // is userToken actual after it ? - yes
	}
	return userToken;
}


bool Impersonate::DoImpersonate()
{
	//_log2.Print(LL_INF, VTCLOG("Impersonate()#0"));

	if (!TheApp.m_systemUser) return false; // no need to impersonate

	HANDLE userToken = QueryUserToken();
	if (userToken==0) return false; // couldn't get user token

	bool ok = (::ImpersonateLoggedOnUser(userToken)!=0);

	if (!ok)
		_log2.Print(LL_ERR, VTCLOG("ERROR %d ImpersonateLoggedOnUser()"), ::GetLastError());

	if (::CloseHandle(userToken)==0)
		_log2.Print(LL_ERR, VTCLOG("ERROR %d CloseHandle()"), ::GetLastError());
	
	return ok;
}


//_________________________________________________________________________________________


Impersonate _Impersonate;

ImpersonateWrapper::ImpersonateWrapper()
{
	m_ok = _Impersonate.DoImpersonate();
}

ImpersonateWrapper::ImpersonateWrapper(bool b)
{
	m_ok = (b) ? _Impersonate.DoImpersonate() : false;
}

ImpersonateWrapper::~ImpersonateWrapper()
{
	if (m_ok) {
		if (::RevertToSelf()==0)
			_log2.Print(LL_ERR, VTCLOG("~ImpersonateWrapper()#1 error=%d"), ::GetLastError());
	}
}

LoadUserProfileWrapper::LoadUserProfileWrapper()
{
	m_hProfile = 0;
	m_hToken   = 0;

	if (!TheApp.m_systemUser) return; // no need to impersonate

	m_hToken = _Impersonate.QueryUserToken();
	if (m_hToken==0) return; // could get user token

	char username[128];
	{
		if (!::ImpersonateLoggedOnUser(m_hToken)) {
			_log2.Print(LL_ERR, VTCLOG("ERROR %d ImpersonateLoggedOnUser()"), ::GetLastError());
			return;
		}
		
		DWORD size = sizeof(username);
		bool ok =true;
		if (::GetUserName(username, &size)==0) {
			_log2.Print(LL_ERR, VTCLOG("ERROR %d GetUserName()"), ::GetLastError());
			ok = false;
		}

		if (::RevertToSelf()==0) {
			_log2.Print(LL_ERR, VTCLOG("ERROR %d RevertToSelf()"), ::GetLastError());
		}

		if (!ok) return;
	}

	//_log.WriteInfo("LoadUserProfileWrapper() user=%s", username);
	
	PROFILEINFO profileInfo = {0};
	profileInfo.dwSize = sizeof(profileInfo);
	profileInfo.dwFlags = PI_NOUI;
	profileInfo.lpUserName = username;

	if (::LoadUserProfile(m_hToken, &profileInfo)==FALSE) {
		_log2.Print(LL_ERR, VTCLOG("ERROR %d LoadUserProfile()"), ::GetLastError());
		return;
	}

	if (profileInfo.hProfile==0 || profileInfo.hProfile==(HANDLE)-1) {
		_log2.Print(LL_ERR, VTCLOG("Invalid profile=%X"), profileInfo.hProfile);
	}
	else
		m_hProfile = profileInfo.hProfile;
}


LoadUserProfileWrapper::~LoadUserProfileWrapper()
{
	if (m_hProfile) {
		if (::UnloadUserProfile(m_hToken, m_hProfile)==0)
			_log2.Print(LL_ERR, VTCLOG("ERROR %d UnloadUserProfile()"), ::GetLastError());		
	}

	if (m_hToken) {
		if (::CloseHandle(m_hToken)==0)
			_log2.Print(LL_ERR, VTCLOG("ERROR %d CloseHandle()"), ::GetLastError());
	}
}
