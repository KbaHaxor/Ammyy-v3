#include "stdafx.h"
#include "AmmyyApp.h"
#include "DlgMain.h"
#include "Common.h"
#include "Service.h"
#include "Vista.h"
#include "TSSessions.h"
#include "../main/aaProtocol.h"
#include "../RL/RLTimer.h"
#include "../RL/RLEvent.h"
#include "../RL/RLBase64Coder.h"
#include "../RL/RLHttp.h"
#include "../target/TrMain.h"
#include "CmdBase.h"
#include "resource.h"
#include <Sddl.h>
#include "CmdSessionEnded.h"
#include "../target/TrService.h"


#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

AmmyyApp TheApp;

CServiceManager ServiceManager(AMMYYSERVICENAME, "Ammyy Admin");


AmmyyApp::AmmyyApp()
{
	m_urgentQuit = false;
	m_dwOSVersion = 0;
	m_hMainWnd  = NULL;
	m_ID = 0;
	m_appName  = "Ammyy Admin";
	m_appNameW = m_appName;
}

HICON AmmyyApp::LoadMainIcon()
{
	return ::LoadIcon((HINSTANCE)0x400000, MAKEINTRESOURCE(IDR_MAINFRAME));
}


void AmmyyApp::ParseCommandLine()
{
	m_CmgArgs.debug = false;
	m_CmgArgs.startClient = false;
	m_CmgArgs.noGUI = false;
	m_CmgArgs.install = false;
	m_CmgArgs.remove = false;
	m_CmgArgs.log = false;	// only errors to log
	m_CmgArgs.elevated = false;
	m_CmgArgs.lunch = false;
	m_CmgArgs.showversion = false;
	m_CmgArgs.setsettings = false;
	m_CmgArgs.newid = false;
	m_CmgArgs.sas = false;
	m_CmgArgs.sasSession = 0;

	m_service = false;

	CStringA cmdLine = m_lpCmdLine;

	for (int i=0; i<cmdLine.GetLength();) {
		CStringA arg = CCommon::GetSubString(cmdLine, " ", i);
		arg.TrimLeft();
		arg.TrimRight();
		if (arg.IsEmpty()) continue;

		if		(arg=="-debug")			m_CmgArgs.debug = true;
		else if (arg=="-install")		m_CmgArgs.install = true;
		else if (arg=="-remove")		m_CmgArgs.remove  = true;
		else if (arg=="-service") 		m_service = true;
		else if (arg=="-nogui")			m_CmgArgs.noGUI = true;
		else if (arg=="-lunch")			m_CmgArgs.lunch = true;
		else if (arg=="-startclient")	m_CmgArgs.startClient = true;
		else if (arg=="-log")			m_CmgArgs.log=true;
		else if (arg=="-elevated")		m_CmgArgs.elevated=true;
		else if (arg=="-showversion")	m_CmgArgs.showversion=true;
		else if (arg=="-setsettings")	m_CmgArgs.setsettings=true;
		else if (arg=="-newid")			m_CmgArgs.newid=true;
		else if (arg.Mid(0, 7)=="-dosas_") {
			m_CmgArgs.sas=true;
			m_CmgArgs.sasSession = atol(arg.Mid(7));
		}
		else if (arg.Mid(0, 11)=="-set_proxy_") {
			RLStream streamIn;
			streamIn.AddRaw((LPCSTR)arg + 11, arg.GetLength()-11);
			RLBase64Coder base64coder(NULL);
			base64coder.Decode(&streamIn, &m_CmgArgs.set_proxy);
		}
		else if (arg=="-connect") {
			int h = cmdLine.Find(" -", i);
			if (h==-1) h = cmdLine.GetLength();
			m_CmgArgs.connect = cmdLine.Mid(i, h-i);
			i = h;
		}
	}

	if (m_CmgArgs.debug)		ServiceManager.m_addArguments += " -debug";
	if (m_CmgArgs.log)			ServiceManager.m_addArguments += " -log";
}


void AmmyyApp::OnLogChange()
{
	bool log = m_CmgArgs.log || settings.m_debugLog;
	_log.SetLogLevel(log ? 9999 : 0);
	_log.m_exactTime = log;
	if (log)
		_log.OpenFile();
	else 
		_log.CloseFile();
}

