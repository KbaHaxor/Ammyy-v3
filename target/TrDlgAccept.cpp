#include "stdafx.h"
#include "trDlgAccept.h"
#include "trService.h"
#include "resource.h"
#include "../main/resource.h"
#include "TrMain.h"
#include "../RL/RLDlgTemplate.h"
#include "../main/RLLanguages.h"
#include "../main/DlgMain.h"


TrDlgAccept::TrDlgAccept()
{
	m_remember = false;
	m_lpTemplateName = MAKEINTRESOURCE(IDD_TR_ACCEPT);
}

TrDlgAccept::~TrDlgAccept()
{
}

	
/*
int TrDlgAccept::DoDialog()
{
	RLDlgTemplate data(512); // 482
	
	data.AddDlgHeader(DS_SYSMODAL | DS_MODALFRAME | DS_SETFOREGROUND | DS_3DLOOK | DS_CENTER | WS_POPUP | WS_CAPTION | 
		DS_SETFONT, 0, 6, 0, 0, 324, 107, 0, 8, L"Microsoft Sans Serif", L"");
	
	data.AddDlgItem(0x50010000, 0, 0x0080FFFF, 7,  86,50,14, IDACCEPT,				L"");
	data.AddDlgItem(0x50010000, 0, 0x0080FFFF, 267,86,50,14, IDREJECT,				L"");
	data.AddDlgItem(0x50020000, 0, 0x0082FFFF, 7,7,310,12,   IDC_ACCEPT_CONN_TEXT,	L"");
	data.AddDlgItem(0x50010003, 0, 0x0080FFFF, 7,25,310,10,  IDC_VIEW_SCREEN,		L"");	
	data.AddDlgItem(0x50010003, 0, 0x0080FFFF, 7,40,310,10,  IDC_ENABLE_FS,			L"");
	data.AddDlgItem(0x50010003, 0, 0x0080FFFF, 7,55,310,10,  IDC_REMEMBER,			L"");

	return RLDlgBase::DoModalIndirect(NULL, data.GetTemplate(), TheApp.m_hInstance);
}
*/


HBRUSH TrDlgAccept::OnCtlColor(HDC hdc, HWND hwnd) 
{ 
	::SetBkMode(hdc, TRANSPARENT);
	return _GUICommon.m_brush[3];
}

INT_PTR TrDlgAccept::WindowProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg==WM_ERASEBKGND)
	{
		RECT rect;
		this->GetClientRect(&rect);
		RLWnd::FillSolidRect((HDC)wParam, &rect, RGB(233,233,233));
		return TRUE;
	}

	if (msg==WM_COMMAND) 
	{
		if (BN_CLICKED  == HIWORD(wParam)) {			
			if ((HWND)lParam==(HWND)m_wndBtn[0]) {
				bool b = m_wndBtn[0].GetCheckBool();
				m_wndBtn[1].SetCheckBool(b);
				::EnableWindow(m_wndBtn[1], (BOOL)b);
				return TRUE;
			}			
		}

		switch (LOWORD(wParam)) 
		{
			// User clicked Accept or pressed return
		case IDACCEPT:
			this->OnExit(true);
			::EndDialog(m_hWnd, IDOK);
			return TRUE;

		case IDREJECT:
			this->OnExit(false);
			::EndDialog(m_hWnd, IDCANCEL);
			return TRUE;
		};
	}
	else if (msg==WM_TIMER) 
	{
		if (m_transport->IsReadyForRead(0)) {
			m_remember = false; // not remember because it's not user answer
			::EndDialog(m_hWnd, IDCANCEL);
		}
	}	
	return 0;
}


BOOL TrDlgAccept::OnInitDialog()
{
	this->SetTextA(TheApp.m_appName);
	this->SetForegroundWindow();

	m_wndBtn[0].AttachDlgItem(m_hWnd, IDC_VIEW_SCREEN);
	m_wndBtn[1].AttachDlgItem(m_hWnd, IDC_REMOTE_CONTROL);
	m_wndBtn[2].AttachDlgItem(m_hWnd, IDC_ENABLE_FS);
	m_wndBtn[3].AttachDlgItem(m_hWnd, IDC_AUDIO_CHAT);
	m_wndBtn[4].AttachDlgItem(m_hWnd, IDC_RDP_SESSION);

	m_wndRemember   .AttachDlgItem(m_hWnd, IDC_REMEMBER);

	if (m_prm.m_id==0) {
		m_remember = false;
		::EnableWindow(m_wndRemember, FALSE);
	}

	for (int i=0; i<COUNTOF(m_wndBtn); i++) m_wndBtn[i].SetCheckBool(true);
			
	{
		CStringW templ = rlLanguages.GetValue(D_ACCEPT_CONN_TEXT);
		
		CStringW str1;
		if (m_prm.m_id>0) {
			str1.Format(L" [ ID=%d ]", m_prm.m_id);
		}

		CStringW str;
		str.Format((LPCWSTR)templ, (LPCWSTR)str1);
		::SetDlgItemTextW(m_hWnd, IDC_ACCEPT_CONN_TEXT, str);
	}
	
	static UINT dlgIds[] = { IDACCEPT, IDREJECT, IDC_VIEW_SCREEN,IDC_REMOTE_CONTROL, IDC_ENABLE_FS, IDC_RDP_SESSION, IDC_AUDIO_CHAT, IDC_REMEMBER, 0 };
	static UINT strIds[] = { D_ACCEPT, D_REJECT,   D_VIEW_SCREEN,  D_REMOTE_CONTROL,   D_ENABLE_FS,   D_RDP_SESSION,   D_AUDIO_CHAT,   D_REMEMBER_MY_ANSWER, 0 };
	rlLanguages.SetDlgItemsText(m_hWnd, dlgIds, strIds);

	// Beep
	::MessageBeep(MB_ICONEXCLAMATION);

	::SetTimer(m_hWnd, 1, 1000, NULL);

	return FALSE;	// Return false to prevent accept button from gaining focus.
}

void TrDlgAccept::OnExit(bool accept)
{
	m_prm.ClearAll();
	m_prm.m_password.SetEmpty();
	
	if (accept)	{
		if (m_wndBtn[0].GetCheckBool()) m_prm.Set(Permission::ViewScreen);
		if (m_wndBtn[1].GetCheckBool()) m_prm.Set(Permission::RemoteControl + Permission::ClipboardOut + Permission::ClipboardIn);
		if (m_wndBtn[2].GetCheckBool()) m_prm.Set(Permission::FileManager);
		if (m_wndBtn[3].GetCheckBool()) m_prm.Set(Permission::AudioChat);
		if (m_wndBtn[4].GetCheckBool()) m_prm.Set(Permission::RDPsession);
	}

	//m_enableRemoteInput = m_wndEnableInput.GetCheckBool();
	//m_enableFileSystem  = m_wndEnableFS   .GetCheckBool();
	m_remember          = m_wndRemember   .GetCheckBool();
}


BOOL TrDlgAccept::OnEndDialog(BOOL ok)
{
	return FALSE; // don't allow to press Enter or Esc
}
