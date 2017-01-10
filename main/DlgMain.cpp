#include "stdafx.h"
#include "DlgMain.h"
#include "AmmyyApp.h"
#include "Common.h"
#include "DlgPermissionList.h"
#include "DlgSettings.h"
#include "DlgAbout.h"
#include "DlgContactBook.h"
#include "resource.h"
#include "Service.h"
#include "Vista.h"
#include "DlgHttpProxy.h"
#include "RLLanguages.h"
#include "../target/TrMain.h"
#include "../viewer/VrMain.h"
#include "../viewer/VrClient.h"
#include "CmdSessionEnded.h"
#include "Image.h"


#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

// Message used for system tray notifications
#define WM_TRAYNOTIFY				WM_USER+1

#define IDC_REMOTE_ID                   1509
#define IDC_SPEED                       1513


GUICommon _GUICommon;

DlgMain* DlgMain::m_pObject = NULL;


LPCWSTR DlgMain::m_pClassName = L"AmmyyAdmin3Main";


DlgMain::DlgMain()
{
	m_hNullBrush = (HBRUSH)::GetStockObject(NULL_BRUSH);
	_GUICommon.m_brush[0]  = ::CreateSolidBrush(RGB(210, 245, 222));
	_GUICommon.m_brush[1]  = ::CreateSolidBrush(RGB(212, 232, 246));
	_GUICommon.m_brush[2]  = ::CreateSolidBrush(RGB(255, 255, 255));
	_GUICommon.m_brush[3]  = ::CreateSolidBrush(RGB(233, 233, 233));

	m_fonts[0] = ::CreateFontA(14,0,0,0,FW_NORMAL, FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
			CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY, VARIABLE_PITCH,"Microsoft Sans Serif");
	
	m_fonts[1] = ::CreateFontA(14,0,0,0,FW_BOLD, FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
			CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY, VARIABLE_PITCH, "Microsoft Sans Serif");

	m_fonts[2] = ::CreateFontA(14,0,0,0,FW_BOLD, FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
			CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY, VARIABLE_PITCH, "Microsoft Sans Serif");


	m_hIconSmall = (HICON)::LoadImage((HINSTANCE)0x400000, MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	m_hIconBig   = (HICON)::LoadImage((HINSTANCE)0x400000, MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
	m_onTimerCalled = false;
	m_pObject = this;
	m_bkgndDC = NULL;
	m_bkgndBrush = NULL;
	m_menu.hFont = NULL;
	m_hMenuForID = NULL;
}

DlgMain::~DlgMain()
{
	for (int i=0; i<COUNTOF(m_fonts); i++) {
		::DeleteObject(m_fonts[i]);
	}
	m_pObject = NULL;

	if (m_bkgndDC!=NULL) {
		::DeleteDC(m_bkgndDC);
		m_bkgndDC = NULL;
	}

	if (m_bkgndBrush!=NULL) {
		::DeleteObject(m_bkgndBrush);
		m_bkgndBrush = NULL;
	}

	if (m_menu.hFont!=NULL)
		::DeleteObject(m_menu.hFont);

	if (m_hMenuForID!=NULL)
		::DestroyMenu(m_hMenuForID);
}

#define CreateChild1(wnd, styleEx, className, style, x, y, cx, cy, id) \
	wnd.CreateWindowExW(styleEx, className,   L"", style,  x, y, cx, cy, m_hWnd, (HMENU)id, 0, 0)


void DlgMain::SetTitle()
{
	CStringA str = "Ammyy Admin v";
	str += settings.GetVersionSTR();
	//str += " Beta";
	str += " - ";
	str += settings.GetLisenceString();

	this->SetTextA(str);
}



BOOL DlgMain::OnInitDialog()
{
	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);
	
	HMENU hSysMenu = GetSystemMenu(FALSE);
	if (hSysMenu != NULL)
	{
		::AppendMenu(hSysMenu, MF_SEPARATOR, 0, NULL);
		::AppendMenu(hSysMenu, MF_STRING, IDM_ABOUTBOX, "&About Ammyy Admin...");
	}		

	SetIcon(m_hIconBig,    TRUE);		// Set big icon
	SetIcon(m_hIconSmall, FALSE);		// Set small icon
	
	SetTitle();

	{
		//::SetWindowPos(m_hWnd, 0, 0, 0, 300, 300, SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOREDRAW );

		SIZE r11 = this->GetClientSize();
		
		int cx = r11.cx;
		
		int x1 = 10;
		int x2 = 280;
		int y = 0;
			
		DWORD dwStyle1 = WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOHSCROLL;
		DWORD dwStyle2 = WS_CHILD | WS_VISIBLE | BS_CHECKBOX | BS_AUTOCHECKBOX | WS_TABSTOP;
		DWORD dwStyle3 = 0x50010080;

		CreateChild1(m_wndYourIdEdit,  0x0004, WC_EDITW,     dwStyle1,   x1+24,  y+ 72,  100, 18,  IDC_YOUR_ID);
		CreateChild1(m_wndYourIpEdit,  0x0004, WC_EDITW,     dwStyle1,   x1+24,  y+132,  180, 18,  IDC_YOUR_IP);

		CreateChild1(m_wndBtnStart,    0x0004, WC_BUTTONW,   0x50010000, x1,     y+176, 110, 25,  IDB_START_STOP);
		CreateChild1(m_wndBtnStop,     0x0004, WC_BUTTONW,   0x50010000, x1+116, y+176, 110, 25,  IDB_STOP);

		CreateChild1(m_wndRemoteIdEdit,0x0004, WC_EDITW,     0x50010080, x2+3,   y+74,   150, 18,   IDC_REMOTE_ID);
		CreateChild1(m_wndSpeed,       0x0004, WC_COMBOBOXW, 0x50010203, x2,     y+102,  210, 100,	IDC_SPEED);		
		CreateChild1(m_wndViewOnly,    0,      WC_BUTTONW,   dwStyle2,   x2,     y+130,  210, 20,	0);
		CreateChild1(m_wndBtnConnect,  0x0004, WC_BUTTONW,   0x50010000, x2,     y+176,  210, 25,	IDB_CONNECT);
		
		CreateChild1(m_wndStatus,    0x0004, WC_STATICW,     0x50010800|SS_LEFTNOWORDWRAP, x1,     m_cy-24, cx-2*x1, 22,  IDC_STATUS);

		Image img;
		img.FromResourceByCenter(contactbook, 34, 20);
		HBITMAP hBitmap = img.GetBitmap();

		m_wndBtnContactBook.Create(x2+172, y+69, 38, 24, m_hWnd, 12348);
		m_wndBtnContactBook.InitImage2(hBitmap, hBitmap, true, false);
	}


	m_wndYourIdEdit  .SetFont(m_fonts[1]);
	m_wndYourIpEdit  .SetFont(m_fonts[0]);
	m_wndRemoteIdEdit.SetFont(m_fonts[1]);
	m_wndSpeed       .SetFont(m_fonts[0]);
	m_wndBtnConnect  .SetFont(m_fonts[1]);
	m_wndBtnStart    .SetFont(m_fonts[0]);
	m_wndBtnStop     .SetFont(m_fonts[0]);
	m_wndViewOnly    .SetFont(m_fonts[0]);
	m_wndStatus      .SetFont(m_fonts[0]);
				
	m_wndRemoteIdEdit.SetTextA(settings.m_operRemoteId);

	FillConnectionType(-1);
			
	m_wndViewOnly.SetCheckBool(settings.m_viewOnly);
	
	// set up language
	{
		// fill language menu	
		HMENU hMenu = ::GetMenu(m_hWnd);
		ASSERT(hMenu!=NULL);
		hMenu = ::GetSubMenu(hMenu, 1);
		ASSERT(hMenu!=NULL);

		if (hMenu) {
			int n = RLLanguages::GetCountLang();
			for (int i=1; i<n; i++) {
				LPCSTR lpLang = RLLanguages::GetLangName(i);
				::AppendMenuA(hMenu, MF_STRING, ID_LANGUAGE_ENGLISH+i, lpLang);
			}
		}	

		int langIndex = RLLanguages::GetLangIndex(settings.m_langId);
		OnChangeLanguage(langIndex + ID_LANGUAGE_ENGLISH);
	}

	{
		m_nid.hWnd = m_hWnd;
		m_nid.cbSize = sizeof(m_nid);
		m_nid.uID = IDR_MAINFRAME;	// never changes after construction
		m_nid.hIcon = m_hIconSmall;
		m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		m_nid.uCallbackMessage = WM_TRAYNOTIFY;
		strcpy(m_nid.szTip, "Ammyy Admin");


		// icon statusbar init
		if (Shell_NotifyIcon(NIM_ADD, &m_nid)==FALSE) {
			::MessageBox(m_hWnd, "Can't create taskbar icon", "Ammyy Admin", MB_ICONWARNING);
		}
	}

	if (TheApp.m_dwOSVersion>=0x60000)
		CVista::SetForegroundWindow(m_hWnd);
	else 
		SetForegroundWindow();

    SetFocus();

	SetTimer(1, 100, NULL);

	TheApp.m_hMainWnd = m_hWnd;
	InteropCommon::m_hErrorWnd = (HWND)m_wndStatus;

	//setup ips
	try {
		CStringA str = TCP::GetIPaddresses();
		str.Replace(", ", " ~ ");
		m_wndYourIpEdit.SetTextA(str);
	}
	catch(RLException& ex) {
		::MessageBox(NULL, ex.GetDescription(), "Ammyy Admin", MB_ICONWARNING);
	}

	m_hMenu = ::GetMenu(m_hWnd);

	int nItems = ::GetMenuItemCount(m_hMenu);
	for(int i=0; i<nItems; i++)
	{    
		MENUITEMINFO mii;
		memset(&mii,0,sizeof(mii));
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_FTYPE;
		mii.wID = i;
		::GetMenuItemInfo(m_hMenu, i, TRUE, &mii);
		mii.fType |= MFT_OWNERDRAW;
		mii.fMask |= MIIM_DATA; // mark last element
		mii.dwItemData = (i==nItems-1) ? 1: 0; // mark last element
		::SetMenuItemInfo(m_hMenu, i, TRUE, &mii);
	}


	NONCLIENTMETRICS ncm={0};
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, (PVOID)&ncm, 0);
	m_menu.hFont = ::CreateFontIndirect(&ncm.lfMenuFont);

	OnSelchangeType();

	return TRUE;  // return TRUE  unless you set the focus to a control
}


