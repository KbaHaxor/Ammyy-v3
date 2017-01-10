#include "stdafx.h"
#include "AmmyyInstApp.h"
#include "../main/common.h"
#include "DlgMain.h"


CAmmyyInstApp TheApp;


CAmmyyInstApp::CAmmyyInstApp()
{
}

CAmmyyInstApp::~CAmmyyInstApp()
{
}

CStringA CAmmyyInstApp::GetSetProxyString()
{
	RLStream set_proxy;

	set_proxy.AddUINT16(RLHttp::m_proxyPort);
	set_proxy.AddString1A(RLHttp::m_proxyAddress);
	set_proxy.AddString1A(RLHttp::m_proxyUsername);
	set_proxy.AddString1A(RLHttp::m_proxyPassword);

	RLStream streamOut;
	RLBase64Coder base64coder(NULL);
	base64coder.Encode(&set_proxy, &streamOut);

	return streamOut.GetString0A();
}


bool CAmmyyInstApp::TryToRun(LPCSTR args)
{
	for (int i=1; i<=2; i++) {
		CStringW path = CCommon::GetFolderCandidate(i) + L"AMMYY_Admin.exe";
		if (CCommon::FileIsExistW(path))
		{
			STARTUPINFO si;
			PROCESS_INFORMATION pi;
			::GetStartupInfo(&si);	

			CStringA currentDirectory = CCommon::GetPath(path);

			if (args) path = "\"" + path + "\" " + args;

			BOOL b = ::CreateProcess(NULL, path.GetBuffer(0), NULL, NULL, FALSE, NULL, NULL, currentDirectory, &si, &pi);
			if (b == FALSE) {
				throw RLException("Can't run '%s' error=%d", path, ::GetLastError());
			}
			return true;
		}
	}
	return false;
}

void CAmmyyInstApp::WinMain()
{
	try {
		CStringA args = "";

		//test code
		/*		
		if (true) {
			RLHttp::m_proxyPort = 7888;
			RLHttp::m_proxyAddress = "proxytest";
			RLHttp::m_proxyUsername = "ggg";
			RLHttp::m_proxyPassword = "111";

			args = "-set_proxy_" + GetSetProxyString();
		}
		*/
		
		

		if (TryToRun(args.GetLength()<=0 ? NULL : (LPCSTR)args)) return;

		//CDlgMain dlg;
		//dlg.DoModal();

	}
	catch(RLException& ex) {
		::MessageBox(NULL, ex.GetDescription(), "Ammyy Admin", MB_ICONERROR);
	}
}



int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
	//TheApp.m_hInstance = hInstance;
	//TheApp.m_lpCmdLine = lpCmdLine;
	TheApp.WinMain();

	return 0;
}
