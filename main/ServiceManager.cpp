#include "stdafx.h"
#include "ServiceManager.h"
#include "Common.h"
#include "TSSessions.h"
#include "../RL/RLRegistry.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


CServiceManager::CServiceManager(LPCSTR lpServiceName, LPCSTR lpDisplayName)
{
	m_serviceName = lpServiceName;
	m_displayName = lpDisplayName;
}


void CServiceManager::GetStatus(bool& bInstall, bool& bStart, bool& bStop, bool& bRemove)
{
	bInstall = false;
	bStart = false;
	bStop = false;
	bRemove = false;


	SC_HANDLE  hsrvmanager = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (hsrvmanager==NULL) {
		return;
	}

	SC_HANDLE hservice = ::OpenService(hsrvmanager, m_serviceName, SERVICE_ALL_ACCESS);

	if (hservice != NULL)
	{
		bRemove = true;

		SERVICE_STATUS status;
		if (::QueryServiceStatus(hservice, &status)!=0) {
			if (status.dwCurrentState==SERVICE_RUNNING) {
				bStop = true;
			}
			if (status.dwCurrentState==SERVICE_STOPPED) {
				bStart = true;
			}
		}
		::CloseServiceHandle(hservice);
	}
	else {
		bInstall = true;
	}


	::CloseServiceHandle(hsrvmanager);
}


void CServiceManager::Install(SC_HANDLE hsrvmanager)
{
	CStringW path = CCommon::GetModuleFileNameW(NULL);

	// Append the service-start flag to the end of the path:
	CStringW serviceCmd = L"\"" + path + L"\" -service";
	serviceCmd += (LPCSTR)m_addArguments;

	CStringW serviceNameW = m_serviceName;
	CStringW displayNameW = m_displayName;

	// Create an entry for the Ammyy service
	SC_HANDLE hservice = ::CreateServiceW(
		hsrvmanager,				// SCManager database
		serviceNameW,				// name of service
		displayNameW,				// name to display
		SERVICE_ALL_ACCESS,			// desired access
		SERVICE_WIN32_OWN_PROCESS,  // service type
		SERVICE_AUTO_START,			// start type
		SERVICE_ERROR_NORMAL,		// error control type
		serviceCmd,					// service's binary
		NULL,						// no load ordering group
		NULL,						// no tag identifier
		NULL,						// dependencies
		NULL,						// LocalSystem account
		NULL);						// no password
	
	if (hservice == NULL)
	{
		DWORD error = ::GetLastError();

		if (error == ERROR_SERVICE_EXISTS) {
			throw RLException("The Ammyy service is already registered");
		} else {
			throw RLException("ERROR %d CreateServiceW() while registering the Ammyy service", error);
		}
	}
	::CloseServiceHandle(hservice);

	TurnSafeMode(true);

/*
	// Now install the servicehelper registry setting...
	// Locate the RunService registry entry
	HKEY runapps;
	if (::RegCreateKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", &runapps) != ERROR_SUCCESS)
	{		
		::throw RLException("Unable to install the ServiceHelper hook\nGlobal user-specific registry settings will not be loaded");		
	} else {
		// Append the service-helper-start flag to the end of the path:
		CStringA servicehelpercmd;
		servicehelpercmd.Format("\"%s\" %s", path, winvncRunServiceHelper);

		// Add the AmmyyHelper entry
		if (::RegSetValueEx(runapps, szAppName, 0, REG_SZ, (BYTE*)(LPCSTR)servicehelpercmd, servicehelpercmd.GetLength()+1) != ERROR_SUCCESS)
		{
			::RegCloseKey(runapps);
			::throw RLException("Unable to install the ServiceHelper hook\nGlobal user-specific registry settings will not be loaded");
			
		}
		::RegCloseKey(runapps);
	}
*/
}

void CServiceManager::TurnSafeMode(bool on)
{
	CStringA key = "SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Network\\" + m_serviceName;

	if (on)
	{
		RLRegistry registry;
		registry.CreateOrOpenA(HKEY_LOCAL_MACHINE, key);
		registry.SetStringValueA(NULL, CStringA("Service"));
	}
	else
	{
		RLRegistry::DeleteKeyA(HKEY_LOCAL_MACHINE, key);
	}
}