void DlgMain::FillConnectionType(int count_old)
{
	int count_new = settings.m_encoders.size();

	int index;

	if (count_old>0) {
		index = m_wndSpeed.GetCurSel();

		if (count_new!=count_old) {
			if (index>=count_old)
				index += count_new - count_old;		// index was after Desktop
			else {
				if (index>=count_new) index = 0;	// index was on Desktop
			}
		}
	}
	else {
		index = settings.m_operProtocol;
	}

	if (index>3+count_new) index = 0;

	m_wndSpeed.DeleteAllItems();

	for (int i=0; i<count_new; i++) {
		CStringA str = "Desktop - " + (CStringA)settings.m_encoders[i].name;
		m_wndSpeed.InsertString(i, str);
	}

	m_wndSpeed.InsertString(count_new+0, "Audio chat");
	m_wndSpeed.InsertString(count_new+1, "File Manager only");
	m_wndSpeed.InsertString(count_new+2, "Speed Test only");
	m_wndSpeed.InsertString(count_new+3, "Microsoft RDP");

	m_wndSpeed.SetCurSel(index);
}

void DlgMain::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		OnHelpAbout();
	}
}


/*
// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDlgMain::OnPaint() 
{
	PAINTSTRUCT ps;

	HDC hdc = ::BeginPaint(m_hWnd, &ps);
	
	::EndPaint(m_hWnd, &ps);


	
//	if (IsIconic())
//	{
//		CPaintDC dc(this); // device context for painting
//
//		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);
//
//		// Center icon in client rectangle
//		int cxIcon = GetSystemMetrics(SM_CXICON);
//		int cyIcon = GetSystemMetrics(SM_CYICON);
//		CRect rect;
//		GetClientRect(&rect);
//		int x = (rect.Width() - cxIcon + 1) / 2;
//		int y = (rect.Height() - cyIcon + 1) / 2;
//
//		// Draw the icon
//		dc.DrawIcon(x, y, m_hIcon);
//	}	
}
*/

