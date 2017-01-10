#if (!defined(_TR_SERVER_H__INCLUDED_))
#define _TR_SERVER_H__INCLUDED_

#include "../main/InteropTarget.h"
#include "../main/TCP.h"
#include "TrDesktopUtils.h"
#include "TrListener.h"

class TrClient;

class TrMain
{
public:
	TrMain();
	void Start();
	void Stop(bool wait, int terminateReason=-1);

	bool IsAllowedNewAuthorized();
	void WaitUntilNoClients();

	bool OnTimer(); // should be called each 100ms
private:
	void OnTimer1();	
public:
	void KillClients(UINT16 reason);

	// Let a client remove itself
	void RemoveClient(TrClient* pClient);	

	static bool IsOutConsole();
	static UINT GetStateTotalWAN();
	       UINT GetStateLocalWAN();
		   void GetStateIncomePort(bool& total, bool& local);
	bool IsRDPClients();	

	TrClient* AddClient();	

private:
	bool IsExistClients();

private:	
	RLStream    m_clients; // TrClient* objects
	void ClientsEnumReset()
	{
		m_clients.SetReadPos(0);
	}
	
	TrClient* ClientsEnumNext()
	{
		if (m_clients.GetAvailForReading()<4) return (TrClient*)-1;
		TrClient* p;
		m_clients.GetRaw(&p, 4);
		return p;
	}

	// Lock to protect the client list from concurrency - lock when reading/updating client list
	RLMutex	m_clients_lock;
	RLMutex	m_start_stop_lock;

	bool		m_openedIncomePort;
	DWORD		m_dwTicksNextTryListen;
	RLMutexWin	m_mutexIncomePort;

public:
	enum STATUS
	{
		Stopped  = 0,
		Started  = 1,
		Stopping = 2,
	};

	STATUS	m_status;
	TrListener m_listener;
	BOOL	m_bStopping;	// stopping event
	UINT16  m_terminateReason;

	TrDesktopOptimizer m_desktopOptimizer;

	volatile BOOL m_eventRemovedClient;

	HWND m_hwnd; // for for AudioOut and for getting WM_ENDSESSION

	SECURITY_ATTRIBUTES m_saPublic;
private:
	SECURITY_DESCRIPTOR m_sdPublic;
};

extern TrMain _TrMain;

#endif // _TR_SERVER_H__INCLUDED_
