#include "stdafx.h"
#include "vrMain.h"
#include "vrOptions.h"
#include "Htmlhelp.h"
#include "commctrl.h"
#include "../main/aaDesktop.h"
#include "../main/resource.h"
#include "VrClient.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


VrOptions::VrOptions()
{
	m_lpTemplateName = MAKEINTRESOURCE(IDD_VR_OPTION1);

	m_FullScreen = false;

	Load();
}



VrOptions& VrOptions::operator=(VrOptions& s)
{
	m_prm				 = s.m_prm;
	m_allowRemoteControl = s.m_allowRemoteControl;
	m_allowClipboardOut  = s.m_allowClipboardOut;
	m_allowClipboardIn   = s.m_allowClipboardIn;	
	m_FullScreen		 = s.m_FullScreen;
	m_autoScrollWndMode  = s.m_autoScrollWndMode;
	m_scaling			 = s.m_scaling;
	m_FitWindow			 = s.m_FitWindow;
	m_scale_num			 = s.m_scale_num;
	m_scale_den			  = s.m_scale_den;
	m_cursorLocal		  = s.m_cursorLocal;
	m_cursorRemoteRequest = s.m_cursorRemoteRequest;

	return *this;
}

VrOptions::~VrOptions() {}


// Greatest common denominator, by Euclid
int gcd(int a, int b) {
	if (a < b) return gcd(b, a);
	if (b == 0) return a;
	return gcd(b, a % b);
}

// reduce scaling factors by greatest common denominator
void VrOptions::SetScaling(int num, int den) 
{
	m_scale_num = num;
	m_scale_den = den;

	if (m_scale_num < 1 || m_scale_den < 1) {
		::MessageBoxA(NULL,  "Invalid scale factor - resetting to normal scale", "Argument error",MB_OK | MB_TOPMOST | MB_ICONWARNING);
		m_scale_num = 1;
		m_scale_den = 1;
	}
	else {
		int g = gcd(m_scale_num, m_scale_den);
		m_scale_num /= g;
		m_scale_den /= g;
	}
	m_scaling = (m_scale_num!=1) || (m_scale_den!=1);
}


INT_PTR VrOptions::WindowProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_HELP:	 {  vrMain.m_pHelp->Popup(lParam); return 0; }
		case WM_COMMAND: { return this->OnCommand(wParam, lParam); }
	}
    return 0;
}


void VrOptions::FillComboBox(RLWndComboBox& wnd, int key)
{
	CStringW strings = rlLanguages.GetValue(key);
	ASSERT(strings.GetLength()!=0);

	int c = strings.Replace('|', 0) + 1;

	LPCWSTR s = strings;
	
	for (int i=0; i<c; i++)
	{
		wnd.InsertStringW(i, s);
		s += wcslen(s)+1;
	}
}

void VrOptions::FillSavedItems()
{
	::SendMessage(::GetDlgItem(m_hWnd, IDC_AUTO_SCROLL_WND_MODE),   BM_SETCHECK, m_autoScrollWndMode, 0);

	{
		// skip "aaSetPointerPos"
		int index = 0;
			 if (m_cursorRemoteRequest==aaSetPointerShape+aaSetPointerPos) index = 2;
		else if (m_cursorRemoteRequest==aaSetPointerShape)                 index = 1;
		m_wndRemoteCursor.SetCurSel(index);
	}

	m_wndLocalCursor.SetCurSel(m_cursorLocal);

	if (m_FitWindow) {
		::SetDlgItemText(m_hWnd, IDC_SCALE_EDIT, "Auto");
	} else {	
		::SetDlgItemInt (m_hWnd, IDC_SCALE_EDIT, ((m_scale_num*100) / m_scale_den), FALSE);
	}
}


BOOL VrOptions::OnInitDialog()
{
	InitCommonControls();
	VrMain::CentreWindow(m_hWnd);

	CStringW caption = L"Ammyy - " + rlLanguages.GetValue(D_SETTINGS) + L" (" + rlLanguages.GetValue(D_OPERATOR) + L")";
	::SetWindowTextW(m_hWnd, caption);

	m_wndRemoteCursor.AttachDlgItem	 (m_hWnd, IDC_REMOTE_CURSOR);
	m_wndLocalCursor.AttachDlgItem	 (m_hWnd, IDC_LOCAL_CURSOR);
	FillComboBox(m_wndRemoteCursor, D_REMOTE_CURSOR_SHAPES);
	FillComboBox(m_wndLocalCursor,  D_LOCAL_CURSOR_SHAPES);
	
	
	::SetDlgItemText(m_hWnd, IDC_CONNECTION_INFO, m_connectionInfo);
				
	::SendMessage(::GetDlgItem(m_hWnd, IDC_FULLSCREEN),	BM_SETCHECK, m_FullScreen, 0);

	{
		static LPCSTR scalecombo[] = {"25","50","75","90","100","150","200","Auto"};
		RLWndComboBox wndScale(::GetDlgItem(m_hWnd, IDC_SCALE_EDIT));		
		for (int i=0; i<COUNTOF(scalecombo); i++)
			wndScale.InsertString(i, scalecombo[i]);
	}					
	
	FillSavedItems();

	m_permPanel.Create(m_hWnd, 8, 4);

	for (int i=0; i<COUNTOF(m_permPanel.m_wnd); i++) {
		bool b1 = m_prm.Get(1<<i);				// have permissions
		bool b2 = b1 && (i>=1 && i<=3);         // possible to change it

		if (b1) {
			     if (i==1) b1 = m_allowRemoteControl;
			else if (i==2) b1 = m_allowClipboardOut;
			else if (i==3) b1 = m_allowClipboardIn;
		}

		m_permPanel.m_wnd[i].SetCheckBool(b1);
		::EnableWindow(m_permPanel.m_wnd[i], b2);
	}
	return TRUE;
}