HCURSOR DlgMain::OnQueryDragIcon()
{
	return (HCURSOR) m_hIconSmall;
}


void DlgMain::SendInitCmd2()
{
begin:
	try {
		SendInitCmd();

		if (m_updating)
		{
			m_wndYourIdEdit.SetTextA("---NEED UPDATE---");

			CStringW exeFileName = CCommon::GetModuleFileNameW(NULL);		// current exe filename

			//settings.Save();

			// run new version
			STARTUPINFOW si;
			PROCESS_INFORMATION pi;
			::GetStartupInfoW(&si);

			CStringW directory = CCommon::GetPathW(exeFileName);

			exeFileName = CCommon::WrapMarks(exeFileName); // in case of space char in full name

			BOOL b = ::CreateProcessW(NULL, exeFileName.GetBuffer(0), NULL, NULL, FALSE, NULL, NULL, directory, &si, &pi);
			if (b == FALSE)
				throw RLException("Can't run '%s' error=%d", (LPCSTR)(CStringA)exeFileName, ::GetLastError());
		}
		else {
			// make ditigs grouping by space
			CStringA strId = CCommon::ConvertIDToString(TheApp.m_ID, "OFF");
			m_wndYourIdEdit.SetTextA(strId);
			SetTitle(); // license type may be changed
			::DrawMenuBar(m_hWnd); // menu should be redrawen
		}
	}
	catch(RLException& ex) 
	{
		CStringA msg = ex.GetDescription();
		if (m_updating) msg = "Critical error while updating Ammyy Admin.\n" + msg;		
		msg += "\n\nWould you like to change proxy settings?";

		if (::MessageBox(m_hWnd, msg, "Ammyy Admin", MB_ICONERROR|MB_YESNO|MB_DEFBUTTON1) ==IDYES)
		{
			DlgHttpProxy dlg;
			if (dlg.DoModal(m_hWnd)==IDOK) {
				try {
					settings.Save();
				}
				catch(RLException& ex) {
					::MessageBox(m_hWnd, ex.GetDescription(), "Ammyy Admin", MB_ICONERROR);
				}
				goto begin;
			}
		}

		_log.WriteError(ex.GetDescription());
		m_wndYourIdEdit.SetTextA("---ERROR---");
	}	

	if (m_updating) {
		TheApp.m_urgentQuit = true;
		::SendMessage(m_hWnd, WM_CLOSE, 0, 0);
		return;
	}

	try {
		_CmdSessionEndedQueue.SendFromQueue();
	}
	catch(RLException& ex){
		_log.WriteError(ex.GetDescription());
	}
}


void DlgMain::OnTimer(UINT nIDEvent) 
{	
	// set states of buttons "start" and "stop"
	{
		bool b1 = _TrMain.m_status==TrMain::STATUS::Stopped;
		bool b2 = _TrMain.m_status==TrMain::STATUS::Started;
		::EnableWindow(m_wndBtnStart, b1);
		::EnableWindow(m_wndBtnStop,  b2);
	}

	// set state of lights
	{
		bool total, local;
		_TrMain.GetStateIncomePort(total, local);
		int state1 = _TrMain.GetStateLocalWAN();
		int state2 = TrMain::GetStateTotalWAN();

		for (int i=0; i<5; i++) {
			m_wndLights[i*2+0].SetState(i<state1);
			m_wndLights[i*2+1].SetState(i<state2);
		}

		m_wndLights[10].SetState(local);
		m_wndLights[11].SetState(total);
	}

	if (!m_onTimerCalled) {
		 m_onTimerCalled = true;

		if (!settings.m_useWAN) {
			m_wndYourIdEdit.SetTextA("OFF");
		}
		else {
			SendInitCmd2();
		}

		if (TheApp.m_urgentQuit) return;

		if (TheApp.m_CmgArgs.startClient) {
			OnBtnStart();
		}
		if (!TheApp.m_CmgArgs.connect.IsEmpty()) {
			m_wndRemoteIdEdit.SetTextA(TheApp.m_CmgArgs.connect);
			OnBtnConnect();
		}
	}

	_TrMain.OnTimer();
}


void DlgMain::OnDownloadFileStatus(LPCSTR msg, int percents)
{
	m_wndStatus.SetTextA(msg);
	UpdateWindow();
}


void DlgMain::OnBtnStart()
{
	try
	{
		if (!settings.m_allowIncomingByIP && !settings.m_useWAN)
			throw RLException("Settings doesn't allow to use Client");
		
		_TrMain.Start();
	}
	catch(RLException& ex) {
		::MessageBox(m_hWnd, ex.GetDescription(), "Ammyy Admin", MB_ICONERROR);
		//RL_ERROR(ex);
	}	
}


void DlgMain::OnBtnStop()
{
	try
	{
		_TrMain.Stop(false, aaCloseSession);
	}
	catch(RLException& ex) {
		RL_ERROR(&ex);
	}	
}


UINT DlgMain::ConvertID(LPCWSTR pID)
{
	UINT remoteID = 0;
	
	while(true)
	{
		WCHAR c = *pID++;

		if (c==0) return remoteID; // NULL-terminated string

		if (c<=' ') continue; // possible space in ID

		if (c>='0' && c<='9') {
			remoteID *= 10;
			remoteID += c-'0';
		}
		else
			return 0; // invalid ID, may be IP or host name		
	}
}


