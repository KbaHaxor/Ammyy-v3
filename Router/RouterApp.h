#if !defined(_ROUTER_APP_H__36867D95A69C__INCLUDED_)
#define _ROUTER_APP_H__36867D95A69C__INCLUDED_

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
	HINSTANCE	m_hInstance;
	LPCSTR		m_lpCmdLine;
	CStringA	m_path;
	CStringA	m_exeName; // short name of current process

	struct CCmdArgs {
		bool autoStartApp;
		bool service;
		bool log;	// debug log
	}m_CmdArgs;
};

extern RouterApp TheApp;

#include "../main/ServiceManager.h"
extern CServiceManager ServiceManager;

#endif // !defined(_ROUTER_APP_H__36867D95A69C__INCLUDED_)
