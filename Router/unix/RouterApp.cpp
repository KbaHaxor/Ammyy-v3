#include "stdafx.h"
#include "RouterApp.h"

RouterApp TheApp;

#ifdef _WIN32
#include "DlgMain.h"
#include "Service.h"
#include "../main/ServiceManager.h"

CService        Service("AmmyyRouter");
CServiceManager ServiceManager("AmmyyRouter", "Ammyy Router");
#else
#include "CRouter.h"
#include <signal.h>
#ifdef _LINUX
#include <sys/resource.h>
#include <unistd.h>
#include <linux/sysctl.h>
#include <sys/syscall.h>
#endif
#endif


RouterApp::RouterApp()
{
}

void RouterApp::ParseCommandLine()
{
	memset(&m_CmdArgs, 0, sizeof(m_CmdArgs));
	m_CmdArgs.log = false;	// only errors to log
	

	CStringA cmdLine = m_lpCmdLine;

	for (int i=0; i<cmdLine.GetLength();) {
		CStringA arg = Common2::GetSubString(cmdLine, " ", i);
		arg.TrimLeft();
		arg.TrimRight();
		if (arg.IsEmpty()) continue;

#ifdef _WIN32
		else if (arg=="-service") 	m_CmdArgs.service = true;
		else if (arg=="-startapp")	m_CmdArgs.autoStartApp = true;
#else
		else if (arg=="-console")	m_CmdArgs.console=true;
		else if (arg=="-daemon")	m_CmdArgs.daemon=true;
		else if (arg=="-wait")	    m_CmdArgs.wait  =true;
		else if (arg=="-nostdin")	m_CmdArgs.nostdin=true;
		else if (arg=="-reuseport")	m_CmdArgs.reuseport=true;
		else if (arg=="-sigsegv")	m_CmdArgs.sigsegv=true;
#endif
		else if (arg=="-createconfig")	m_CmdArgs.createconfig=true;
		else if (arg=="-log")			m_CmdArgs.log=true;
		else {
			char c = arg[0]; // can be '55'
			if (c<'0' || c>'9') throw RLException("Invalid argument %s", (LPCSTR)arg);
		}
	}

#ifdef _WIN32
	if (m_CmgArgs.log)	ServiceManager.m_addArguments += " -log";
#endif
}


void RouterApp::OnLogChange()
{
	_log.m_exactTime = m_CmdArgs.log;
	if (m_CmdArgs.log) {		
		_log.OpenFile();
		_log.SetLogLevel(99);
	}
	else {
		_log.SetLogLevel(0);
		_log.CloseFile();		
	}
}

#ifndef _WIN32
void RouterApp::signal_handler(int sig) 
{
	if (sig==SIGPIPE) return; // sometimes occurs when calling send() for socket

	//_log.WriteError("signal_handler %d", sig);

	if (sig==SIGHUP || sig==SIGINT || sig==SIGTERM || sig==SIGTSTP)
	{
		TheApp.m_stopping = true;

		// emulate Press Enter
		if (TheApp.m_CmdArgs.console && (!TheApp.m_CmdArgs.nostdin)) {
			ungetc('\n', stdin);
		}
	}
}
#endif


// on Linux can be run as
// "setsid ./ammyy_router -console -nostdin -189 > out.log 2>&1 &"

