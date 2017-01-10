#include "stdafx.h"
#include "RouterApp.h"
#include "DlgMain.h"
#include "Service.h"
#include "../main/ServiceManager.h"

RouterApp TheApp;


CService        Service("AmmyyRouter");
CServiceManager ServiceManager("AmmyyRouter", "Ammyy Router");


RouterApp::RouterApp()
{
}

void RouterApp::ParseCommandLine()
{
	memset(&m_CmdArgs, 0, sizeof(m_CmdArgs));
	m_CmdArgs.log = false;	// only errors to log
	m_CmdArgs.service = false;

	CStringA cmdLine = m_lpCmdLine;

	for (int i=0; i<cmdLine.GetLength();) {
		CStringA arg = Common2::GetSubString(cmdLine, " ", i);
		arg.TrimLeft();
		arg.TrimRight();
		if (arg.IsEmpty()) continue;

		else if (arg=="-service") 	m_CmdArgs.service = true;
		else if (arg=="-startapp")	m_CmdArgs.autoStartApp = true;
		else if (arg=="-log")		m_CmdArgs.log=true;
	}

	if (m_CmdArgs.log)	ServiceManager.m_addArguments += " -log";
}


void RouterApp::OnLogChange()
{
	_log.m_exactTime = m_CmdArgs.log;
	if (m_CmdArgs.log) {		
		_log.OpenFile();
	}
	else {
		_log.CloseFile();
	}
}


void RouterApp::WinMain()
{
	_log.Init(this->m_hInstance);
	//_log.SetLogLevel(1);
	RLEH::m_msgbox = true;
	//RLEH::Init(true); //tmp_code

	WSADATA wsaData;
	VERIFY(::WSAStartup(MAKEWORD(2,2), &wsaData) == 0);

	CStringA pathFull = Common2::GetModuleFileNameA(0);
	m_path    = Common2::GetPath(pathFull);
	m_exeName = CCommon::GetShortFileName(pathFull);

	try {
		if (::SetCurrentDirectory(m_path)==0) {
			_log.WriteError("SetCurrentDirectory() error=%d, '%s'", ::GetLastError(), m_path);
		}

		ParseCommandLine();
		OnLogChange();

		if (m_CmdArgs.service) {
			RLEH::m_msgbox = false;
			CService::WinMain();
			return;
		}
		else {
			DlgMain dlg;
			dlg.DoModal();
		}
	}
	catch(RLException& ex) {
		RL_ERROR(ex.GetDescription());
	}
}

LPCSTR RouterApp::GetBuildDateTime()
{
	return __DATE__ " at " __TIME__;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
	/*
	#ifdef _DEBUG
		int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );	// Get current flag
		tmpFlag |= _CRTDBG_LEAK_CHECK_DF;						// Turn on leak-checking bit
		_CrtSetDbgFlag( tmpFlag );								// Set flag to the new value

		//_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF );

		//_CrtSetAllocHook(YourAllocHook);
	#endif
	*/

	TheApp.m_hInstance = hInstance;
	TheApp.m_lpCmdLine = lpCmdLine;
	TheApp.WinMain();

	return 0;
}