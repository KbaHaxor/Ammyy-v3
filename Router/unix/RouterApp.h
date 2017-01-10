#if !defined(AFX_TCPTUNNEL_H__976B31C0_0116_4797_B127_36867D95A69C__INCLUDED_)
#define AFX_TCPTUNNEL_H__976B31C0_0116_4797_B127_36867D95A69C__INCLUDED_

class RouterApp
{
public:
	RouterApp();

	static LPCSTR GetBuildDateTime();
	void OnLogChange();
	void WinMain();	

private:	
	void ParseCommandLine();

public:
#ifdef _WIN32
	HINSTANCE	m_hInstance;
#endif
	LPCSTR		m_lpCmdLine;
	CStringA	m_path;
	CStringA	m_exeName; // short name of current process

	struct CCmdArgs {
#ifdef _WIN32
		bool autoStartApp;
		bool service;
#else
		bool console;
		bool daemon;
		bool wait;
		bool nostdin;
		bool reuseport;
		bool sigsegv;
#endif
		bool createconfig;
		bool log;	// debug log
	}m_CmdArgs;

private:
#ifndef _WIN32
	static void signal_handler(int sig);
	bool m_stopping;
#endif

};

extern RouterApp TheApp;

#ifdef _WIN32
#include "../main/ServiceManager.h"
extern CServiceManager ServiceManager;
#endif

#endif // !defined(AFX_TCPTUNNEL_H__976B31C0_0116_4797_B127_36867D95A69C__INCLUDED_)
