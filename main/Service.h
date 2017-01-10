#if !defined(AFX_SERVICE_H__AB824C7F_0BAC_4820_AAF7_04EDF189770A__INCLUDED_)
#define AFX_SERVICE_H__AB824C7F_0BAC_4820_AAF7_04EDF189770A__INCLUDED_

class CService  
{
public:
	CService();
	virtual ~CService();

	static void WinMain();

//private:
	static HANDLE CreateProcess1(DWORD sessionId, LPWSTR lpDesktop, LPCWSTR lpCommandLine);
private:
	static HANDLE GetToken(ULONG sessionId);
	static bool IsProcessRunning(HANDLE hProcess);
	static void  WINAPI ServiceMain(DWORD argc, char**argv);
	static DWORD WINAPI ServiceWorkThread(LPVOID);
	static DWORD WINAPI ServiceHandlerProc(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
	
	       BOOL ReportStatus(DWORD state, DWORD exitcode, DWORD waithint);
	       void ServiceStop();

	static BOOL CreateRemoteSessionProcessW(DWORD dwSessionId, BOOL bUseDefaultToken, HANDLE hToken, 
        LPCWSTR lpApplicationName, LPWSTR lpCommandLine, 
		LPSECURITY_ATTRIBUTES lpProcessAttributes, 
		LPSECURITY_ATTRIBUTES lpThreadAttributes,
        BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, 
        LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);

	static DWORD MarshallString(LPCWSTR pszText, RLStream& stream);


private:
	SERVICE_STATUS			m_srvstatus; // current status of the service
	SERVICE_STATUS_HANDLE	m_hstatus;
	DWORD					m_dwWorkThreadId;	
};

extern CService Service;


#define AMMYYSERVICENAME "AmmyyAdmin"

#endif // !defined(AFX_SERVICE_H__AB824C7F_0BAC_4820_AAF7_04EDF189770A__INCLUDED_)
