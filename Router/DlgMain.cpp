#include "stdafx.h"
#include "DlgMain.h"
#include "resource.h"
#include "RouterApp.h"
#include "CRouter.h"




DlgMain::DlgMain()
{
	m_lpTemplateName = MAKEINTRESOURCE(IDD_DLG_MAIN);
	m_autoStart = false;
}

DlgMain::~DlgMain()
{

}

BOOL DlgMain::OnInitDialog()
{
	RLWnd wnd;
	wnd.AttachDlgItem(m_hWnd, IDC_BUILD_TIME);

	CStringA text = CStringA("Build Time: ") + RouterApp::GetBuildDateTime();

	wnd.SetTextA(text);

	if (TheApp.m_CmdArgs.autoStartApp) OnBtnStart();

	return TRUE;
}

BOOL DlgMain::OnEndDialog(BOOL ok)
{
	Router.Stop();

	return TRUE;
}

void DlgMain::OnBtnStart()
{
	try {
		Router.Start();
	}
	catch(RLException& ex) {
		::MessageBox(m_hWnd, ex.GetDescription(), "ERROR", MB_ICONEXCLAMATION);
	}
}

void DlgMain::OnBtnStop()
{
	try {
		Router.Stop();
	}
	catch(RLException& ex) {
		::MessageBox(m_hWnd, ex.GetDescription(), "ERROR", MB_ICONEXCLAMATION);
	}
}

void DlgMain::OnInitMenuPopup(HMENU hMenu, UINT nIndex, BOOL bSysMenu)
{
	if (bSysMenu) return;

	UINT menuId = ::GetMenuItemID(hMenu, 0);

	if (menuId==ID_SERVICE_INSTALL) {
		bool bInstall , bStart, bStop, bRemove;
		ServiceManager.GetStatus(bInstall, bStart, bStop, bRemove);

		::EnableMenuItem(hMenu, 0, (bInstall ? MF_ENABLED : MF_GRAYED) | MF_BYPOSITION);
		::EnableMenuItem(hMenu, 1, (bStart   ? MF_ENABLED : MF_GRAYED) | MF_BYPOSITION);
		::EnableMenuItem(hMenu, 2, (bStop    ? MF_ENABLED : MF_GRAYED) | MF_BYPOSITION);
		::EnableMenuItem(hMenu, 3, (bRemove  ? MF_ENABLED : MF_GRAYED) | MF_BYPOSITION);

		return;
	}
	else if (menuId==ID_START) {
		bool bStarted = Router.IsRunning();
		::EnableMenuItem(hMenu, 0, (bStarted ? MF_GRAYED : MF_ENABLED) | MF_BYPOSITION);
		::EnableMenuItem(hMenu, 1, (bStarted ? MF_ENABLED : MF_GRAYED) | MF_BYPOSITION);
	}
}


INT_PTR DlgMain::WindowProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg==WM_COMMAND) 
	{
		WORD id = LOWORD(wParam);

		if (id==ID_START) {
			OnBtnStart();
			return TRUE;
		}
		if (id==ID_STOP) {
			OnBtnStop();
			return TRUE;
		}
		if (id>=ID_SERVICE_INSTALL && id<=ID_SERVICE_REMOVE) {
			ServiceManager.RunCmd(m_hWnd, id-ID_SERVICE_INSTALL);
			return TRUE;
		}
	}
	else if (msg==WM_INITMENUPOPUP) {
		OnInitMenuPopup((HMENU)wParam, LOWORD(lParam), HIWORD(lParam));
	}


	return 0;
}