void AmmyyApp::WinMainWithOutGUI()
{
	try {
		_log.WriteInfo("WinMainWithOutGUI()#0 sessionId=%d, activeSessionId=%d, processId=%u, root='%s'",
			TSSessions.GetSessionId(), TSSessions.WTSGetActiveConsoleSessionId(), ::GetCurrentProcessId(), 
			(LPCSTR)(CStringA)TheApp.GetRootFolderW());

		RLEvent event;
		event.Create(AMMYY_SERVICE_FINISH_EVENT);

		CServerInteract interact;

		RLTimer t1(false);
		RLTimer t3(false);

		bool was_ok_last_time = false;

		// settings.Load(); already done in Init()
		_TrMain.Start(); // start here, cause if we couldn't get ID, we can accept by IP

		while (true)
		{
			if (event.IsSet()) {
				_log.WriteInfo("WinMainWithOutGUI()#%u", 2);
				_TrMain.Stop(true, aaCloseSession);
				_log.WriteInfo("WinMainWithOutGUI()#%u", 3);
				break;
			}

			if ((!_TrMain.IsRDPClients()) && TrMain::IsOutConsole()) {
				_log.WriteInfo("WinMainWithOutGUI()#%u", 4);
				_TrMain.Stop(true, aaErrorSessionInactive);
				break;
			}

			// read setting every 1 minute			
			if (t3.IsElapsedOrNotStarted(60*1000)) {
				try {
					settings.Load();
				}
				catch(RLException& ex) {
					_log.WriteError(ex.GetDescription());
				}
				t3.Start();
			}

			// check in 20 seconds, if was error or in 1 hour if was ok and session was ended
			//
			bool b1 =  (!was_ok_last_time) ? t1.IsElapsedOrNotStarted(   20*1000) :
				                            (t1.IsElapsedOrNotStarted(60*60*1000) && _TrMain.m_eventRemovedClient);
			
			if (b1)	
			{
				try {					
					_log.WriteInfo("WinMainWithOutGUI()#%u", 5);

					if (settings.m_useWAN) {
						CStringA publicRouters = settings.m_publicRouters;
						UINT32   id             = TheApp.m_ID;

						interact.SendInitCmd();

						if (interact.m_updating) {
							//_log.WriteInfo("Closing because need an update");
							break;
						}

						if (id != TheApp.m_ID || publicRouters != settings.m_publicRouters) {
							_TrMain.KillClients(aaCloseSession);
							_TrMain.WaitUntilNoClients();
						}
						interact.m_bNeedComputerId = false;	// mark to avoid checking computerId in next time on server						
					}
					was_ok_last_time = true;

					_TrMain.m_eventRemovedClient = FALSE; // clear event

					_log.WriteInfo("WinMainWithOutGUI()#%u", 9);
				}
				catch(RLException& ex) {
					_log.WriteError(ex.GetDescription());
					was_ok_last_time = false;
				}
				t1.Start();
			}
			
			if (!_TrMain.OnTimer()) break;
			
			::Sleep(100);
		}
	}
	catch(RLException& ex) {
		_log.WriteError(ex.GetDescription());
	}

	_log.WriteInfo("WinMainWithOutGUI()#11");

	OnExit();

	_log.WriteInfo("WinMainWithOutGUI()#12");
}