void DlgMain::OnBtnConnect()
{
	try
	{		
		CStringW strRemoteHost = m_wndRemoteIdEdit.GetTextW();

		strRemoteHost.TrimRight();
		strRemoteHost.TrimLeft();

		if (strRemoteHost.IsEmpty()) {
			LPCSTR msg = rlLanguages.GetValueA(D_CLIENT_ID_NOT_PROVIDED);
			::MessageBox(m_hWnd, msg, "Ammyy Admin", MB_ICONWARNING);
			return;
		}

		UINT remoteID = DlgMain::ConvertID(strRemoteHost);			

		CStringW caption;

		if (remoteID!=0)
		{
			CStringW computerAlias = DlgContactBook::FindNameByID(remoteID);

			if (computerAlias[0] > 0)
				caption.Format(L"%s=%u", (LPCWSTR)computerAlias, remoteID);
			else
				caption.Format(L"%u", remoteID);
		}
		else
			caption = strRemoteHost;

		bool viewOnly = m_wndViewOnly.GetCheckBool();

		int maxSessions = settings.GetMaxSessionsOnViewer();

		if (VrClient::GetClientsCount() >= maxSessions) 
		{
			CStringW msg;
			msg.Format(rlLanguages.GetValue(D_EXCEED_SESSIONS_VIEWER), maxSessions, (LPCSTR)settings.GetLisenceString());
			msg += L".\n";
			msg += rlLanguages.GetValue(D_BUY_LICENSE);
			::MessageBoxW(m_hWnd, msg, TheApp.m_appNameW, MB_ICONWARNING);
			return;
		}

		VrClient* viewer = new VrClient();
		viewer->m_caption  = caption;
		viewer->m_remoteId = remoteID;
		viewer->m_host     = strRemoteHost;

		// set profile & encoder
		{
			int i = m_wndSpeed.GetCurSel();
			int profile = i - settings.m_encoders.size();
			if (profile<0) {
				viewer->m_profile = InteropViewer::VrDesktop;
			}
			else {
				viewer->m_profile = profile;
				i = 0; // set first encoder, need for VrAudioChat profile
			}
			viewer->m_encoder = settings.m_encoders[i];
			viewer->m_encoder.name = L""; // no need name there
		}

		viewer->m_opts.m_allowRemoteControl = !viewOnly;
		viewer->m_opts.m_allowClipboardOut  = !viewOnly;
		viewer->m_opts.m_allowClipboardIn   = !viewOnly;
		viewer->Thread01_start();
	}
	catch(RLException& ex) {
		_log.WriteError(ex.GetDescription());
		::MessageBox(m_hWnd, ex.GetDescription(), TheApp.m_appName, MB_ICONWARNING);
	}
}

void DlgMain::OnSelchangeType()
{
	bool on = m_wndSpeed.GetCurSel() < settings.m_encoders.size();
	m_wndViewOnly.ShowWindow1(on);
}


void DlgMain::OnClose()
{
	OnBtnStop();
	InteropCommon::m_hErrorWnd = NULL;
	::Shell_NotifyIcon(NIM_DELETE, &m_nid); // destroy icon

	bool     operActive   = true;
	UINT16   operProtocol = m_wndSpeed.GetCurSel();
	CStringA operRemoteId = m_wndRemoteIdEdit.GetTextA();
	bool     viewOnly     = m_wndViewOnly.GetCheckBool();

	
	if (settings.m_operProtocol!= operProtocol || 
		settings.m_operRemoteId!= operRemoteId ||
		settings.m_viewOnly    != viewOnly)
	{
		settings.m_operProtocol = operProtocol;
		settings.m_operRemoteId = operRemoteId;
		settings.m_viewOnly     = viewOnly;

		try {
			settings.Save();
		}
		catch(RLException& ex) {
			::MessageBox(m_hWnd, ex.GetDescription(), "Ammyy Admin", MB_ICONERROR);
		}
	}

	try {
		VrMain::OnExitProcess();
	}
	catch(RLException& ex) {
		::MessageBox(m_hWnd, ex.GetDescription(), "Ammyy Admin", MB_ICONERROR);
	}
}



void DlgMain::GetMenuText(HMENU hMenu, int menuCmd, WCHAR* buffer, int bufferLen)
{
	MENUITEMINFOW mii = {0};	
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING;
	mii.dwTypeData = buffer;
    mii.cch = bufferLen;	// in chars
	mii.wID = menuCmd;
	::GetMenuItemInfoW(hMenu, menuCmd, FALSE, &mii);
}

LRESULT DlgMain::OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT* mis)
{
	WCHAR text[256];
	GetMenuText(m_hMenu, mis->itemID, text, COUNTOF(text));

	HDC hdc = ::GetDC(hwnd);
	HGDIOBJ prevFont = ::SelectObject(hdc, m_menu.hFont);

	SIZE size;
	::GetTextExtentPoint32W(hdc, text, wcslen(text), &size);
	mis->itemWidth  = size.cx;
	mis->itemHeight = size.cy;

	::SelectObject(hdc, prevFont);
	::ReleaseDC(hwnd, hdc);
	return TRUE;
}

static void MyTextOut(HDC hdc, RECT rr, LPCWSTR text, bool center)
{
	SIZE size;
	int len = wcslen(text);
	::GetTextExtentPoint32W(hdc, text, len, &size);
	int x = rr.left + (rr.right  - rr.left - size.cx)/2;
	int y = rr.top  + (rr.bottom - rr.top  - size.cy)/2 - 1;

	if (!center) {
		x = rr.right - size.cx - 20;
	}

	::ExtTextOutW(hdc, x, y, ETO_OPAQUE, &rr, text, len, NULL);
}

LRESULT DlgMain::OnDrawItem(DRAWITEMSTRUCT* dis)
{
	if (dis->hwndItem==m_wndBtnContactBook) {
		m_wndBtnContactBook.DrawItem(dis);
	}
	else if (dis->CtlType==ODT_MENU) 
	{
		WCHAR text[256];
		this->GetMenuText(m_hMenu, dis->itemID, text, COUNTOF(text));

		HDC hdc = dis->hDC;

		COLORREF backColor = RGB(221,221,221);

		COLORREF c1, c2;

		// Set the appropriate foreground and background colors. 
		if (dis->itemState & (ODS_SELECTED | ODS_HOTLIGHT))
		{ 
			c1 = ::GetSysColor(COLOR_MENU);
			c2 = RGB(49,106,197); // blue in Windows XP
		}
		else
		{
			c1 = ::GetSysColor(COLOR_MENUTEXT);
			c2 = backColor;
		} 

		c1 = ::SetTextColor(hdc, c1);
		c2 = ::SetBkColor  (hdc, c2);

		RECT rr = dis->rcItem;

		MyTextOut(hdc, rr, text, true);

		if(dis->itemData==1)	//draw last empty menuItem as background
		{
			rr.left = rr.right;
			rr.right = m_cx + ::GetSystemMetrics(SM_CXFIXEDFRAME);	//do not overdraw window border
			::SetTextColor(hdc, RGB(224,0,0));
			::SetBkColor(hdc, backColor);
			//::ExtTextOutW(hdc, nTextX, nTextY, ETO_OPAQUE, &rr, NULL, 0, NULL);
		
			CStringW text;
			if (settings.IsFreeLicense())
				//text = "Free license (for non-commercial use only!)";
				text = "Free license (for home use only!)";
			MyTextOut(hdc, rr, text, false);
		}	

		// restore the original colors
		::SetTextColor(hdc, c1);
		::SetBkColor  (hdc, c2);
	}
	return TRUE;
}


