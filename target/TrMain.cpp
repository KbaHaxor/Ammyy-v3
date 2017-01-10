#include "stdafx.h"
#include "TrMain.h"
#include "TrClient.h"
#include "TrService.h"
#include "../main/AmmyyApp.h"
#include "../main/TSSessions.h"
#include "../main/Common.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

static LPCSTR IncomePortMutex="Global\\Ammyy.Target.IncomePort";


TrMain _TrMain;

TrMain::TrMain()
{
	m_status = STATUS::Stopped;
	m_openedIncomePort = false;
	m_eventRemovedClient = FALSE;
}


void TrMain::OnTimer1()
{
	if (!m_mutexIncomePort.IsValid()) {
		if (!settings.m_allowIncomingByIP) return; // off by settings
		m_mutexIncomePort.CreateAndOwn(&m_saPublic, IncomePortMutex);

		if (!m_mutexIncomePort.IsValid()) return; // we couldn't take ownership of mutex
	}

	if (!m_openedIncomePort) {
		if (CCommon::TimeIsReady(m_dwTicksNextTryListen)) {
			try {
				m_listener.OpenPort(DEFAULT_INCOME_PORT);  //TODO: take from settings
				m_openedIncomePort = true;
			}
			catch(RLException& ) {
				m_dwTicksNextTryListen = ::GetTickCount() + 2000; // in 2 seconds
				throw;
			}
		}
	}

	m_listener.OnTimer();	
}

bool TrMain::IsAllowedNewAuthorized()
{
	int count = 0;

	RLMutexLock l2(m_clients_lock);

	ClientsEnumReset();
	while(true)
	{
		TrClient* client = ClientsEnumNext();
		if (client==(TrClient*)-1) break;
		if (client==NULL) continue;
		if (client->m_state==InteropCommon::STATE::OPERATOR_AUTHORIZED) count++;
	}

	return (count<settings.GetMaxSessionsOnTarget());
}


bool TrMain::OnTimer()
{
	RLMutexLock l(m_start_stop_lock);

	if (m_status==STATUS::Stopping) {
		if (!IsExistClients()) {
			m_status = STATUS::Stopped;
			InteropCommon::SendInfo("Stopped");
		}
		return true;
	}

	if (m_status!=STATUS::Started) return true;

	try {

		MSG msg;
		::PeekMessage(&msg, m_hwnd, 0, 0, PM_REMOVE);
		if (m_terminateReason == aaErrorSessionEnd) return false;

		OnTimer1();
	}
	catch(RLException& ex) {
		InteropCommon::ShowError(ex.GetDescription());
	}

	if (TheApp.m_ID==0) return true; // no need to connect to router if we haven't ID

	int  countAuthorized = 0;
	int  countConnectedUsingRouter = 0;

	{
		RLMutexLock l2(m_clients_lock);

		ClientsEnumReset();
		while(true)
		{
			TrClient* p = ClientsEnumNext();
			if (p==(TrClient*)-1) break;
			if (p==NULL) continue;

			if (!p->m_LAN) {
				// we have connection waiting on Router, so NO NEED new connection
				if (p->m_state<InteropCommon::STATE::OPERATOR_CONNECTED) return true;
				if (!p->m_transport->m_direct) countConnectedUsingRouter++;
			}

			if (p->m_state==InteropCommon::STATE::OPERATOR_AUTHORIZED) countAuthorized++;
		}
	}

	// we need at least 1 connection to Router, to Router says "Computer is busy"
	//	
	if (countConnectedUsingRouter==0 || countAuthorized<settings.GetMaxSessionsOnTarget())
	{
		TrClient* newClient = this->AddClient();
		if (newClient) {
			newClient->Thread01_start();
		}
	}
	return true;
}


// Try to allocate a client id...
//
TrClient* TrMain::AddClient()
{
	RLMutexLock l(m_clients_lock);

	int n = m_clients.GetLen()/4;

	if (n>=512) throw RLException("too many clients already connected");

	TrClient* pNewClient = new TrClient();

	TrClient** pClients = (TrClient**)m_clients.GetBuffer();

	int id=0;

	for (; id<n ;id++)
	{
		if (pClients[id]==NULL) {
			pNewClient->m_id = id;
			pClients[id] = pNewClient;
			return pNewClient;
		}
	}

	// extend m_clients	
	pNewClient->m_id = id;
	m_clients.AddRaw(&pNewClient, 4);
	return pNewClient;
}


void TrMain::KillClients(UINT16 reason)
{	
	RLMutexLock l(m_clients_lock);

	ClientsEnumReset();
	while(true)
	{
		TrClient* client = ClientsEnumNext();
		if (client==(TrClient*)-1) break;
		if (client==NULL) continue;

		_log2.Print(LL_INF, VTCLOG("killing client"));
		client->KillClient(reason); // Kill the client
	}

	_log2.Print(LL_INF, VTCLOG("KillClients(%d) done"), (int)reason);
}


bool TrMain::IsExistClients()
{
	RLMutexLock l(m_clients_lock);

	ClientsEnumReset();
	while(true)
	{
		TrClient* client = ClientsEnumNext();
		if (client==(TrClient*)-1) break;
		if (client!=NULL) return true;
	}

	return false;
}

bool TrMain::IsRDPClients()
{
	RLMutexLock l(m_clients_lock);

	ClientsEnumReset();
	while(true)
	{
		TrClient* client = ClientsEnumNext();
		if (client==(TrClient*)-1) break;
		if (client==NULL) continue;

		if (client->m_rdp) return true;
	}

	return false;
}