void AmmyyApp::WinMain()
{
	ParseCommandLine();

	try {
		if (m_CmgArgs.showversion) {
			CStringA msg;
			msg.Format("v%s - %s", (LPCSTR)Settings::GetVersionSTR(), (LPCSTR)CCommon::GetBuildDateTime());
			::MessageBoxA(0, msg, "Ammyy Admin", MB_OK);
			return;
		}

		if (m_CmgArgs.sas) {
			//Init(); // for debug ONLY
			TrService::CallSAS(m_CmgArgs.sasSession);
			return;
		}

		Init();

		if (m_CmgArgs.setsettings) {
			DlgMain dlg;
			dlg.OnBtnSettings();
			return;
		}

		_log.WriteInfo("----------------WinMain()#1 version=%s, threadId=%X system=%d, %d=%d=%d, build='%s'", 
			settings.GetVersionSTR(), ::GetCurrentThreadId(), (int)m_systemUser,
			(int)m_service, (int)m_CmgArgs.noGUI, (int)m_CmgArgs.elevated, CCommon::GetBuildDateTime());

		if (m_service) {
			CService::WinMain();
			return;
		}
		if (m_CmgArgs.noGUI) {
			WinMainWithOutGUI();
			return;
		}
		if (m_CmgArgs.install) {
			ServiceManager.RunCmd(NULL, 0);
			return;
		}
		if (m_CmgArgs.remove) {
			ServiceManager.RunCmd(NULL, 3);
			return;
		}

		HWND hWndFound = ::FindWindowW(DlgMain::m_pClassName, NULL);
		if (hWndFound!=NULL) {
			::SwitchToThisWindow(hWndFound, TRUE);
			return;
		}
		
		// Make elevating for Vista
		if (settings.m_runAsSystemOnVista && (!m_systemUser) && (m_dwOSVersion>=0x60000))
		{
			try {
				bool isElevated = CVista::IsElevated();

				_log.WriteInfo("WinMain()#2 %d", (int)isElevated);

				if (isElevated) {
					// try to run as SYSTEM account
					CStringA serviceName;
					serviceName.Format("AmmyyAdmin_%X", ::GetCurrentProcessId());

					CServiceManager manager(serviceName, serviceName);
					manager.m_addArguments += " -lunch";
					manager.JustLunch(TSSessions.GetSessionId());
					return; // close this process
				}
				else {
					if (!m_CmgArgs.elevated) {
						if (CVista::Elevate()) return; // close this process
					}
				}
			}
			catch(RLException& ex) {
				_log.WriteError(ex.GetDescription());
				::MessageBox(NULL, ex.GetDescription(), "Ammyy Admin", MB_ICONERROR);
			}
		}

		_CmdSessionEndedQueue.m_fileName = (CStringA)TheApp.m_rootFolderW + "sessions.bin"; // other file name than in Router

		DlgMain dlg;
		dlg.DoModal();

		if (!m_urgentQuit) {
			settings.Save();
		}
	}
	catch(RLException& ex) {
		_log.WriteError(ex.GetDescription());

		if (!_log.m_silent)
			::MessageBox(NULL, ex.GetDescription(), "Error", MB_ICONERROR);
	}

	OnExit();
}



void AmmyyApp::IsSystemUser()
{
	m_systemUser = false;

	HANDLE hToken = NULL;
	if (::OpenProcessToken(::GetCurrentProcess(), TOKEN_READ, &hToken)==0)	
		throw RLException("error %d in IsSystemUser()#1", ::GetLastError());
	
	char buffer[8*1024];

	DWORD cbSize = 0;
	BOOL b = ::GetTokenInformation(hToken, TokenUser, buffer, sizeof(buffer), &cbSize);
	::CloseHandle(hToken);
	if (b==0)
		throw RLException("error in IsSystemUser()#2");

	TOKEN_USER* pTokenUser = (TOKEN_USER*)buffer;
	LPSTR sidAsString = NULL;

	b = ::ConvertSidToStringSidA(pTokenUser->User.Sid, &sidAsString);

	if (b==0 || sidAsString==NULL)
		throw RLException("error %d in IsSystemUser()#3", ::GetLastError());

	m_systemUser = _stricmp(sidAsString, "S-1-5-18")==0;
	//_log.WriteInfo("SID: %s", sidAsString);
	::LocalFree(sidAsString);

	//	char name[128];
	//	DWORD size = sizeof(name);
	//	if (::GetUserName(name, &size)==0) {
	//		name[0] = 0;
	//		_log.WriteError("GetUserName() error=%d", ::GetLastError());
	//	}
	//	m_userName = name;
	//	m_userName.MakeUpper();
}