void DlgMain::OnClickYourID()
{
	if (TheApp.m_ID!=0) return; // already have ID

	const int CMD_GET_ID = 3;

	if (m_hMenuForID==NULL) {
		m_hMenuForID = ::CreatePopupMenu();
		::AppendMenu(m_hMenuForID, MF_STRING, CMD_GET_ID, "Get ID");
	}

	POINT p;
	::GetCursorPos(&p);

	int ret = ::TrackPopupMenu(m_hMenuForID, TPM_RETURNCMD, p.x, p.y, 0, m_hWnd, NULL);
	if (ret==CMD_GET_ID) {
		SendInitCmd2();
	}
}


LRESULT DlgMain::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{	
	if(msg == WM_TRAYNOTIFY)
	{ 
		if (lParam==WM_LBUTTONDOWN) {
			::ShowWindow(m_hWnd, SW_NORMAL);
			::SetForegroundWindow(m_hWnd);
			::SetFocus(m_hWnd);
			return TRUE; 
		}
	} 	
	else if (msg == WM_SIZE) {
		if (wParam==SIZE_MINIMIZED) {
			::ShowWindow(m_hWnd, SW_HIDE);
			return TRUE;
		}
	}	
	else if (msg == WM_CLOSE) {
		OnClose();
	}	
	else if (msg==WM_TIMER) {
		OnTimer((UINT)wParam);
		return 0;
	}	
	else if (msg==WM_SYSCOMMAND) {
		OnSysCommand(wParam, lParam);
		//return 0;	// if uncomment it, Close won't work
	}		
	else if (msg==WM_COMMAND) 
	{
		WORD id = LOWORD(wParam);

		if (id==IDOK) { 
			if (::GetFocus() == m_wndRemoteIdEdit) OnBtnConnect();
			return TRUE; 
		}

		if (id==IDCANCEL) return TRUE;	// block closing window by press escape 

		if (id==ID_MENU_SETTINGS)		{ OnBtnSettings();	  return TRUE; }
		if (id==ID_MENU_CONTACTBOOK)	{ OnBtnContactBook(); return TRUE; }
		if (id==ID_HELP_ABOUT)			{ OnHelpAbout();	  return TRUE; }
		if (id==ID_HELP_AMMYY_WEBSITE)	{ OnHelpWebsite();	  return TRUE; }
		if (id==ID_MENU_EXIT)			{ OnMenuExit();		  return TRUE; }
		if (id==IDB_START_STOP)			{ OnBtnStart();		  return TRUE; }
		if (id==IDB_STOP)				{ OnBtnStop();		  return TRUE; }
		if (id==IDB_CONNECT)			{ OnBtnConnect();	  return TRUE; }
		if (id==IDC_SPEED)				{ OnSelchangeType();  return TRUE; }

		if ((HWND)lParam==(HWND)m_wndBtnContactBook) { OnBtnContactBook(); return TRUE; } 
		
		if (id>=ID_LANGUAGE_ENGLISH && id<=ID_LANGUAGE_LAST) {
			OnChangeLanguage(id);
			return TRUE;
		}

		if (id==IDC_YOUR_ID) {
			if (HIWORD(wParam)==256) {
				OnClickYourID();
			}
			return TRUE;
		}

		if (id>=ID_SERVICE_INSTALL && id<=ID_SERVICE_REMOVE) {
			ServiceManager.RunCmd(m_hWnd, id-ID_SERVICE_INSTALL);
			return TRUE;
		}
	}	
	//else if (msg==WM_NOTIFY) 
	//{
	//	NMHDR* pNMHdr = (NMHDR*)lParam;
	//}	
	else if (msg==WM_INITMENUPOPUP) {
		OnInitMenuPopup((HMENU)wParam, LOWORD(lParam), HIWORD(lParam));
		return 0;
	}
	else if (msg==WM_QUERYDRAGICON) {
		return (INT_PTR)OnQueryDragIcon();
	}
//	else if (msg==WM_PAINT) {
//		OnPaint();
//		return 0;
//	}
	else if (msg==WM_DRAWITEM) {
		return this->OnDrawItem((DRAWITEMSTRUCT*)lParam);
	}
	else if (msg==WM_MEASUREITEM) {
		return this->OnMeasureItem(hwnd, (MEASUREITEMSTRUCT*)lParam);
	}
	else if (msg==WM_CTLCOLORSTATIC || msg==WM_CTLCOLOREDIT || msg == WM_CTLCOLORBTN) 
	{
		return this->OnCtlColor((HWND)lParam, (HDC)wParam);
	}
	else if (msg==WM_ERASEBKGND) {
		this->OnEraseBackground((HDC)wParam);
		return TRUE;
	}
	else if (msg==WM_DESTROY) {
		::PostQuitMessage(0);
		return 0;
	}


	return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}


BOOL DlgMain::OnEndDialog(BOOL ok)
{
	if (ok) { // pressed Enter
		OnBtnConnect();		
	}

	return FALSE; // prevent close by ESC or Enter
}

void DlgMain::OnBtnSettings() 
{
	try {
		CStringW caption = L"Ammyy - " + rlLanguages.GetValue(D_SETTINGS);
		RLPropertySheet dlg(caption);

		DlgSettingsPage3 p3(rlLanguages.GetValue(D_CLIENT),			  MAKEINTRESOURCEW(IDD_SETTINGS_CLIENT));
		DlgSettingsPage4 p4(rlLanguages.GetValue(D_OPERATOR),		  MAKEINTRESOURCEW(IDD_SETTINGS_OPERATOR));
		DlgSettingsPage1 p1(rlLanguages.GetValue(D_SETTINGS_COMMON),  MAKEINTRESOURCEW(IDD_SETTINGS_COMMON));
		DlgSettingsPage2 p2(rlLanguages.GetValue(D_SETTINGS_NETWORK), MAKEINTRESOURCEW(IDD_SETTINGS_NETWORK));
		dlg.AddPage(&p3);
		dlg.AddPage(&p4);
		dlg.AddPage(&p1);
		dlg.AddPage(&p2);

		bool useWAN = settings.m_useWAN;

		int count_encoders_old = settings.m_encoders.size();

		//CDlgSettings dlg;
		if (dlg.DoModal(m_hWnd)==IDOK) {
			settings.Save();
			TheApp.OnLogChange();

			if (useWAN!=settings.m_useWAN) {
				::MessageBox(m_hWnd, "You need to restart application for applying new settings", "Ammyy Admin", MB_OK | MB_ICONWARNING);
			}
		}
		FillConnectionType(count_encoders_old); // call it even was IDCANCEL
	}
	catch(RLException& ex) {
		::MessageBox(m_hWnd, ex.GetDescription(), "Error", MB_ICONERROR);
	}
}