void RouterApp::WinMain()
{
	try {
	#ifdef _WIN32
		_log.Init(this->m_hInstance);
		RLEH::m_msgbox = true;
		RLEH::Init(true);
	#endif

		CStringA path = Common2::GetModuleFileNameA(0);
		m_path    = Common2::GetPath(path);
		m_exeName = Common2::GetFileName(path);

	#ifdef _WIN32
		if (::SetCurrentDirectory(m_path)==0)
			_log.WriteError("SetCurrentDirectory() error=%d, '%s'", ::GetLastError(), m_path);	

		WSADATA wsaData;
		VERIFY(::WSAStartup(MAKEWORD(2,2), &wsaData) == 0);
	#else
		CStringA logName = m_path + m_exeName + ".log";
		_log.InitA(logName);
		_log.m_console = true;
	#endif

		ParseCommandLine();
		OnLogChange();

	#ifdef _WIN32
		if (m_CmdArgs.service) {
			RLEH::m_msgbox = false;
			CService::WinMain();
			return;
		}
		else {
			DlgMain dlg;
			dlg.DoModal();
		}
	#else
		if (m_CmdArgs.createconfig) {
			printf("creating config file\n");
			CRouter::ConfigCreateFile();
			return;
		}


		if (!(m_CmdArgs.console || m_CmdArgs.daemon || m_CmdArgs.wait)) {			
			printf("Ammyy Router usage: ./%s -console | -daemon | -wait\n", (LPCSTR)m_exeName);
			printf("build time - %s\n", GetBuildDateTime());
			printf("\n");
			return;
		}

		m_stopping = false;

		{
			// these singnals make core dump
			// SIGSEGV, // segmentation fault
			// SIGFPE,	// floating point exceptions
			// SIGABRT, SIGBUS, SIGILL, SIGSYS, SIGTRAP

			int signals[] = {
				
				SIGHUP,  // parent is closed
				SIGINT,  // Ctrl+C pressed
				SIGTERM, // kill command
				SIGTSTP, // Ctrl+Z pressed
					
				// for catching problem
				SIGPIPE, SIGALRM, SIGXCPU, SIGXFSZ, SIGVTALRM, 
				SIGPROF, SIGQUIT, SIGTTIN, SIGTTOU, SIGUSR1, 
				SIGUSR2, SIGPOLL, SIGCONT,

				// SIGKILL, SIGSTOP, //error=22 while set this handler
				// SIGEMT, SIGSTP, SIGINFO  // not found in Linux
			};

			for (int i=0; i<sizeof(signals)/sizeof(signals[0]); i++) 
			{
				int signum = signals[i];
				sighandler_t p = signal(signum, signal_handler);
				//printf("sig(%d)=%X\n", signum, p);

				if (p==SIG_ERR)
					_log.WriteError("signal(%d), error=%d", signum, errno);
			}
		}


	#ifdef _LINUX
		{
			struct rlimit limit;
			limit.rlim_cur = limit.rlim_max = -1;
			if (setrlimit(RLIMIT_CORE, &limit)) _log.WriteError("setrlimit(RLIMIT_CORE) error=%d", errno);

			limit.rlim_cur = limit.rlim_max = 64000;
			if (setrlimit(RLIMIT_NOFILE, &limit)) _log.WriteError("setrlimit(RLIMIT_NOFILE) error=%d", errno);

			struct rlimit l1, l2;
			if (getrlimit(RLIMIT_CORE,   &l1))	_log.WriteError("getrlimit(RLIMIT_CORE)   error=%d", errno);
			if (getrlimit(RLIMIT_NOFILE, &l2))	_log.WriteError("getrlimit(RLIMIT_NOFILE) error=%d", errno);
			printf("ulimit -c %d %d\n", l1.rlim_cur, l1.rlim_max);
			printf("ulimit -n %d %d\n", l2.rlim_cur, l2.rlim_max);
		}

		//tmp_code
		// "echo 6 > /proc/sys/net/ipv4/tcp_retries2"
		/*
		{
			int name[] = { CTL_NET, NET_IPV4_TCP_RETRIES2 };
			char   oldval[64];
			size_t oldvalue_size = sizeof(oldval);
			
			struct __sysctl_args args = {0};

			memset(&args, 0, sizeof(struct __sysctl_args));

			args.name = name;
			args.nlen = sizeof(name)/sizeof(name[0]);
			args.oldval = oldval;
			args.oldlenp = &oldvalue_size;

			int v = syscall(SYS__sysctl, &args);

			printf("v = %d\n", v);
		}
		*/
	#endif

		// read & check ports here, just for daemon
		try {
			Router.ConfigReadFile();
		}
		catch(RLException& ex) {
			printf("ERROR:%s\n", (LPCSTR)ex.GetDescription());
			return;
		}

		// wait untill all ports ok in any case
		{
			while(true) {
				try {
					Router.CheckPorts();
					if (m_CmdArgs.wait) {
						printf("done all ports is ready\n");
						return; // ok
					}
					else 
						break; // ok, so we can exit;
				}
				catch(RLException& ex) {
					printf("ERROR:%s\n", (LPCSTR)ex.GetDescription());
				}

				::Sleep(1000);

				if (m_stopping) return; // user want to cancel
			}
		}

		if (m_CmdArgs.console) {
			Router.Start();
			printf("Ammyy Router started, press Enter to stop Router\n");

			if (m_CmdArgs.nostdin){
				while(!m_stopping) usleep(200*1000);
			}
			else {
				// wait press enter	
				int i;
				scanf("%c", &i);
			}

			printf("Ammyy Router stopping\n");
			Router.Stop();
			printf("Ammyy Router stopped\n");
		}
		else {
			printf("Ammyy Router starting as daemon\n");

			daemon(1, 0);
			_log.m_console = false;

			Router.Start();

			_log.WriteInfo("Deamon is started");

			while(!m_stopping) usleep(200*1000);

			_log.WriteInfo("Deamon is finishing");
			Router.Stop();
			_log.WriteInfo("Deamon is finished");
		}
	#endif
	}
	catch(RLException& ex) {
		RL_ERROR(ex.GetDescription());
	}
}

LPCSTR RouterApp::GetBuildDateTime()
{
	return __DATE__ " at " __TIME__;
}

#ifdef _WIN32
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
#else
int main(int argc, char *argv[])
{
	CStringA cmd;
	for (int i=1; i<argc; i++) {
		if (i>1) cmd += " ";
		cmd += argv[i];
	}

	TheApp.m_lpCmdLine = cmd;
	TheApp.WinMain();
	return 0;
}
#endif

