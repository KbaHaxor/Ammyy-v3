#if !defined(AFX_SERVICE_H__AB824C7F_0BAC_4820_AAF7_04EDF189770A__INCLUDED_)
#define AFX_SERVICE_H__AB824C7F_0BAC_4820_AAF7_04EDF189770A__INCLUDED_

class CService  
{
public:
	CService(LPCSTR lpServiceName);
	virtual ~CService();

	static void WinMain();

private:
	static void  WINAPI ServiceMain(DWORD argc, char**argv);
	static DWORD WINAPI ServiceHandlerProc(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
	
	BOOL ReportStatus(DWORD state, DWORD exitcode, DWORD waithint);
	void OnStart();
    void OnStop();



private:
	SERVICE_STATUS			m_srvstatus; // current status of the service
	SERVICE_STATUS_HANDLE	m_hstatus;

	LPCSTR m_serviceName;
};

extern CService Service;

#endif // !defined(AFX_SERVICE_H__AB824C7F_0BAC_4820_AAF7_04EDF189770A__INCLUDED_)