void AmmyyApp::Init()
{
	_log.m_silent = m_service || m_CmgArgs.noGUI;

	if (m_service)
	{
		CStringW path = CCommon::GetPathW(CCommon::GetModuleFileNameW(NULL)) + L"AMMYY_service.log";
		_log.InitW(path);
	}
	else {
		_log.Init((HMODULE)m_hInstance);
	}

	OnLogChange();

	RLEH::m_msgbox = !_log.m_silent;
	RLEH::Init(true);
	RLEH::m_strAppName			= "Ammyy Admin";
	CmdBase::m_strAppName		= "Ammyy Admin";
	CmdBase::m_strAppModule		= "exe";
	CmdBase::m_strAppVersion	= settings.GetVersionSTR();

	// Set this process to be the last application to be shut down.
	::SetProcessShutdownParameters(0x100, 0);

	//  Get the current OS version
	{
		OSVERSIONINFO osInfo;
		osInfo.dwOSVersionInfoSize = sizeof(osInfo);
		::GetVersionEx(&osInfo);
		m_dwOSVersion = MAKELONG(osInfo.dwMinorVersion, osInfo.dwMajorVersion);
		_log.WriteInfo("OS version %X", m_dwOSVersion);
	}

	IsSystemUser();
	

	// no need socket and settings for service
	if (!m_service) 
	{
		// Initialise socket
		WSADATA wsaData;
		if (::WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
			throw RLException("Error=%d WSAStartup()", ::GetLastError());
		}

		settings.Load();

		if (m_CmgArgs.set_proxy.GetLen()>0)
		{
			RLHttp::m_proxyPort		= m_CmgArgs.set_proxy.GetUINT16();
			RLHttp::m_proxyHost		= m_CmgArgs.set_proxy.GetString1A();
			RLHttp::m_proxyUsername	= m_CmgArgs.set_proxy.GetString1A();
			RLHttp::m_proxyPassword	= m_CmgArgs.set_proxy.GetString1A();

			settings.Save();
		}

		if (settings.m_startClient) m_CmgArgs.startClient = true;
	}
}



void AmmyyApp::OnExit() 
{
	try {
		_TrMain.Stop(true);
	}
	catch(RLException& ex) {
		_log.WriteError(ex.GetDescription());
	}
	::WSACleanup();
}


void AmmyyApp::SetRootFolder()
{
	if (TheApp.m_CmgArgs.debug) {
		m_rootFolderW = CCommon::GetPathW(CCommon::GetModuleFileNameW(NULL));
		CCommon::CheckFolderTailW(m_rootFolderW);
	}
	else {
		for(int i=1; i<=2; i++)
		{
			CStringW path = CCommon::GetFolderCandidate(i);

			if (!CCommon::FileIsExistW(path)) {
				if (::CreateDirectoryW(path, NULL)==FALSE) {
					_log.WriteError("CreateDirectoryW('%s') error=%u", (LPCSTR)(CStringA)path, ::GetLastError()); 
					continue;
				}
				if (i==1) {
					try {
						CCommon::SetDirectoryForEveryone(path);
					}
					catch(RLException& ex) {
						_log.WriteError(ex.GetDescription());
					}
				}
			}
			else {
				if (i==1 && !CCommon::IsFolderWrittable(path)) continue;
			}
			m_rootFolderW = path;
			break;
		}
	}

	if (m_rootFolderW.IsEmpty())
		throw RLException("Can't find any writtable folder"); // not found

	if (::SetCurrentDirectoryW((LPCWSTR)m_rootFolderW)==0)
		throw RLException("SetCurrentDirectoryW('%s') error=%u", (LPCSTR)(CStringA)m_rootFolderW, ::GetLastError());
}

CStringW AmmyyApp::GetRootFolderW()
{
	if (m_rootFolderW.IsEmpty()) SetRootFolder();
	return m_rootFolderW;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
	#ifdef _DEBUG
		int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );	// Get current flag
		tmpFlag |= _CRTDBG_LEAK_CHECK_DF;						// Turn on leak-checking bit
		_CrtSetDbgFlag( tmpFlag );								// Set flag to the new value

		//_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF );

		//_CrtSetAllocHook(YourAllocHook);
	#endif

	TheApp.m_hInstance = hInstance;
	TheApp.m_lpCmdLine = lpCmdLine;
	TheApp.WinMain();	

	return 0;
}