UINT TrMain::GetStateLocalWAN()
{
	RLMutexLock l(m_clients_lock);

	UINT status = InteropCommon::STATE::OFF;

	ClientsEnumReset();
	while(true)
	{
		TrClient* client = ClientsEnumNext();
		if (client==(TrClient*)-1) break;
		if (client==NULL) continue;

		UINT s = client->m_state;
		if (s>status) status=s;
	}

	return status;
}

// Wait for all the clients to exit
//
void TrMain::WaitUntilNoClients()
{
	while (IsExistClients()) { ::Sleep(2); }	
}


// RemoveClient should ONLY EVER be used by the client to remove itself.
void TrMain::RemoveClient(TrClient* client)
{
	{
		RLMutexLock l(m_clients_lock);

		TrClientId id = client->m_id;

		//ASSERT(id<m_clients.GetLen()/4);

		TrClient** pClients = (TrClient**)m_clients.GetBuffer();

		if (pClients[id]!=client)
			throw RLException("in RemoveClient()#1");
		
		pClients[id] = NULL;
	}

	_log2.Print(LL_INF, VTCLOG("deleteing client object from memory"));
	delete client;

	m_eventRemovedClient = TRUE;

	_log2.Print(LL_INF, VTCLOG("RemoveClient() done"));
}



// __________________________________________________________________________________________________


bool TrMain::IsOutConsole()
{
	if ((!TheApp.m_CmgArgs.noGUI)) return false;
	return TSSessions.IsOutConsole();
}


UINT TrMain::GetStateTotalWAN()
{
	char name[64];

	for (int i=InteropCommon::STATE::OPERATOR_AUTHORIZED; i>0; i--) {
		sprintf(name, "Global\\Ammyy.Target.StateEvent_%d_", i);
		if (RLEvent::IsEventExist(name)) return i;
	}

	return InteropCommon::STATE::OFF;
}

void TrMain::GetStateIncomePort(bool& total, bool& local)
{
	total = RLMutexWin::IsExist(IncomePortMutex);
	local = m_openedIncomePort;
}

static LRESULT CALLBACK WndProcDef(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	if (iMsg==WM_ENDSESSION)
	{
		_log2.Print(LL_INF, VTCLOG("WM_ENDSESSION wParam=%X lParam=%X"), wParam, lParam);
		_TrMain.Stop(true, aaErrorSessionEnd); // calls KillClients
		
		return 0;
	}
/*
	case WM_WTSSESSION_CHANGE:
		{
			if (wParam==WTS_SESSION_LOGOFF)
				_log2.Print(0, VTCLOG("Log off sessionId=%d"), lParam);
		}
*/

	else
		return ::DefWindowProcA(hwnd, iMsg, wParam, lParam);
}


void TrMain::Start()
{
	RLMutexLock l(m_start_stop_lock);

	if (m_status != STATUS::Stopped) return;

	m_bStopping = FALSE;

	// Clear the client mapping table
	m_clients.Reset();

	m_terminateReason = aaCloseSession;
	m_status = STATUS::Started;
	m_dwTicksNextTryListen = ::GetTickCount();

	::InitializeSecurityDescriptor(&m_sdPublic, SECURITY_DESCRIPTOR_REVISION);
	::SetSecurityDescriptorDacl(&m_sdPublic, TRUE, NULL, FALSE);
	m_saPublic.nLength = sizeof(m_saPublic);
	m_saPublic.lpSecurityDescriptor = &m_sdPublic;
	m_saPublic.bInheritHandle = FALSE;

	
	// create window
	{
		LPCSTR szClassName = "AmmyyAdminTarget3";

		WNDCLASSEX wndclass;

		wndclass.cbSize			= sizeof(wndclass);
		wndclass.style			= 0;
		wndclass.lpfnWndProc	= &WndProcDef;
		wndclass.cbClsExtra		= 0;
		wndclass.cbWndExtra		= 0;
		wndclass.hInstance		= TheApp.m_hInstance;
		wndclass.hIcon			= NULL;
		wndclass.hCursor		= NULL;
		wndclass.hbrBackground	= NULL;
		wndclass.lpszMenuName	= NULL;
		wndclass.lpszClassName	= szClassName;
		wndclass.hIconSm		= NULL;

		::RegisterClassExA(&wndclass);

		m_hwnd = ::CreateWindowA(szClassName, "", WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, CW_USEDEFAULT, 100, 100, NULL, NULL, TheApp.m_hInstance, NULL);

		if (m_hwnd == NULL) 
			throw RLException("in TrMain::Start() error=%d", ::GetLastError());
	}
}

void TrMain::Stop(bool wait, int terminateReason)
{
	if (terminateReason!=-1) m_terminateReason=terminateReason;

	RLMutexLock l(m_start_stop_lock);

	if (m_status==STATUS::Started)
	{
		_log.WriteInfo("TrMain::Stop1()#%s", "1");

		m_bStopping = TRUE;
		
		InteropCommon::SendInfo("Stopping");

		if (m_openedIncomePort) {
			m_listener.FreePort(DEFAULT_INCOME_PORT); // TODO: need to save port on OpenPort() in case it was changed
			m_openedIncomePort = false;
		}
		m_mutexIncomePort.Close();

		_log.WriteInfo("TrMain::Stop1()#%s", "2");

		KillClients(m_terminateReason);	// Kill all clients!
		
		m_status = STATUS::Stopping;
	}

	if (m_hwnd!=NULL) {
		if (::DestroyWindow(m_hwnd)==0)
			_log.WriteError("DestroyWindow() error=%d", ::GetLastError());
		m_hwnd = NULL;
	}
	
	_log.WriteInfo("TrMain::Stop1()#%s", "3");

	if (wait) {
		WaitUntilNoClients();
		OnTimer(); // for changing status Stopping->Stopped
	}
}