void DlgMain::OnBtnContactBook()
{	
	if ((HWND)m_dlgContactBook==NULL) {
		m_dlgContactBook.m_pDlgMain = this;
		m_dlgContactBook.DoModeless(m_hWnd); //m_hWnd
	}
	::ShowWindow((HWND)m_dlgContactBook, SW_SHOW);
	::SetForegroundWindow(m_dlgContactBook);
	
	/*
	DlgContactBook dlg;
	dlg.DoModal(m_hWnd);

	if (dlg.m_connectToID.GetLength()!=0) {
		m_wndRemoteIdEdit.SetTextW(dlg.m_connectToID);
		OnBtnConnect();
	}
	*/
}


void DlgMain::OnInitMenuPopup(HMENU hMenu, UINT nIndex, BOOL bSysMenu)
{
	if (bSysMenu) return;

	//CStringA str;
	//str.Format("-%d %d %d %d\n", ::GetMenuItemCount(hMenu), nIndex, hMenu, bSysMenu);
	//::OutputDebugString(str);

	if (::GetMenuItemID(hMenu, 0)==ID_SERVICE_INSTALL) {
		bool bInstall , bStart, bStop, bRemove;
		ServiceManager.GetStatus(bInstall, bStart, bStop, bRemove);

		::EnableMenuItem(hMenu, 0, (bInstall ? MF_ENABLED : MF_GRAYED) | MF_BYPOSITION);
		::EnableMenuItem(hMenu, 1, (bStart   ? MF_ENABLED : MF_GRAYED) | MF_BYPOSITION);
		::EnableMenuItem(hMenu, 2, (bStop    ? MF_ENABLED : MF_GRAYED) | MF_BYPOSITION);
		::EnableMenuItem(hMenu, 3, (bRemove  ? MF_ENABLED : MF_GRAYED) | MF_BYPOSITION);

		return;
	}

	if (nIndex==1) // language menu
	{
		UINT nIndexMax = ::GetMenuItemCount(hMenu);

		for (int i=0; i<(int)nIndexMax; i++)
		{
			UINT uCheck = ((m_iLang==i) ? MF_CHECKED : MF_UNCHECKED) | MF_BYPOSITION;
			::CheckMenuItem(hMenu, i, uCheck);
		}
	}
}