BOOL VrOptions::OnCommand(WPARAM wParam, LPARAM lParam)
{
	WORD msg = HIWORD(wParam);

	switch (LOWORD(wParam))	{
	
	case IDC_LOAD: { OnBtnLoad(); return TRUE; }
	case IDC_SAVE: { OnBtnSave(); return TRUE; }

	case IDC_SCALE_EDIT:
			if (msg==CBN_KILLFOCUS) {
				Lim(m_hWnd, IDC_SCALE_EDIT, 1, 150);
			}
			return 0;			
	}
	return 0;
}

bool VrOptions::IsChecked(int nIDDlgItem)
{
	HWND hwnd = ::GetDlgItem(m_hWnd, nIDDlgItem);
	return (::SendMessage(hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED);
}

void VrOptions::Load()
{	
	m_cursorLocal			= settings.m_cursorLocal;
	m_cursorRemoteRequest	= settings.m_cursorRemoteRequest;
	m_autoScrollWndMode		= settings.m_autoScrollWndMode;

	ApplyScale(settings.m_scale);
}


void VrOptions::OnBtnLoad()
{
	Load();
	FillSavedItems();
}

void VrOptions::OnBtnSave()
{
	OnOK1();

	settings.m_scale				 = ::GetDlgItemInt(m_hWnd, IDC_SCALE_EDIT, NULL, FALSE);
	settings.m_cursorLocal			 = m_cursorLocal;
	settings.m_cursorRemoteRequest	 = m_cursorRemoteRequest;
	settings.m_autoScrollWndMode	 = m_autoScrollWndMode;

	try {
		settings.Save();
	}
	catch(RLException& ex) {
		::MessageBox(m_hWnd, ex.GetDescription(), "Ammyy Admin", MB_ICONWARNING);
	}
}

void VrOptions::ApplyScale(UINT16 scale)
{
	if (scale > 0) {
		this->SetScaling(scale, 100);
		m_FitWindow = false;
	} else {	
		m_FitWindow = true;
		m_scaling = true;
		m_scale_num = m_scale_den = 1; // will be set later
	}
}


BOOL VrOptions::OnEndDialog(BOOL ok)
{	
	if (ok) {
		::SetFocus(::GetDlgItem(m_hWnd, IDOK)); // TODO maximp: I don't know why
		this->OnOK1();
	}
	return TRUE;
}

void VrOptions::OnOK1()
{	
	int i = GetDlgItemInt(m_hWnd, IDC_SCALE_EDIT, NULL, FALSE);
	ApplyScale(i);			
													
	m_FullScreen			= IsChecked(IDC_FULLSCREEN);
	m_autoScrollWndMode		= IsChecked(IDC_AUTO_SCROLL_WND_MODE);

	// remote cursor, skip "aaSetPointerPos"
	{
		int index = m_wndRemoteCursor.GetCurSel();

		     if (index==1) index = aaSetPointerShape;
		else if (index==2) index = aaSetPointerShape+aaSetPointerPos;
		else               index = 0;

		m_cursorRemoteRequest = index;
	}

	m_cursorLocal = m_wndLocalCursor.GetCurSel();

	m_allowRemoteControl = m_permPanel.m_wnd[1].GetCheckBool();
	m_allowClipboardOut  = m_permPanel.m_wnd[2].GetCheckBool();
	m_allowClipboardIn   = m_permPanel.m_wnd[3].GetCheckBool();
}


void VrOptions::Lim(HWND hwnd, int control, DWORD min, DWORD max)
{
	int error;

	DWORD buf = GetDlgItemInt(hwnd, control, &error, FALSE);
	if (buf > max && error) {
		buf = max;
		SetDlgItemInt(hwnd, control, buf, FALSE);
	}
	if (buf < min && error) {
		buf = min;
		SetDlgItemInt(hwnd, control, buf, FALSE);
	}
}