void CServiceManager::StopAndRemove(SC_HANDLE hsrvmanager, BOOL bRemove)
{
	SC_HANDLE hservice = ::OpenService(hsrvmanager, m_serviceName, SERVICE_ALL_ACCESS);

	if (hservice != NULL)
	{
		SERVICE_STATUS status;

		if (::QueryServiceStatus(hservice, &status)==0) {
			DWORD error = ::GetLastError();
			::CloseServiceHandle(hservice);
			throw RLException("ERROR %d QueryServiceStatus()", error);
		}
		
		if (status.dwCurrentState != SERVICE_STOPPED) {
			// Try to stop the Ammyy service
			if (::ControlService(hservice, SERVICE_CONTROL_STOP, &status))
			{
				while(::QueryServiceStatus(hservice, &status))
				{
					if (status.dwCurrentState == SERVICE_STOP_PENDING)
						Sleep(100);
					else
						break;
				}

				if (status.dwCurrentState != SERVICE_STOPPED) {
					::CloseServiceHandle(hservice);
					throw RLException("The Ammyy service could not be stopped");
				}
			}
			else {
				DWORD error = ::GetLastError();
				::CloseServiceHandle(hservice);
				throw RLException("ERROR %d ControlService()", error);
			}
		}

		if (bRemove) {
			// Now remove the service from the SCM
			if (::DeleteService(hservice)==0) 
			{
				DWORD error = ::GetLastError();
				::CloseServiceHandle(hservice);
				if (error == ERROR_SERVICE_MARKED_FOR_DELETE) {
					throw RLException("The Ammyy service is already marked for removing");
				} else {
					throw RLException("The Ammyy service could not be removed, error=%d", error);
				}
			}
			TurnSafeMode(false);
		}
		::CloseServiceHandle(hservice);
	}
	else {
		throw RLException("The Ammyy service could not be found");
	}


	// Attempt to remove the service-helper hook
	/*
	HKEY runapps;
	if (::RegOpenKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", &runapps) == ERROR_SUCCESS)
	{
		// Attempt to delete the WinVNC key
		if (::RegDeleteValue(runapps, szAppName) != ERROR_SUCCESS)
		{
			::RegCloseKey(runapps);
			throw RLException("The ServiceHelper hook entry could not be removed from the registry");
		}
		::RegCloseKey(runapps);
	}
	*/

}


void CServiceManager::Start(SC_HANDLE hsrvmanager)
{
	SC_HANDLE hservice = ::OpenService(hsrvmanager, m_serviceName, SERVICE_ALL_ACCESS);

	if (hservice == NULL) {
		throw RLException("The Ammyy service could not be found");
	}

	if (::StartService(hservice, 0, NULL)==0)
	{
		DWORD error = ::GetLastError();
		::CloseServiceHandle(hservice);
		throw RLException("ERROR %d while starting the AMMYY service", error);
	}
		
	::CloseServiceHandle(hservice);
}


void CServiceManager::RunCmd(HWND hWnd, WORD cmd)
{
	SC_HANDLE hsrvmanager = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	try {
		if (hsrvmanager==NULL)
			throw RLException("OpenSCManager() error %d", ::GetLastError());

		CStringA msg;

		if (cmd==0) {
			CServiceManager::Install(hsrvmanager);
			msg = "The " + m_displayName + " service was successfully registered\n""It will automatically be run the next time this computer is restart or you can start it manually.";
		}
		else if (cmd==1) {
			CServiceManager::Start(hsrvmanager);
			msg = "The " + m_displayName + " service has been started";
		}
		else if (cmd==2) {
			CServiceManager::StopAndRemove(hsrvmanager, FALSE);
			msg = "The " + m_displayName + " service has been stopped";
		}
		else if (cmd==3) {
			CServiceManager::StopAndRemove(hsrvmanager, TRUE);
			msg = "The " + m_displayName + " service has been removed";
		}

		::CloseServiceHandle(hsrvmanager);

		if (!msg.IsEmpty()) {
			if (hWnd!=(HWND)-1)
				::MessageBox(hWnd, (LPCSTR)msg, (LPCSTR)m_displayName, MB_ICONINFORMATION | MB_OK);
		}
	}
	catch(RLException& ex) {
		if (hsrvmanager!=NULL) ::CloseServiceHandle(hsrvmanager);
			if (hWnd!=(HWND)-1)
				::MessageBox(hWnd, ex.GetDescription(), m_displayName, MB_ICONEXCLAMATION | MB_OK);
			else
				_log.WriteError(ex.GetDescription());
	}
}


void CServiceManager::JustLunch(int sessionId)
{
	if (sessionId<0)
		throw RLException("JustLunch() Invalid Session");

	SC_HANDLE hsrvmanager = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (hsrvmanager==NULL)
		throw RLException("OpenSCManager() error %d", ::GetLastError());

	// check if service exist
	try {
		StopAndRemove(hsrvmanager, TRUE);
	}
	catch(RLException&) {}


	SC_HANDLE hservice = 0;

	try {
		CServiceManager::Install(hsrvmanager);

		hservice = ::OpenService(hsrvmanager, m_serviceName, SERVICE_ALL_ACCESS);

		if (hservice==NULL) 
			throw RLException("OpenService() error %d", ::GetLastError());

		char session[16];
		sprintf(session, "s%u", sessionId);
		LPCSTR lpSession = session;

		if (::StartService(hservice, 1, &lpSession)==0)
			throw RLException("StartService() error %d", ::GetLastError());

		while(true) {
			SERVICE_STATUS status;
			if (::QueryServiceStatus(hservice, &status)==0)
				throw RLException("QueryServiceStatus() error %d", ::GetLastError());

			if (status.dwCurrentState==SERVICE_STOPPED) break;
			::Sleep(10);
		}

		if (::DeleteService(hservice)==0)
			throw RLException("DeleteService() error %d", ::GetLastError());

		::CloseServiceHandle(hservice);
		::CloseServiceHandle(hsrvmanager);

		TurnSafeMode(false);
	}
	catch(RLException& ex) {
		if (hservice)    {
			::DeleteService(hservice);
			::CloseServiceHandle(hservice);
		}
		if (hsrvmanager) ::CloseServiceHandle(hsrvmanager);
		throw ex;
	}
}