void DlgMain::SetMenuText(HMENU hMenu, UINT uItem, UINT uTextIndex)
{
	CStringW text = rlLanguages.GetValue(uTextIndex);

	MENUITEMINFOW mii;
	memset(&mii, 0, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING;
	mii.dwTypeData = text.GetBuffer(0);
	mii.cch = text.GetLength();
	BOOL b = ::SetMenuItemInfoW(hMenu, uItem, TRUE, &mii);
	ASSERT(b!=FALSE);
}


void DlgMain::OnChangeLanguage(UINT nID)
{
	m_iLang = nID - ID_LANGUAGE_ENGLISH;

	settings.m_langId = RLLanguages::GetLangByIndex(m_iLang);

	rlLanguages.Select(m_iLang);
	
	// tool tips
	m_wndBtnContactBook.SetToolTipTextW(rlLanguages.GetValue(D_CONTACT_BOOK));

	// dialog controls
	{
		RLWnd* m_wndItems[] = { &m_wndBtnStart, &m_wndBtnStop, &m_wndBtnConnect, &m_wndViewOnly };
		static const UINT resIDs[] = { D_START, D_STOP, D_CONNECT, D_VIEW_ONLY};

		for (int i=0; i<sizeof(m_wndItems)/sizeof(m_wndItems[0]); i++)
		{	
			CStringW str = rlLanguages.GetValue(resIDs[i]);
			::SetWindowTextW(*m_wndItems[i], (LPCWSTR)str);
		}
	}
	
	// fill menu
	{
		HMENU hMenu = ::GetMenu(m_hWnd);
		ASSERT (hMenu!=NULL);
		hMenu = ::GetSubMenu(hMenu, 0);
		ASSERT(hMenu!=NULL);

		SetMenuText(hMenu, 0, D_SETTINGS);
		SetMenuText(hMenu, 1, D_CONTACT_BOOK);
		SetMenuText(hMenu, 2, D_SERVICE);
		SetMenuText(hMenu, 4, D_EXIT);
		
		HMENU hMenuService = ::GetSubMenu(hMenu, 2);
		ASSERT(hMenuService!=NULL);

		SetMenuText(hMenuService, 0, D_INSTALL);
		SetMenuText(hMenuService, 1, D_START);
		SetMenuText(hMenuService, 2, D_STOP);
		SetMenuText(hMenuService, 3, D_REMOVE);
	}

	m_dlgContactBook.OnChangeLanguage();

	this->CreateBackgroundImage();
	this->Invalidate();
}

void DlgMain::OnHelpAbout() 
{
	DlgAbout dlgAbout;	
	dlgAbout.DoModal(m_hWnd);
}

void DlgMain::OnHelpWebsite() 
{
	LPCTSTR url = "http://www.ammyy.com";
	::ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
}

void DlgMain::OnMenuExit() 
{
	::SendMessage(m_hWnd, WM_CLOSE, 0, 0);
}


//_______________________________________________________________________________________________


static void CreateCompexFrame1(Image& img, int x, int y, int cx, GIMPBMP& icon, int borderColor, int bkgndColor, int iconColor)
{
	int x1 = x;	
	int x2 = x1+frame1_l.cx;
	int x3 = x2+cx;

	img.DrawPicture2(frame1_l, x1, y,   borderColor, bkgndColor);
	img.DrawPicture2(frame1_r, x3, y+6, borderColor, bkgndColor);	
	img.StretchHorizontal(x3, x2, y+6, y+6+frame1_r.cy);
	img.DrawPicture1(icon, x1+8, y+5,  iconColor, Image::BLEND);
}

static HBITMAP CreateIndicator1(Image& imgBackground, int px, int py, UINT32 color)
{
	GIMPBMP& img_a = indicator01_a; // alpha
	GIMPBMP& img_b = indicator01_b; // brightness

	Image img(img_a.cx, img_a.cy);

	for(int x=0; x<img_a.cx; x++)
	{
	for(int y=0; y<img_a.cy; y++)
	{
		const UINT8 a = *img_a.GetPixelAddr(x, y);
		const UINT8 b = *img_b.GetPixelAddr(x, y);

		Color c1 = imgBackground.Get(px+x, py+y);
		Color c2 = Color(color)*b;
		c1 *= 255-a;
		c1 += c2*a;
		c1.a = 255;
		img.Set(x, y, c1);
	}
	}

	return img.GetBitmap();
}

static void StretchVertical(RLStream& out, const GIMPBMP& img, int cy, int h1)
{
	int bytesPerRow = img.cx * img.bytes_per_pixel;
	int size = bytesPerRow * cy;

	GIMPBMP* pOut = (GIMPBMP*)out.GetBuffer1(size+sizeof(GIMPBMP));
	pOut->bytes_per_pixel = img.bytes_per_pixel;
	pOut->cx = img.cx;
	pOut->cy = cy;
	pOut->pixel_data = (BYTE*)(pOut + 1);

	int h2 = cy - img.cy;
	int h3 = img.cy - h1;

	//h2 += h3; h3 = 0;

	BYTE* pDst = pOut->GetPixelAddr(0, 0);
	BYTE* pSrc =   img.GetPixelAddr(0, 0);
	memcpy(pDst, pSrc, bytesPerRow*h1);

	pDst += bytesPerRow*h1;
	pSrc += bytesPerRow*h1;

	for (;h2>0; h2--) {
		memcpy(pDst, pSrc, bytesPerRow);
		pDst += bytesPerRow;
	}

	memcpy(pDst, pSrc, bytesPerRow*h3);
}



static const COLORREF c1 = RGB(110,110,110);
//static const COLORREF c1 = RGB(128,128,128);


LRESULT DlgMain::OnCtlColor(HWND wnd, HDC hdc)
{
	if (wnd==m_wndViewOnly || wnd==m_wndStatus) 
	{
		RECT r;
		POINT p;
		::GetWindowRect(wnd, &r);
		p.x = r.left;
		p.y = r.top;
		::ScreenToClient(m_hWnd, &p);

		::SetBkMode(hdc, TRANSPARENT);
		::SetBrushOrgEx( hdc, -p.x, -p.y, NULL);

		if (wnd==m_wndStatus) {
			::SetTextColor(hdc, c1);
		}
		
		return (LRESULT)m_bkgndBrush;
	}
	else if (wnd==m_wndYourIdEdit) {
		::SetBkMode(hdc, TRANSPARENT);
		return (LRESULT)_GUICommon.m_brush[0];
	}
	else if (wnd==m_wndYourIpEdit) {
		::SetBkMode(hdc, TRANSPARENT);
		return (LRESULT)_GUICommon.m_brush[1];
	}
	else if (wnd==m_wndRemoteIdEdit) {
		::SetBkMode(hdc, TRANSPARENT);
		return (LRESULT)_GUICommon.m_brush[2];
	}
	else if (wnd==m_wndBtnStop || wnd==m_wndBtnStart || wnd==m_wndBtnConnect || wnd==m_wndBtnContactBook) {
		return (LRESULT)m_hNullBrush;
	}

	return 0;
}


static void TextOutMy1(HDC hdc, int x, int y, CStringW& text)
{
	::TextOutW(hdc, x, y, text, text.GetLength());
}

void DlgMain::CreateBackgroundImage()
{
	if (m_bkgndDC!=NULL) {
		::DeleteDC(m_bkgndDC);
		m_bkgndDC = NULL;
	}

	if (m_bkgndBrush!=NULL) {
		::DeleteObject(m_bkgndBrush);
		m_bkgndBrush = NULL;
	}

	HWND hWnd = m_hWnd;
	int w = m_cx;
	int h = m_cy;

	//Draw images into background
	int VerticalSeparator_color = Color(140,140,140,255).getInt();
	//int ID_IP_borderColor = Color(128,128,128,255).getInt();
	int borderColor = Color(127,157,185,255).getInt();
	int IP_fillColor = Color(212, 232, 246,0).getInt();
	int ID_fillColor = Color(210,245,222,0).getInt();
	int ID_IP_iconsColor = Color(96, 117, 108,255).getInt();

	Image img(m_cx, m_cy);
	img.Clear(0xFF0000);

	//Gradients draw
	int cy_main = h - 36;
	img.DrawGradientV(188,188, 0, 1);	//top separator	
	img.DrawGradientV(251,210, 1, cy_main);	//main
	img.DrawGradientV(178,178, cy_main+1, 1);	//bottom separator
	img.DrawGradientV(237,177, cy_main+2, h-cy_main-2);	//bottom

	// draw vertical delimeter
	{
		RLStream delimiter;
		StretchVertical(delimiter, mainwnd_vert_delimiter, cy_main, 88);
		img.DrawPicture1(*(GIMPBMP*)delimiter.GetBuffer(), 262,1, VerticalSeparator_color, Image::SUBTRACT);
	}


	// TODO: just tried to make 2 vertical cylinders
	//int cx1 = img.m_w/2;
	//img.DrawGradientH(251, 180, 0, cx1-0, 1, cy_main);
	//img.DrawGradientH(251, 180, cx1+0, img.m_w, 1, cy_main);

	const int y1 = 0;

	img.DrawRect(280, y1+69, 155+2, 22+2, Color(255,255,255,255).getInt()); //client's ID inside
	img.DrawBox (280, y1+69, 155+2, 22+2, borderColor);                     //client's ID border
	//img.DrawBox(0, 1, w-1, h-2,   Color(188,188,188,255).getInt());	//all window border

	CreateCompexFrame1(img, 6, y1+ 61, 100, icon_id, borderColor, ID_fillColor, ID_IP_iconsColor);
	CreateCompexFrame1(img, 6, y1+121, 180, icon_ip, borderColor, IP_fillColor, ID_IP_iconsColor);
	

	if ((HWND)m_wndLights[0]==NULL) // do it only once
	{
		UINT32 colorInG = Color(150,249,151).getInt();
		UINT32 colorInY = Color(251,253,139).getInt();
		UINT32 colorInR = Color(254,107,115).getInt();
		UINT32 colorInO = Color(247,247,247).getInt();

		int cx = indicator01_a.cx;
		int cy = indicator01_a.cy;
		Image img0, img1;

		int SIZE = 12;

		for (int i=0; i<COUNTOF(m_wndLights); i++)
		{
			UINT32 colorOn = colorInG;
			
			int x = 165;
			int y = y1+68 + SIZE*(i%2);

			if (i<10) {
					 if (i>=8) colorOn = colorInR;
				else if (i>=4) colorOn = colorInY;
			
				x += SIZE*(i/2);
			}
			else {
				colorOn = colorInY;
				x += 70;
				y += 60;
			}

			img.CopyTo(x, y, cx, cy, img0);
			img.CopyTo(x, y, cx, cy, img1);
			img0.DrawIndicator01(0, 0, colorInO);
			img1.DrawIndicator01(0, 0, colorOn);
			HBITMAP hBitmap0 = img0.GetBitmap();
			HBITMAP hBitmap1 = img1.GetBitmap();

			m_wndLights[i].Create3(x, y, cx, cy, m_hWnd, 1237+10+i, hBitmap0, hBitmap1);

			::DeleteObject(hBitmap0);
			::DeleteObject(hBitmap1);
		}
	}

	HBITMAP hBitmap = img.GetBitmap();
	m_bkgndBrush = ::CreatePatternBrush(hBitmap);
	ASSERT(m_bkgndBrush!=NULL);
	m_bkgndDC = ::CreateCompatibleDC(NULL);

	::SelectObject(m_bkgndDC, hBitmap);
	::DeleteObject(hBitmap);

	// Draw Text_______________________________________________________________________________________



	int bkMode = ::SetBkMode(m_bkgndDC, TRANSPARENT);
	HGDIOBJ hFont = ::SelectObject(m_bkgndDC, m_fonts[0]);
	::SetTextAlign(m_bkgndDC, TA_LEFT | TA_BOTTOM);
	::SetTextColor(m_bkgndDC, c1);

		// TODO: change ID to IP, need to check on other languages
	CStringW str1 = rlLanguages.GetValue(D_YOUR_ID);
	CStringW str2 = str1;
	
	// TODO: change ID to IP, need to check on other languages
	str2.Replace(L"ID", L"IP");

	TextOutMy1(m_bkgndDC, 34, y1+ 65, str1);
	TextOutMy1(m_bkgndDC, 34, y1+125, str2);
	TextOutMy1(m_bkgndDC, 280,y1+ 65, rlLanguages.GetValue(D_CLIENT_ID));

	::SelectObject(m_bkgndDC, m_fonts[2]);
	::SetTextAlign(m_bkgndDC, TA_CENTER | TA_BOTTOM);
	//::SetTextColor(m_bkgndDC, RGB(0,0,0));

	TextOutMy1(m_bkgndDC, 1*m_cx/4, 26, rlLanguages.GetValue(D_CLIENT)   + ". " + rlLanguages.GetValue(D_WAIT_FOR_SESSION));
	TextOutMy1(m_bkgndDC, 3*m_cx/4, 26, rlLanguages.GetValue(D_OPERATOR) + ". " + rlLanguages.GetValue(D_CREATE_SESSION));

	//::SetTextAlign(m_bkgndDC, TA_CENTER | TA_TOP);
	//::SelectObject(m_bkgndDC, m_fonts[0]);
	
	//TextOutMy1(m_bkgndDC, 1*m_cx/4, 22, rlLanguages.GetValue(D_WAIT_FOR_SESSION));
	//TextOutMy1(m_bkgndDC, 3*m_cx/4, 22, rlLanguages.GetValue(D_CREATE_SESSION));
	
	// restore values
	::SetBkMode(m_bkgndDC, bkMode);
	::SelectObject(m_bkgndDC, hFont);
}

void DlgMain::OnEraseBackground(HDC hdc)
{
	::BitBlt(hdc, 0, 0, m_cx, m_cy, m_bkgndDC, 0, 0, SRCCOPY);
}

void DlgMain::DoModal()
{
	HINSTANCE hInstance = 0;

	WNDCLASSW wc = {0};
	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)WindowProcStatic;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon	= 0;
	wc.hCursor	= ::LoadCursor((HINSTANCE) NULL, IDC_ARROW);
	wc.lpszMenuName =  NULL;
	wc.lpszClassName = m_pClassName;
	if (!::RegisterClassW(&wc))
		throw RLException("RegisterClass() error=%u", ::GetLastError());

	this->m_cx = 506;
	this->m_cy = 250;

	HMENU hmenu = ::LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU1));

	m_hWnd = ::CreateWindowExW(0, m_pClassName, L"", WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, m_cx, m_cy, NULL, hmenu, hInstance, NULL);
 	
	if (!m_hWnd)
		throw RLException("CreateWindowEx() error=%u", ::GetLastError());

	::SetWindowLongW(m_hWnd, GWL_USERDATA, (LONG)this);

	//resize window
	//::SetWindowPos(hWnd, 0,0,0, m_cx, m_cy, SWP_NOMOVE);
	SIZE s = this->GetClientSize();
	::SetWindowPos(m_hWnd, 0,0,0, m_cx + (m_cx-s.cx), m_cy + (m_cy-s.cy), SWP_NOMOVE);

	this->OnInitDialog();

	//this->Init(hWnd, CLIENT_W, CLIENT_H);

	::ShowWindow(m_hWnd, SW_SHOW);
	::UpdateWindow(m_hWnd);

	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) 
	{
		if(::IsDialogMessage(m_hWnd, &msg)) continue;	// for TAB
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	::DestroyMenu(hmenu);
}