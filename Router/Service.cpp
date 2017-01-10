#include "stdafx.h"
#include "Service.h"
#include "CRouter.h"
#include "../RL/RLEvent.h"


#pragma comment(lib, "Advapi32.lib")



CService::CService(LPCSTR lpServiceName)
{
	m_hstatus = 0;
	memset(&m_srvstatus, 0, sizeof(m_srvstatus));
	m_serviceName = lpServiceName;
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
		{(LPSTR)Service.m_serviceName, (LPSERVICE_MAIN_FUNCTION)CService::ServiceMain},
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
    Service.m_hstatus = ::RegisterServiceCtrlHandlerEx(Service.m_serviceName, ServiceHandlerProc, (void*)33);

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

	Service.OnStart();
    return;
}


// Service control routine
DWORD CService::ServiceHandlerProc(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
    switch(dwControl)
    {
	case SERVICE_CONTROL_STOP:
		// STOP : The service must stop		
        Service.OnStop();
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

void CService::OnStart()
{
	try {
		Router.Start();

		this->ReportStatus(SERVICE_RUNNING, NO_ERROR, 0);
	}
	catch(RLException& ex) {
		_log.WriteError(ex.GetDescription());

		this->ReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
	}
}


void CService::OnStop()
{
	try {
		Router.Stop();
	}
	catch(RLException& ex) {
		_log.WriteError(ex.GetDescription());
	}

	this->m_srvstatus.dwCurrentState = SERVICE_STOPPED;
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








