#include "stdafx.h"
#include "DlgHttpProxy.h"
#include "resource.h"
#include "../RL/RLHttp.h"
#include "../RL/RLRegistry.h"
#include "ImpersonateWrapper.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

DlgHttpProxy::DlgHttpProxy()
{
	m_lpTemplateName = MAKEINTRESOURCE(IDD_HTTP_PROXY);
}

DlgHttpProxy::~DlgHttpProxy()
{

}

UINT16 DlgHttpProxy::GetIEProxy(CStringA& host)
{
	const LPCSTR szSysProxyRegKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings";

	RLRegistry registry;	

#ifndef CUSTOMIZER
	LoadUserProfileWrapper impersonate;
	registry.OpenA(impersonate.GetProfile(), szSysProxyRegKey, KEY_READ);
#else
	registry.OpenA(HKEY_CURRENT_USER, szSysProxyRegKey, KEY_READ);
#endif

	DWORD ProxyEnable = 0;

	if (!registry.GetDWORDValueA("ProxyEnable", ProxyEnable)) {
		throw RLException("Not found ProxyEnable in registry");
	}

	if (ProxyEnable==0) return 0;

	CStringA server;

	if (!registry.GetStringValueA("ProxyServer", server))
		throw RLException("Not found ProxyServer in registry");

	for (int i=0; i<2; i++) {
		static LPCSTR keys[] = {"https=", "http="};
		int i1 = server.Find(keys[i]);
		if (i1<0) continue; // not found
		if (i1>0 && server[i1-1]!=';') continue; // not correct
		i1 += strlen(keys[i]);
		int i2 = server.Find(";", i1);
		if (i2<0) i2 = server.GetLength();
		server = server.Mid(i1, i2-i1);
		break;
	}

	if (server.Find('=')>=0) return 0; // if ftp proxy is set, but no https & http

	UINT16 port = 80; // if port is not set, IE use 80 even for https

	int i1 = server.Find(':');
	if (i1>0) {
		port = atol((LPCSTR)server+i1+1);
		host = server.Left(i1);
	}
	else
		host = server;
	
	return port;
}

void DlgHttpProxy::OnBtnSetIEProxy()
{
	CStringA address = m_wndIEAddress.GetTextA();
	CStringA portstr = m_wndIEPort.GetTextA();
	UINT16   port	 = (UINT16)atol(portstr);

	m_use = (port!=0 && (!address.IsEmpty()));

	if (!m_use) {
		address = "";
		portstr = "";
		m_wndUsername.SetTextA("");
		m_wndPassword.SetTextA("");
	}
	
	m_wndAddress.SetTextA(address);
	m_wndPort.SetTextA(portstr);
	m_wndUseProxy.SetCheckBool(m_use);
	SetControlsEnabled();
}


void DlgHttpProxy::OnProxyTypeChanged()
{
	bool use = m_wndUseProxy.GetCheckBool();

	if (m_use==use) return; // not changed
	m_use = use;
	SetControlsEnabled();

	if (!m_use) {
		m_wndAddress.SetTextA("");
		m_wndPort.SetTextA("");
		m_wndUsername.SetTextA("");
		m_wndPassword.SetTextA("");
	}
}


void DlgHttpProxy::SetControlsEnabled()
{
	BOOL b = (m_use) ? TRUE : FALSE;
	::EnableWindow((HWND)m_wndAddress,  b);
	::EnableWindow((HWND)m_wndPort,     b);
	::EnableWindow((HWND)m_wndUsername, b);
	::EnableWindow((HWND)m_wndPassword, b);
}


BOOL DlgHttpProxy::OnInitDialog()
{
	m_wndUseProxy.AttachDlgItem(m_hWnd, IDC_USE_PROXY);
	m_wndAddress.AttachDlgItem( m_hWnd, IDC_ADDRESS);
	m_wndPort.AttachDlgItem(    m_hWnd, IDC_PORT);
	m_wndUsername.AttachDlgItem(m_hWnd, IDC_USERNAME);
	m_wndPassword.AttachDlgItem(m_hWnd, IDC_PASSWORD);
	m_wndIEAddress.AttachDlgItem(m_hWnd, IDC_IE_ADDRESS);
	m_wndIEPort.AttachDlgItem(m_hWnd, IDC_IE_PORT);

	m_use = (RLHttp::m_proxyPort!=0);

	m_wndUseProxy.SetCheckBool(m_use);

	if (m_use) {
		m_wndUsername.SetTextA(RLHttp::m_proxyUsername);
		m_wndPassword.SetTextA(RLHttp::m_proxyPassword);

		char portstr[16];
		sprintf(portstr, "%u", (int)RLHttp::m_proxyPort);
		m_wndPort.SetTextA(portstr);
		m_wndAddress.SetTextA (RLHttp::m_proxyHost);
	}
	
	SetControlsEnabled();


	try {
		CStringA host;
		UINT16 port = GetIEProxy(host);

		char portstr[16];
		if (port==0)
			portstr[0] = 0;
		else
			sprintf(portstr, "%u", (int)port);

		m_wndIEAddress.SetTextA(host);
		m_wndIEPort.SetTextA(portstr);
	}
	catch(RLException& ex) {
		::MessageBox(m_hWnd, ex.GetDescription(), "Ammyy Admin", MB_ICONERROR);
	}

	return TRUE;
}


BOOL DlgHttpProxy::OnEndDialog(BOOL ok)
{
	if (!ok) return TRUE;

	UINT16 port = 0;
	CStringA host, username, password;

	if (m_use)
	{
		username = m_wndUsername.GetTextA();
		password = m_wndPassword.GetTextA();

		host	 = m_wndAddress.GetTextA();
		port	 = (WORD)atol(m_wndPort.GetTextA());

		if (host.IsEmpty()) {
			::MessageBox(m_hWnd, "Incorrect address", "Ammyy Admin", MB_ICONWARNING);
			return FALSE;
		}

		if (port==0) {
			::MessageBox(m_hWnd, "Incorrect port", "Ammyy Admin", MB_ICONWARNING);
			return FALSE;
		}
	}
	
	// all is fine	
	RLHttp::m_proxyPort  = port;
	RLHttp::m_proxyHost  = host;
	RLHttp::m_proxyUsername = username;
	RLHttp::m_proxyPassword = password;

	return TRUE;
}


INT_PTR DlgHttpProxy::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_COMMAND:
		{
			if (LOWORD(wParam)==IDC_USE_PROXY) { OnProxyTypeChanged(); return TRUE;	}
			if (LOWORD(wParam)==IDC_SET_IE_PROXY) { OnBtnSetIEProxy(); return TRUE; }
			break;
		};
	}
	return 0;
}
