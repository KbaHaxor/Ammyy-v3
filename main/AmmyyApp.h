#if !defined(_AMMYY_APP_7C7B1A91179F__INCLUDED_)
#define _AMMYY_APP_7C7B1A91179F__INCLUDED_

class AmmyyApp
{
public:
	AmmyyApp();

	void WinMain();
	void OnLogChange();
	HICON  LoadMainIcon();

	CStringW GetRootFolderW();

private:
	void	 SetRootFolder();
	void	 IsSystemUser();
	void	 Init();
	void	 OnExit();
	void	 ParseCommandLine();
	void     WinMainWithOutGUI();	
	CStringW m_rootFolderW;
	bool	 m_service;

public:
	struct CCmgArgs {
		bool debug;	// if true - take dll files near this exe
		bool startClient;
		bool noGUI;
		bool install;
		bool remove;
		bool log;
		bool elevated;
		bool lunch;
		bool showversion;
		bool setsettings;
		bool newid;
		bool sas; // Ctrl+Alt+Del for Vista or Win7
		UINT sasSession; 
		RLStream  set_proxy;
		CStringA  connect;
	}m_CmgArgs;

	DWORD		m_dwOSVersion;
	bool		m_systemUser;

public:
	bool    m_urgentQuit;	// when new version is loaded
	CStringA		m_appName;
	CStringW		m_appNameW;
	HINSTANCE		m_hInstance;
	LPCSTR			m_lpCmdLine;
	HWND			m_hMainWnd;
	UINT32			m_ID; // Ammyy ID is sent by web-server, 0 - if OFF or was error
};

extern AmmyyApp TheApp;

#include "ServiceManager.h"

extern CServiceManager ServiceManager;


#define AMMYY_SERVICE_FINISH_EVENT     "Global\\Ammyy.Service.FinishEvent"
#define AMMYY_SERVICE_CLOSE_SELF_EVENT "Global\\Ammyy.Service.CloseSelfEvent"


#endif // _AMMYY_APP_7C7B1A91179F__INCLUDED_
