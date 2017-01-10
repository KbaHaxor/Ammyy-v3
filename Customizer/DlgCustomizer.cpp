#include "stdafx.h"
#include "AmmyCustomizer.h"
#include "DlgCustomizer.h"
#include "../RL/RLResource.h"
#include "../main/DlgPermissionList.h"


#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <windows.h>

#define AMMY_ADDR_ID 63;
#define AMMY_PORT_ID 64;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma comment(lib, "Comdlg32.lib")

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

/*
class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}
*/

/////////////////////////////////////////////////////////////////////////////
// DlgCustomizer dialog

DlgCustomizer::DlgCustomizer()
{
	m_lpTemplateName = MAKEINTRESOURCE(IDD_AMMYCUSTOMIZER_DIALOG);
}


INT_PTR DlgCustomizer::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_COMMAND:
		{
			WORD wItem = LOWORD(wParam);

			if (wItem==IDC_BUTTON1)		{ OnBrowseSourceIcon();	return TRUE; }
			if (wItem==IDC_BUTTON2)		{ OnTargetBrowse();		return TRUE; }
			if (wItem==IDUPDATE)		{ OnBtnUpdate();		return TRUE; }
			if (wItem==IDB_PERMISSIONS)	{ OnBtnPermissions();	return TRUE; }			

			break;
		};
/*
	case WM_NOTIFY:
		{
			NMHDR* pNMHdr = (NMHDR*)lParam;

			if (pNMHdr->code == HDN_ITEMCLICKA) {
				LPNMHEADER phdr = (LPNMHEADER)lParam;
				if (phdr->iButton == 0) this->SortItems(phdr->iItem);					
				return 0;
			}

			if (pNMHdr->idFrom==IDC_LIST) {
				if (pNMHdr->code==LVN_ITEMCHANGED) return this->OnItemChanged((NMLISTVIEW*)lParam);			
				if (pNMHdr->code==NM_DBLCLK) { this->OnEdit(); return 0; }				
			}
			break;
		};
*/
	}
	return 0;
}



BOOL DlgCustomizer::OnInitDialog()
{
	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	m_UpdateButton = ::GetDlgItem(m_hWnd, IDUPDATE);

	m_wndIcoPath.AttachDlgItem(m_hWnd, IDC_EDIT_SOURCEICONFILE);
	m_wndExePath.AttachDlgItem(m_hWnd, IDC_EDIT_TARGETFILE);


	/*
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}
	*/

	CStringA txt = "Ammyy Customizer v3.0 - ";
	txt += __DATE__ " at " __TIME__;
	this->SetTextA(txt);
	
	::EnableWindow(m_UpdateButton, FALSE);

	HICON hIconSmall = (HICON)::LoadImage((HINSTANCE)0x400000, MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	HICON hIconBig   = (HICON)::LoadImage((HINSTANCE)0x400000, MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);

	SetIcon(hIconBig,    TRUE);		// Set big icon
	SetIcon(hIconSmall, FALSE);		// Set small icon
		
	return TRUE;  // return TRUE  unless you set the focus to a control
}

/*
void DlgCustomizer::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}


// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void DlgCustomizer::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR DlgCustomizer::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}
*/

CStringW OpenFileDialog(HWND ownerWnd, LPCWSTR filter, LPCWSTR title)
{
	OPENFILENAMEW ofn = {0};
    WCHAR fileName[MAX_PATH];
    wcscpy(fileName, L"");

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = ownerWnd;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = sizeof(fileName);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = title;
	ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST|OFN_HIDEREADONLY;	

    if (::GetOpenFileNameW(&ofn)==0) return "";

	return CStringW(ofn.lpstrFile);
}


void DlgCustomizer::OnBrowseSourceIcon() 
{
	CStringW path = OpenFileDialog(m_hWnd, L"Icon (*.ico)\0*.ico\0", L"Select ICO file");

	if (path.GetLength()>0) {	
		m_wndIcoPath.SetTextW(path);
	}
}

void DlgCustomizer::OnTargetBrowse() 
{
	CStringW path = OpenFileDialog(m_hWnd, L"*.exe (*.exe)\0*.exe\0", L"Select Ammyy Admin EXE file");
	
	if (path.GetLength()>0) {
		m_wndExePath.SetTextW(path);

		BOOL b = (path.GetLength()>0) ? TRUE : FALSE;
		::EnableWindow(m_UpdateButton, b);
	}
}

void DlgCustomizer::OnBtnPermissions()
{
	DlgPermissionList dlg;
	if (dlg.DoModal(m_hWnd)!= IDOK) return;
}

void DlgCustomizer::OnBtnUpdate() 
{
	RLWnd wndRouter(::GetDlgItem(m_hWnd, IDC_ROUTER_2));
	RLWnd wndURL(::GetDlgItem(m_hWnd, IDC_URL));

	Updater updater;
	updater.m_url            = wndURL.GetTextA();
	updater.m_privateRouters = wndRouter.GetTextA();	
	updater.m_exePath		 = m_wndExePath.GetTextA();
	updater.m_iconPath		 = m_wndIcoPath.GetTextA();
	
	try {
		updater.RemoveSignature(updater.m_exePath);
		updater.DoUpdate();
		::MessageBox(m_hWnd, "The exe file was updated", "Ammyy Customizer", MB_OK | MB_ICONINFORMATION);
	}
	catch(RLException& ex) {
		::MessageBox(m_hWnd, ex.GetDescription(), "Ammyy Customizer", MB_ICONERROR);
	}
}
