// This is the main source for a VrClient object.
// It handles almost everything to do with a connection to a server.
// The decoding of specific rectangle encodings is done in separate files.

#include "stdafx.h"
#include "vrMain.h"
#include "../common/omnithread/omnithread.h"
#include "vrClient.h"
#include "commctrl.h"
#include <Tlhelp32.h>
#include "../main/resource.h"
#include "../main/RLLanguages.h"
#include "VrDlgPasswordInput.h"
#include "../target/TrKeyDef.h"
#include "../main/common.h"
#include "../main/AmmyyApp.h"
#include "../main/rsa/rsa.h"
#include "../main/TCP.h"
#include "../main/DlgEncoder.h"
#include "../target/TrMain.h"
#include "../target/TrClient.h"
#include "../Common/Common2.h"


#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

std::vector<void*>	VrClient::m_objects;
omni_mutex			VrClient::m_objectsLock;


const UINT WM_RDP_RUN = WM_USER+2;


// *************************************************************************
//  A Client connection involves two threads - the main one which sets up
//  connections and processes window messages and inputs, and a 
//  client-specific one which receives, decodes and draws output data 
//  from the remote server.
//  This first section contains bits which are generally called by the main
//  program thread.
// *************************************************************************


VrClient::VrClient()
{
	omni_mutex_lock l(m_objectsLock);
	m_objects.push_back(this);

	m_transport = new TransportTCP2();
	m_hWorkerThread = NULL;
	m_hwnd1 = NULL;
	m_hwnd2 = NULL;
	m_hwndscroll = NULL;
	m_hToolbar = NULL;
	m_hwndNextViewer = NULL;
	m_dormant = false;
	m_desktop_unavailable = false;
	
	m_killing = false;
	m_bCloseWindow = false;
	m_running = false;
	m_workThreadFinished = false;

	m_hScrollPos = 0; 
	m_vScrollPos = 0;

	m_cursor.set = false;
	m_cursor.x = 0;
	m_cursor.y = 0;

	m_pInterop    = this;

	m_dwUncommitedScreenUpdates = 0;

	// Create a buffer for various network operations
	m_netBuffer.GetBuffer1(4096);

	m_mouseMove.x = -1;
	m_mouseMove.y = -1;

	m_plcmnt_BeforeFullScreen.length = 0;
	m_desktop_on = false;
	m_audiochat_on = false;
}

void VrClient::Thread01()
{
	if (vrMain.m_pHotkeys==NULL)
		vrMain.m_pHotkeys = new VrHotKeys();
	
	if (vrMain.m_pHelp==NULL)
		vrMain.m_pHelp = new VrHelp();

	try 
	{
		this->Run();
				
		MSG msg;
		BOOL bRet;
		while ((bRet = GetMessage(&msg,NULL,0,0) )!=0)
		{
			if (bRet==-1) {
				_log2.Print(LL_WRN, VTCLOG("GetMessage() error=%d"), ::GetLastError());
				break;
			}

			if ( !vrMain.m_pHotkeys->TranslateAccel(&msg) && 
				 !vrMain.m_pHelp->TranslateMsg(&msg) )
			{				
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
		}
	} 
	catch(RLException& ex) {
		::MessageBox(NULL, ex.GetDescription(), "Ammyy Viewer", MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TOPMOST);
	}

	try {
		delete this;
	}
	catch(RLException& ex) {
		::MessageBox(NULL, ex.GetDescription(), "Ammyy Viewer", MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TOPMOST);
	}
}


void VrClient::ThrowError(int code=-1)
{
	if (code==-1) {
		aaTerminateMsg msg;
		ReadExact(((char*)&msg)+1, sizeof(msg)-1);
		code = msg.code;
	}

	if (code==Router_ComputerIsBusy) {
		CStringA string = rlLanguages.GetValueA(D_COMPUTER_BUSY) + CStringA(".\n") + rlLanguages.GetValueA(D_BUY_LICENSE);
		throw RLException((LPCSTR)string);
	}		
	else if (code == aaErrorAccessRejected) {
		LPCSTR string = rlLanguages.GetValueA(D_ACCESSREJECTED);
		throw RLException((LPCSTR)string);
	}
	else if (code == aaCloseSession || code == aaErrorSessionEnd)
	{
		LPCSTR string = rlLanguages.GetValueA(D_REMOTEPC_CLOSE);
		throw RLException((LPCSTR)string);
	}
	else if (code == aaErrorSessionInactive)
	{
		LPCSTR string = rlLanguages.GetValueA(D_SESSION_INACTIVE);
		throw RLException((LPCSTR)string);
	}
	else if (code == aaErrorRDPServer) 
	{
		throw RLException("Error while connect to RDP server on remote side");
	}
	else if (code == aaErrorInternalError)
	{
		throw RLException("Internal Error occurred on the remote side");
	}
	
	throw RLException("Unknow error=%d at OnErrorMsg()#2", code);
}


void VrClient::SetTittle(LPCSTR suffix)
{
	//CStringW str = m_pInterop->m_caption + L" - " + suffix;
	CStringW str = m_pInterop->m_caption;
	::SetWindowTextW(m_hwnd1, str);
}


void VrClient::Run()
{
	CreateDisplay();

	EnableFullControlOptions();

	m_desktop_cx = 400;
	m_desktop_cy = 200;
	m_size_is_set = false;

	m_desktop_cx = m_desktop_cx * m_opts.m_scale_den / m_opts.m_scale_num;
	m_desktop_cy = m_desktop_cy * m_opts.m_scale_den / m_opts.m_scale_num;

	SizeWindow(false);
	DoFullScreenMode(true);

	SetTittle("...");
	SetStatus("");

	// This starts the worker thread. The rest of the processing continues in run_undetached.
	//start_undetached();
	m_hWorkerThread = (HANDLE)_beginthreadex(NULL, 0, run_undetached_static, this, 0, NULL);
	if (m_hWorkerThread == NULL)
		throw RLException("Failed _beginthreadex() (errno = %d)", errno);
}


void VrClient::CreateDisplay() 
{
	WNDCLASSW wndclass;
	
	wndclass.style			= 0;
	wndclass.lpfnWndProc	= VrClient::WndProcDefW;
	wndclass.cbClsExtra		= 0;
	wndclass.cbWndExtra		= 0;
	wndclass.hInstance		= vrMain.m_hInstance;
	wndclass.hIcon			= TheApp.LoadMainIcon();
	wndclass.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground	= (HBRUSH) GetSysColorBrush(COLOR_BTNFACE);
    wndclass.lpszMenuName	= NULL;
	wndclass.lpszClassName	= L"AmmyyViewer";

	::RegisterClassW(&wndclass);

	wndclass.style			= 0;
	wndclass.lpfnWndProc	= VrClient::WndProcScroll;
	wndclass.cbClsExtra		= 0;
	wndclass.cbWndExtra		= 0;
	wndclass.hInstance		= vrMain.m_hInstance;
	wndclass.hIcon			= NULL;
	wndclass.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground	= (HBRUSH) GetStockObject(BLACK_BRUSH);
    wndclass.lpszMenuName	= NULL;
	wndclass.lpszClassName	= L"ScrollClass";

	::RegisterClassW(&wndclass);
	 
	wndclass.style			= 0;
	wndclass.lpfnWndProc	= VrClient::WndProcDefW;
	wndclass.cbClsExtra		= 0;
	wndclass.cbWndExtra		= 0;
	wndclass.hInstance		= vrMain.m_hInstance;
	wndclass.hIcon			= NULL;
	wndclass.hCursor		= NULL;
	wndclass.hbrBackground	= (HBRUSH) GetStockObject(BLACK_BRUSH);
    wndclass.lpszMenuName	= NULL;
	wndclass.lpszClassName	= L"ChildClass";

	::RegisterClassW(&wndclass);
	
	m_hwnd1 = ::CreateWindowW(L"AmmyyViewer", L"AmmyyViewer",
			      WS_BORDER|WS_CAPTION|WS_SYSMENU|WS_SIZEBOX| WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_CLIPCHILDREN,
			      CW_USEDEFAULT, CW_USEDEFAULT, 
				  CW_USEDEFAULT, CW_USEDEFAULT,
			      NULL,                // Parent handle
			      NULL,                // Menu handle
			      vrMain.m_hInstance,
			      NULL);

	::SetWindowLongW(m_hwnd1, GWL_USERDATA, (LONG) this);
	::SetWindowLongW(m_hwnd1, GWL_WNDPROC, (LONG)VrClient::WndProc1_static);
	::ShowWindow(m_hwnd1, SW_HIDE);

	// maximp : removed WS_BORDER, to m_hwndscroll has the same size as m_hwnd2
	m_hwndscroll = ::CreateWindowW(L"ScrollClass", NULL,
			      WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			      CW_USEDEFAULT, CW_USEDEFAULT,
				  CW_USEDEFAULT, CW_USEDEFAULT,
			      m_hwnd1,             // Parent handle
			      NULL,                // Menu handle
			      vrMain.m_hInstance,
			      NULL);
	::SetWindowLongW(m_hwndscroll, GWL_USERDATA, (LONG)this);
	::ShowWindow(m_hwndscroll, SW_HIDE);
	
	// Create a memory DC which we'll use for drawing to the local framebuffer
	m_hBitmapDC = ::CreateCompatibleDC(NULL);

	// Add stuff to System menu
	m_hMenu = ::GetSystemMenu(m_hwnd1, FALSE);

	AppendMenu(m_hMenu, MF_SEPARATOR, NULL, NULL);
	AppendMenu(m_hMenu, MF_STRING, IDC_OPTION1_BUTTON, "Connection &options");
	AppendMenu(m_hMenu, MF_STRING, IDC_OPTION2_BUTTON, "Connection &encoder");
	AppendMenu(m_hMenu, MF_STRING, ID_REQUEST_REFRESH, "Request screen &refresh\tCtrl-Alt-Shift-R");
	AppendMenu(m_hMenu, MF_SEPARATOR, NULL, NULL);
	AppendMenu(m_hMenu, MF_STRING, ID_FULLSCREEN,	   "&Full screen\tCtrl-Alt-Shift-F");
	AppendMenu(m_hMenu, MF_STRING, ID_TOOLBAR,		   "Show &toolbar");
	AppendMenu(m_hMenu, MF_SEPARATOR, NULL, NULL);
	AppendMenu(m_hMenu, MF_STRING, ID_CONN_CTLALTDEL,  "Send Ctrl-Alt-&Del");
	AppendMenu(m_hMenu, MF_STRING, ID_CONN_CTLESC,	   "Send Ctrl-Esc");
	AppendMenu(m_hMenu, MF_STRING, ID_CONN_CTLDOWN,	   "Ctrl key down");
	AppendMenu(m_hMenu, MF_STRING, ID_CONN_ALTDOWN,	   "Alt key down");	

	//AppendMenu(hsysmenu, MF_SEPARATOR, NULL, NULL);

	DrawMenuBar(m_hwnd1);

	CreateToolbar();

	m_hwnd2 = ::CreateWindowW(L"ChildClass", NULL,
			      WS_CHILD | WS_CLIPSIBLINGS,
			      CW_USEDEFAULT, CW_USEDEFAULT,
			      CW_USEDEFAULT, CW_USEDEFAULT, // x,y-size
			      m_hwndscroll,			// Parent handle
			      NULL,					// Menu handle
			      vrMain.m_hInstance,
			      NULL);
	
	m_audioOu.m_hOwnerWnd = m_hwnd2;
	vrMain.m_pHotkeys->SetWindow(m_hwnd1);
    ::ShowWindow(m_hwnd2, SW_HIDE);
		
	::SetWindowLongW(m_hwnd2, GWL_USERDATA, (LONG) this);
	::SetWindowLongW(m_hwnd2, GWL_WNDPROC,  (LONG)VrClient::WndProc2_static);
	
	CheckMenuItem(m_hMenu, ID_TOOLBAR, MF_BYCOMMAND|MF_CHECKED);	
	
	// record which client created this window
	
	// We want to know when the clipboard changes, so
	// insert ourselves in the viewer chain. But doing
	// this will cause us to be notified immediately of
	// the current state.
	// We don't want to send that.
	m_hwndNextViewer = ::SetClipboardViewer(m_hwnd2);
}

void VrClient::CreateToolbar()
{
	const int MAX_TOOLBAR_BUTTONS = 20;
	TBBUTTON but[MAX_TOOLBAR_BUTTONS];
	memset(but, 0, sizeof(but));
	int i = 0;

	but[i].iBitmap		= 0;
	but[i].idCommand	= IDC_OPTION1_BUTTON;
	but[i].fsState		= TBSTATE_ENABLED;
	but[i++].fsStyle	= TBSTYLE_BUTTON;

	but[i].iBitmap		= 1;
	but[i].idCommand	= IDC_OPTION2_BUTTON;
	but[i].fsState		= TBSTATE_ENABLED;
	but[i++].fsStyle	= TBSTYLE_BUTTON;

	but[i++].fsStyle	= TBSTYLE_SEP;

	but[i].iBitmap		= 3;
	but[i].idCommand	= IDD_FILE_MANAGER;
	but[i].fsState		= TBSTATE_INDETERMINATE;
	but[i++].fsStyle	= TBSTYLE_BUTTON;

	but[i].iBitmap		= 11;
	but[i].idCommand	= ID_AUDIO_CHAT;
	but[i].fsState		= TBSTATE_ENABLED;
	but[i++].fsStyle	= TBSTYLE_CHECK;

	but[i].iBitmap		= 2;
	but[i].idCommand	= IDC_DESKTOP_BUTTON;
	but[i].fsState		= TBSTATE_ENABLED;
	but[i++].fsStyle	= TBSTYLE_CHECK;

	but[i++].fsStyle	= TBSTYLE_SEP;

	but[i].iBitmap		= 4;
	but[i].idCommand	= ID_FULLSCREEN;
	but[i].fsState		= TBSTATE_ENABLED;
	but[i++].fsStyle	= TBSTYLE_BUTTON;

	but[i].iBitmap		= 5;
	but[i].idCommand	= ID_REQUEST_REFRESH;
	but[i].fsState		= TBSTATE_ENABLED;
	but[i++].fsStyle	= TBSTYLE_BUTTON;

	but[i].iBitmap		= 8;
	but[i].idCommand	= ID_CONN_CTLDOWN;
	but[i].fsState		= TBSTATE_ENABLED;
	but[i++].fsStyle	= TBSTYLE_CHECK;

	but[i].iBitmap		= 7;
	but[i].idCommand	= ID_CONN_CTLESC;
	but[i].fsState		= TBSTATE_ENABLED;
	but[i++].fsStyle	= TBSTYLE_BUTTON;

	but[i].iBitmap		= 9;
	but[i].idCommand	= ID_CONN_ALTDOWN;
	but[i].fsState		= TBSTATE_ENABLED;
	but[i++].fsStyle	= TBSTYLE_CHECK;

	but[i].iBitmap		= 6;
	but[i].idCommand	= ID_CONN_CTLALTDEL;
	but[i].fsState		= TBSTATE_ENABLED;
	but[i++].fsStyle	= TBSTYLE_BUTTON;

	/*
	but[i++].fsStyle	= TBSTYLE_SEP;

	but[i].iBitmap		= 10;
	but[i].idCommand	= ID_DISCONNECT;
	but[i].fsState		= TBSTATE_ENABLED;
	but[i++].fsStyle	= TBSTYLE_BUTTON;
	*/

	int numButtons = i;
	ASSERT(numButtons <= MAX_TOOLBAR_BUTTONS);	
	
	m_hToolbar = ::CreateToolbarEx(m_hwnd1,
		WS_CHILD | TBSTYLE_TOOLTIPS | WS_CLIPSIBLINGS | TBSTYLE_FLAT,
		ID_TOOLBAR, 12, vrMain.m_hInstance,
		IDB_BITMAP1, but, numButtons, 0, 0, 0, 0, sizeof(TBBUTTON));
	
	if (m_hToolbar != NULL) {
		SendMessage(m_hToolbar, TB_SETINDENT, 4, 0);

		m_hwndToolBarLine = ::CreateWindowW(L"ScrollClass", NULL,
			      WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER,
			      CW_USEDEFAULT, CW_USEDEFAULT,
				  CW_USEDEFAULT, CW_USEDEFAULT,
			      m_hToolbar,             // Parent handle
			      NULL,                // Menu handle
			      vrMain.m_hInstance,
			      NULL);
	}
}

void VrClient::SetStateToolbarButton(int id, bool enable, bool checked)
{
	UINT   lParam2 = (enable) ? TBSTATE_ENABLED : TBSTATE_INDETERMINATE;
	if (enable && checked) lParam2 |= TBSTATE_CHECKED;

	::SendMessage(m_hToolbar, TB_SETSTATE, (WPARAM)id, (LPARAM)lParam2);
}

void VrClient::EnableAction(int id, bool enable, bool checked)
{
	WPARAM wParam1 = (enable) ? MF_BYCOMMAND | MF_ENABLED : MF_BYCOMMAND | MF_GRAYED;
	
	::EnableMenuItem(m_hMenu, id, wParam1);
	this->SetStateToolbarButton(id, enable, checked);
}

void VrClient::EnableFullControlOptions()
{
	bool desktop     = m_prm.Get(Permission::ViewScreen);
	bool fileManager = m_prm.Get(Permission::FileManager);	 // target allows access to it's file system
	bool audioChat   = m_prm.Get(Permission::AudioChat);	 // target allows using Audio Chat
	bool remoteInput  = m_opts.m_allowRemoteControl;

	EnableAction(IDC_OPTION1_BUTTON, m_running);
	EnableAction(IDC_OPTION2_BUTTON, m_running && desktop);
	EnableAction(IDD_FILE_MANAGER,   m_running && fileManager);
	EnableAction(IDC_DESKTOP_BUTTON, m_running && desktop,   m_desktop_on);
	EnableAction(ID_AUDIO_CHAT,		 m_running && audioChat, m_audiochat_on);
	EnableAction(ID_FULLSCREEN,		 true); // m_running && m_desktop_on || m_opts.m_FullScreen;
	EnableAction(ID_REQUEST_REFRESH, m_running && m_desktop_on);
	EnableAction(ID_CONN_CTLALTDEL,  m_running && m_desktop_on && remoteInput);
	EnableAction(ID_CONN_CTLDOWN,	 m_running && m_desktop_on && remoteInput);
	EnableAction(ID_CONN_ALTDOWN,	 m_running && m_desktop_on && remoteInput);
	EnableAction(ID_CONN_CTLESC,	 m_running && m_desktop_on && remoteInput);
}


void VrClient::SwitchOffKey()
{
	if (!m_running || !m_desktop_on) return;
	if (!m_opts.m_allowRemoteControl) return;

	CheckMenuItem(m_hMenu, ID_CONN_ALTDOWN, MF_BYCOMMAND|MF_UNCHECKED);
	CheckMenuItem(m_hMenu, ID_CONN_CTLDOWN, MF_BYCOMMAND|MF_UNCHECKED);
	this->SetStateToolbarButton(ID_CONN_CTLDOWN, true);
	this->SetStateToolbarButton(ID_CONN_ALTDOWN, true);

	SendKeyEvent(XK_Alt_L,     false);
	SendKeyEvent(XK_Control_L, false);
	SendKeyEvent(XK_Shift_L,   false);
	SendKeyEvent(XK_Alt_R,     false);
	SendKeyEvent(XK_Control_R, false);
	SendKeyEvent(XK_Shift_R,   false);
}


void VrClient::DoCrypt()
{	
	// read sign AND aaPingRequest (optional, very rare case)
	m_transport->m_decryptor_on = false;
	{
		BYTE b = m_transport->ReadByte();
		if  (b==aaPingRequest)
			 b = m_transport->ReadByte();

		if (b!=aaPreInitMsg_v3[0]) throw INVALID_PROTOCOL;
	}
	m_transport->m_decryptor_on = true;


	if (m_pInterop->m_encryption==Settings::Encryption::None)
	{
		m_transport->SetKey(NULL);
	}
	else	
	{
		BYTE key[20];

		if (m_pInterop->m_encryption==Settings::Encryption::DynamicKeyAndRSA)
		{		
			BYTE keyEncrypted[128];

			m_transport->ReadExact(keyEncrypted, sizeof(keyEncrypted));

			m_pInterop->m_pRSA->Decrypt(RSA_PRIVATE, sizeof(key), keyEncrypted, key);
		}
		else // Settings::Encryption::DynamicKey
		{ 
			m_transport->ReadExact(key, sizeof(key));
		}
		m_transport->SetKey(key);
	}

	m_pInterop->FreeRSA();
}


void VrClient::DoAuthorization()
{
	m_prm.ClearAll();

	bool first = true;

	while(true)
	{
		UINT8 status;
		m_transport->ReadExact(&status, sizeof(status));

		if (status==aaAuthorizationOK) break;

		if (status==aaError) ThrowError();

		if (status!=aaAuthorizationNeedPassword) throw INVALID_PROTOCOL;

		UINT32 remoteId = m_pInterop->GetRemoteId();
		
		VrDlgPasswordInput dlg;
		dlg.m_first = first;
		dlg.m_computer = (remoteId==0) ? m_host : CCommon::ConvertIDToString(remoteId, "NOT PROVIDED");
		if (dlg.DoModal(m_hwnd1) == IDOK) {
			BYTE buffer[1+16];
			buffer[0] = 0; // means OK
			RLMD5 password;
			password.Calculate(dlg.m_password);
			memcpy(&buffer[1], password.hash, 16);			
			m_transport->SendExact(buffer, sizeof(buffer), TRUE);
			first = false;
		}
		else {
			UINT8 flag = 0xFF;
			m_transport->SendExact(&flag, sizeof(flag), TRUE);
			ThrowError(aaErrorAccessRejected); // throw exception
		}	
	}

	m_transport->ReadExact(&m_prm.m_values, sizeof(m_prm.m_values));

	// no need to aaTerminateMsg here cause Target will close this connection first
	if (m_prm.IsClear()) ThrowError(aaErrorAccessRejected);

	m_opts.m_prm = m_prm;

	if (m_opts.m_allowRemoteControl) m_opts.m_allowRemoteControl = m_prm.Get(Permission::RemoteControl);
	if (m_opts.m_allowClipboardOut ) m_opts.m_allowClipboardOut  = m_prm.Get(Permission::ClipboardOut);
	if (m_opts.m_allowClipboardIn  ) m_opts.m_allowClipboardIn   = m_prm.Get(Permission::ClipboardIn);
}

void VrClient::CheckPermissions()
{
	bool ok = false; // no need '=false' but for evidence

	switch (m_pInterop->m_profile)
	{
		case InteropViewer::VrSpeedTestOnly:   ok = true; break; // ANY permisission
		case InteropViewer::VrAudioChat:       ok = m_prm.Get(Permission::AudioChat  ); break;
		case InteropViewer::VrRDP:             ok = m_prm.Get(Permission::RDPsession ); break;
		case InteropViewer::VrFileManagerOnly: ok = m_prm.Get(Permission::FileManager); break;
		default:
			ok = m_prm.Get(Permission::ViewScreen);
	}
	
	if (!ok) {
		aaTerminateMsg msg;
		msg.type = aaError;
		msg.code = aaCloseSession;		
		m_transport->SendExact(&msg, sizeof(msg), TRUE);
		ThrowError(aaErrorAccessRejected); // throw exception
	}
}



void VrClient::ReadOtherInfo()
{
	RLStream stream;
	m_transport->ReadStream16(stream);

	CStringA os_type    = stream.GetString1A();
	CStringA os_version = stream.GetString1A();
	m_computerName      = stream.GetString1A();

	m_os_description = os_type + " " + os_version;

	int i = m_computerName.ReverseFind('\n');

	if (i>=0) {
		m_appBuild     = m_computerName.Mid(i+1);
		m_computerName = m_computerName.Mid(0, i);
	}

	SetTittle(m_computerName);
	
	_log2.Print(LL_INF, VTCLOG("Remote computer name \"%s\""),m_computerName);
}


// ____________________________________ DirectTCP _________________________________________________


void VrClient::MakeConnectionKey(RLStream& bufferOut, UINT64 time1)
{
	char* buffer = bufferOut.GetBufferWr();

	memcpy(buffer, &time1, 8);

	for (int i=0; i<8; i++)
		buffer[i] ^= 0xFF;

	CCommon::FillRandom(&buffer[8], 8);

	bufferOut.AddRaw(NULL, 16);
}

bool VrClient::IsIPv4Included(const RLStream& ips, UINT32 ip)
{
	int count = ips.GetLen() / 4;
	for (int i=0; i<count; i++) {
		UINT32 ip1 = ((UINT32*)ips.GetBuffer())[i];
		if (ip1==ip) return true;
	}
	return false;
}

void VrClient::PushBackPort(RLStream& ports, UINT16 port)
{
	int count = ports.GetLen() / 2;
	for (int i=0; i<count; i++) {
		UINT16 port1 = ((UINT16*)ports.GetBuffer())[i];
		if (port1==port) return; // already exist
	}
	ports.AddUINT16(port);
}

int VrClient::DoDirectConnectOutgoing(DirectConnect& dc, UINT32 ip, UINT16 port)
{
	if (CCommon::TimeIsReady(dc.dwTicksToStop)) return 1; // time is finished

	_log.WriteInfo("DoDirectConnectOutgoing#1 %s, %u", (LPCSTR)Common2::IPv4toString(ip), (int)port);

	RLStream buffer(32);

	// only first time
	if (!dc.tryed) {
		buffer.AddUINT8(aaDirectConnect);
		dc.tryed = true;
	}
				
	buffer.AddUINT8(aaDcWaitTCP);	
	const char* key = buffer.GetBufferWr();
	MakeConnectionKey(buffer, this->m_router_time);
	buffer.AddUINT16(port);

	RLTimer timer;

	m_transport->SendExact(buffer.GetBuffer(), buffer.GetLen(), TRUE);
	UINT8 msg = m_transport->ReadByte();

	dc.roundTrip = timer.GetElapsedSeconds()*1000;

	if (msg==aaDcFailed) return 3; // partner couldn't listen this port
	
	if (msg==aaDcOK) 
	{
		UINT timeout = dc.GetTimeOut();

		TCP tcp;
		UINT8 reply = (TrClient::DirectConnectByTCP(tcp, ip, DEFAULT_INCOME_PORT, timeout, key)) ? aaDcOK : aaDcFailed;
		m_transport->SendByte(reply, TRUE);

		_log.WriteInfo("DoDirectConnectOutgoing#2 %u", (int)reply);

		if (reply==aaDcOK)
		{
			msg = m_transport->ReadByte();
			if (msg==aaDcOK) {
				// good, let's change transport						
				m_transport->SetTCPSocket(tcp.Detach(), true);
				return 0; // ok
			}
			else if (msg!=aaDcFailed) throw INVALID_PROTOCOL;
		}
		return 2; // other problems
	}
	
	throw INVALID_PROTOCOL;
}


int VrClient::DoDirectConnectIncoming(DirectConnect& dc, UINT32 ip, UINT16 port)
{
	if (CCommon::TimeIsReady(dc.dwTicksToStop)) return 1; // time is finished

	RLStream buffer(32);

	// only first time
	if (!dc.tryed) {
		buffer.AddUINT8(aaDirectConnect);
		dc.tryed = true;
	}

	if (dc.roundTrip<0 && (!dc.fast)) {
		buffer.AddUINT8(aaDcPing);
		RLTimer timer;
		m_transport->SendExact(buffer.GetBuffer(), buffer.GetLen(), TRUE);
		UINT8 msg = m_transport->ReadByte();
		dc.roundTrip = timer.GetElapsedSeconds()*1000;

		if (msg!=aaDcPingReply) throw INVALID_PROTOCOL;
		buffer.Reset();
	}

	_log.WriteInfo("DoDirectConnectIncoming#1 %s, %u", (LPCSTR)Common2::IPv4toString(ip), (int)port);

	UINT timeout = dc.GetTimeOut();

	buffer.AddUINT8(aaDcConnect);
	const char* key = buffer.GetBufferWr();
	MakeConnectionKey(buffer, this->m_router_time);
	buffer.AddUINT32(timeout);			// timeout
	buffer.AddUINT32(ip);
	buffer.AddUINT16(port);
	m_transport->SendExact(buffer.GetBuffer(), buffer.GetLen(), TRUE);

	if (TrClient::DirectConnectWaitTCP(key, m_transport)) return 0; //ok
	
	return 2;
}


// 1. In the same network
//  1.A client <- operator
//  1.B client -> operator
//
// 2. Different networks or couldn't make direct conection in the same network
//  2.A client <- operator, if [client   has external IP or directIn ports]
//  2.B client -> operator, if [operator has external IP or directIn ports]
//  2.C UDT - not implemented yet!!!


void VrClient::DoDirectConnect()
{
	if (m_transport->m_direct) return;

	UINT8  flags;
	ReadExact(&flags, sizeof(flags));

	if ((flags&1)==0) return; // not allow DirectTCP  // DirectUDT

	bool p_proxy = (flags&4)!=0;
	bool t_proxy = RLHttp::m_proxyPort!=0;

	UINT32 p_extIP;				// partner external IP
	UINT16 p_intPort;           // partner internal TCP port
	RLStream p_intIPs;			// partner internal IPs
	RLStream p_directInPorts;

	ReadExact(&p_extIP, sizeof(p_extIP));
	m_transport->ReadStream16(p_intIPs);
	ReadExact(&p_intPort, sizeof(p_intPort));
	m_transport->ReadStream16(p_directInPorts);
	
	if (!settings.m_allowDirectTCP) return; // not allowing by settings

	m_cmdSessionEnded.m_time = this->m_router_time;
	m_cmdSessionEnded.m_ip_v = Common2::IPv4toString(m_external_ip);
	m_cmdSessionEnded.m_ip_t = Common2::IPv4toString(p_extIP);

	SetStatus("Trying to establish direct connection");

	DirectConnect dc;
	dc.dwTicksToStop = ::GetTickCount() + 5000; // in 5 seconds stop any attemps
	dc.tryed = false;
	dc.roundTrip = -1; // means not set


	// if both use proxy or both not use proxy, should be in local network
	bool tryInTheSameNetwork = (p_extIP == m_external_ip) && (p_proxy==t_proxy);
	
	if (tryInTheSameNetwork)
	{
		dc.fast = true;

		// 1.A		
		{
			UINT16 port = p_intPort;
			int countIPs = p_intIPs.GetLen() / 4;
			for (int i=0; i<countIPs; i++) 
			{
				UINT32 partner_intIP = ((UINT32*)p_intIPs.GetBuffer())[i];

				int v = DoDirectConnectOutgoing(dc, partner_intIP, port);

				if (v==0 || v==1) return; // ok || time is finished
				if (v==3) break;  // partner couldn't listen this port, so no need to try with other addresses
			}
		}
		
		// 1.B
		{
			for (int i=0; i<1; i++) // only one attempt
			{
				UINT16 port= TrListener::GetIntranetPort();

				TrListenerWrapper listener(port);
				if (!listener.InOpened()) {
					_log.WriteInfo("DoDirectConnect() couldn't listen port %d", (int)port);
					continue;
				}

				RLStream internal_ips;
				TCP::GetIPaddresses(internal_ips);

				int countIPs = internal_ips.GetLen() / 4;
				for (int i=0; i<countIPs; i++)
				{
					UINT32 internal_ip = ((UINT32*)internal_ips.GetBuffer())[i];

					int v = DoDirectConnectIncoming(dc, internal_ip, port);
					
					if (v==0 || v==1) return; // ok || time is finished
				}
			}
		}
	}

	dc.fast = false;

	// 2.A	
	if (!t_proxy) // if this computer doesn't use proxy
	{
		if (IsIPv4Included(p_intIPs, p_extIP) && (!tryInTheSameNetwork)) {
			PushBackPort(p_directInPorts, p_intPort);
		}

		int c = p_directInPorts.GetLen() / 2;
		for (int i=0; i<c; i++) {
			UINT16 p_port = ((UINT16*)p_directInPorts.GetBuffer())[i];

			int v = DoDirectConnectOutgoing(dc, p_extIP, p_port);

			if (v==0 || v==1) return; // ok || time is finished
		}
	}	

	// 2.B
	if (!p_proxy) // if partner's computer doesn't use proxy
	{
		RLStream directInPorts;
		TrClient::GetReadyDirectInPort(directInPorts);

		RLStream intIPs; // internal IPs
		TCP::GetIPaddresses(intIPs);

		if (IsIPv4Included(intIPs, m_external_ip) && (!tryInTheSameNetwork)) {
			PushBackPort(directInPorts, TrListener::GetIntranetPort());
		}

		int c = directInPorts.GetLen() / 2;
		for (int i=0; i<c; i++) {
			UINT16 port = ((UINT16*)directInPorts.GetBuffer())[i];

			TrListenerWrapper listener(port);

			if (listener.InOpened()) {
				int v = DoDirectConnectIncoming(dc, m_external_ip, port);

				if (v==0 || v==1) return; // ok || time is finished
			}
		}
	}
	
	if (dc.tryed) {
		m_transport->SendByte(aaDcExit, TRUE);
	}
}

// ___________________________________________________________________________________________________


void VrClient::SizeWindow(bool centered)
{
	RECT workrect;
	RLWnd::GetWorkArea(&workrect);
	int workCX = workrect.right  - workrect.left;
	int workCY = workrect.bottom - workrect.top;
	_log2.Print(LL_INF, VTCLOG("Screen work area is %d x %d"), workCX, workCY);

	RECT fullwinrect;
	fullwinrect.left = 0;
	fullwinrect.top  = 0;
	fullwinrect.right  = m_desktop_cx * m_opts.m_scale_num / m_opts.m_scale_den;
	fullwinrect.bottom = m_desktop_cy * m_opts.m_scale_num / m_opts.m_scale_den;

	//::AdjustWindowRectEx(&fullwinrect, 
	//	::GetWindowLong(m_hwnd2, GWL_STYLE ), FALSE,
	//	::GetWindowLong(m_hwnd2, GWL_EXSTYLE));

	m_fullwinCX = fullwinrect.right  - fullwinrect.left;
	m_fullwinCY = fullwinrect.bottom - fullwinrect.top;

	//::AdjustWindowRectEx(&fullwinrect, 
	//	::GetWindowLong(m_hwndscroll, GWL_STYLE ) & ~WS_HSCROLL & ~WS_VSCROLL & ~WS_BORDER, FALSE,
	//	::GetWindowLong(m_hwndscroll, GWL_EXSTYLE));
	
	::AdjustWindowRectEx(&fullwinrect, 
		::GetWindowLong(m_hwnd1, GWL_STYLE), FALSE,
		::GetWindowLong(m_hwnd1, GWL_EXSTYLE));

	if (::GetMenuState(m_hMenu, ID_TOOLBAR, MF_BYCOMMAND) == MF_CHECKED) {
		RECT rtb;
		::GetWindowRect(m_hToolbar, &rtb);
		fullwinrect.bottom += rtb.bottom - rtb.top - 2;
	}

	int fullwinCX = fullwinrect.right  - fullwinrect.left;
	int fullwinCY = fullwinrect.bottom - fullwinrect.top;


	if (fullwinCX>workCX) {
		int h = ::GetSystemMetrics(SM_CYHSCROLL);
		if (fullwinCY+ h <=workCY) fullwinCY+=h;	// expand size if posible for horizont scrollbar
	}
	
	if (fullwinCY>workCY) {
		int h = ::GetSystemMetrics(SM_CXVSCROLL);
		if (fullwinCX+ h <=workCX) fullwinCX+=h;	// expand size if posible for vertical scrollbar
	}

	int winCX = min(fullwinCX, workCX);
	int winCY = min(fullwinCY, workCY);
	
	int x,y;
	WINDOWPLACEMENT winplace;
	winplace.length = sizeof(WINDOWPLACEMENT);
	::GetWindowPlacement(m_hwnd1, &winplace);
	if (centered) {
		x = (workCX - winCX)/2;
		y = (workCY - winCY)/2;
	} else {
		// Try to preserve current position if possible		
		if ((winplace.showCmd == SW_SHOWMAXIMIZED) || (winplace.showCmd == SW_SHOWMINIMIZED)) {
			x = winplace.rcNormalPosition.left;
			y = winplace.rcNormalPosition.top;
		} else {
			RECT tmprect;
			::GetWindowRect(m_hwnd1, &tmprect);
			x = tmprect.left;
			y = tmprect.top;
		}
		if (x + winCX > workrect.right)	 x = workrect.right  - winCX;
		if (y + winCY > workrect.bottom) y = workrect.bottom - winCY;
	}
	winplace.rcNormalPosition.top  = y;
	winplace.rcNormalPosition.left = x;
	winplace.rcNormalPosition.right  = x + winCX;
	winplace.rcNormalPosition.bottom = y + winCY;
	::SetWindowPlacement(m_hwnd1, &winplace);
	::SetForegroundWindow(m_hwnd1);
	PositionChildWindow();
}

void VrClient::PositionChildWindow()
{	
	RECT rparent;
	::GetClientRect(m_hwnd1, &rparent);
	
	int parentCX = rparent.right - rparent.left;
	int parentCY = rparent.bottom - rparent.top;
				
	if (::GetMenuState(m_hMenu, ID_TOOLBAR, MF_BYCOMMAND) == MF_CHECKED) {
		RECT rtb;
		::GetWindowRect(m_hToolbar, &rtb);
		int tbCY = rtb.bottom - rtb.top - 2;
		::SetWindowPos(m_hToolbar,		  HWND_TOP, rparent.left, rparent.top,        parentCX, tbCY, SWP_SHOWWINDOW);
		::SetWindowPos(m_hwndToolBarLine, HWND_TOP, rparent.left, rparent.top+tbCY-3, parentCX, 1,    SWP_SHOWWINDOW);

		parentCY    -= tbCY;
		rparent.top += tbCY;

	} else {
		::ShowWindow(m_hToolbar, SW_HIDE);
	}
	
	::SetWindowPos(m_hwndscroll, HWND_TOP, rparent.left, rparent.top, parentCX, parentCY, SWP_SHOWWINDOW);

	if (!m_desktop_on) {
		::SetWindowPos(m_hwnd2, HWND_TOP, 0, 0, parentCX, parentCY, SWP_SHOWWINDOW);
		::InvalidateRect(m_hwnd2, NULL, FALSE);
		::UpdateWindow(m_hwnd2);
		return;
	}
	
	if (!m_opts.m_FitWindow) {
		if (m_opts.m_FullScreen) {
			m_autoScroll = true;
			::ShowScrollBar(m_hwndscroll, SB_HORZ, FALSE);
			::ShowScrollBar(m_hwndscroll, SB_VERT, FALSE);
		} else {
			::ShowScrollBar(m_hwndscroll, SB_VERT, parentCY<m_fullwinCY);
			::ShowScrollBar(m_hwndscroll, SB_HORZ, parentCX<m_fullwinCX);
			::GetClientRect(m_hwndscroll, &rparent);
			parentCX = rparent.right - rparent.left;
			parentCY = rparent.bottom - rparent.top;
			::ShowScrollBar(m_hwndscroll, SB_VERT, parentCY<m_fullwinCY);
			::ShowScrollBar(m_hwndscroll, SB_HORZ, parentCX<m_fullwinCX);
			::GetClientRect(m_hwndscroll, &rparent);	
			parentCX = rparent.right - rparent.left;
			parentCY = rparent.bottom - rparent.top;
			m_autoScroll = (parentCY<m_fullwinCY) || (parentCX<m_fullwinCX);
		}
	} else {
		if (!::IsIconic(m_hwnd1)) {
			::ShowScrollBar(m_hwndscroll, SB_HORZ, FALSE);
			::ShowScrollBar(m_hwndscroll, SB_VERT, FALSE);
			::GetClientRect(m_hwndscroll, &rparent);
			parentCX = rparent.right - rparent.left;
			parentCY = rparent.bottom - rparent.top;
			if ((parentCX < 1) || (parentCY < 1))
				return;
				
			int den = max(m_desktop_cx * 400 / parentCX,
						  m_desktop_cy * 400 / parentCY);

			while (true) {
				m_fullwinCX = (m_desktop_cx * 400 + den - 1) / den;
				m_fullwinCY = (m_desktop_cy * 400 + den - 1) / den;

				if ((m_fullwinCX<=parentCX) && (m_fullwinCY<=parentCY)) break;
					
				den++;
			}

			m_opts.SetScaling(400, den);
		}
		m_autoScroll = false;
	}

		
	int x = (parentCX>m_fullwinCX) ? (parentCX-m_fullwinCX)/2 : rparent.left;
	int y = (parentCY>m_fullwinCY) ? (parentCY-m_fullwinCY)/2 : rparent.top;

	m_clientCX = min( (int)parentCX, (int)m_fullwinCX);
	m_clientCY = min( (int)parentCY, (int)m_fullwinCY);
		
	// this causes wndproc*, so may be it's better changing m_clientCX/CY after SetWindowPos()
	::SetWindowPos(m_hwnd2, HWND_TOP, x, y, m_clientCX, m_clientCY, SWP_SHOWWINDOW);

	m_hScrollMax = m_fullwinCX;
	m_vScrollMax = m_fullwinCY;
           
	int newhpos, newvpos;
	if (!m_opts.m_FitWindow) {
		newhpos = max(0, min(m_hScrollPos, m_hScrollMax - max(m_clientCX, 0)));
		newvpos = max(0, min(m_vScrollPos, m_vScrollMax - max(m_clientCY, 0)));
	} else {
		newhpos = 0;
		newvpos = 0;
	}
	RECT clichild;
	GetClientRect(m_hwnd2, &clichild);
	::ScrollWindowEx(m_hwnd2, m_hScrollPos-newhpos, m_vScrollPos-newvpos, NULL, &clichild, NULL, NULL, SW_INVALIDATE);
									
	m_hScrollPos = newhpos;
	m_vScrollPos = newvpos;
	if (!m_opts.m_FitWindow) {
		this->UpdateScrollbars();
	} else {
		::InvalidateRect(m_hwnd2, NULL, FALSE);
	}
	::UpdateWindow(m_hwnd2);
}

void VrClient::FreeScreenBuffer()
{
	if (m_hBitmap != NULL) {
		::SelectObject(m_hBitmapDC, m_hBitmapOld);
		m_hBitmapOld = NULL;
		::DeleteObject(m_hBitmap);
		m_hBitmap = NULL;
	}
}


// this fucntion should be called only from thread which read remote screen in other case can be unhandled exeption
// because while we do FreeScreenBuffer(), other thread can write to screen data
//
void VrClient::CreateScreenBuffer() 
{
	RLMutexLock l(m_bitmapdcMutex);

	// Remove old bitmap object if it already exists
	this->FreeScreenBuffer();

	// We create a bitmap which has the same pixel characteristics as
	// the local display, in the hope that blitting will be faster.
	
	TempDC hdc(m_hwnd2);
	
	//m_hBitmap = ::CreateCompatibleBitmap(hdc, m_desktop_cx, m_desktop_cy);

	{
		BITMAPINFOHEADER infoHeader;
		infoHeader.biSize          = sizeof(infoHeader);
		infoHeader.biWidth         = m_desktop_cx;
		infoHeader.biHeight        = m_desktop_cy;
		infoHeader.biPlanes        = 1;
		infoHeader.biBitCount      = 32;
		infoHeader.biCompression   = BI_RGB;
		infoHeader.biSizeImage     = 0;
		infoHeader.biXPelsPerMeter = 0;
		infoHeader.biYPelsPerMeter = 0;
		infoHeader.biClrUsed       = 0;
		infoHeader.biClrImportant  = 0;

		m_pvBits = NULL;

		m_hBitmap = ::CreateDIBSection(m_hBitmapDC, (tagBITMAPINFO *)&infoHeader, 0, &m_pvBits, 0, 0);
	}
	
	if (m_hBitmap == NULL)
		throw RLException("Error %d creating local image of screen size %dx%d", ::GetLastError(), 
								(int)m_desktop_cx, (int)m_desktop_cy);

	m_hBitmapOld = (HBITMAP)::SelectObject(m_hBitmapDC, m_hBitmap);
}


void VrClient::DrawMessage(HDC hdc, LPCSTR text, COLORREF color)
{
	COLORREF oldbgcol  = ::SetBkColor  (hdc, RGB(0xcc, 0xcc, 0xcc));
	COLORREF oldtxtcol = ::SetTextColor(hdc, color);	
	
	RECT rect;
	::GetClientRect(m_hwnd2, &rect);
	//::SetRect(&rect, 0, 0, m_desktop_cx, m_desktop_cy);

	//CStringA str;
	//str.Format("DrawMessage#1 %u %u %u %u \n",rect.left, rect.right, rect.top, rect.bottom); 
	//::OutputDebugString(str);
	
	// fill background
	::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

	//mesure text
	RECT txtRect = rect;
	::DrawText (hdc, text, -1, &txtRect, DT_CENTER|DT_WORDBREAK|DT_CALCRECT);

	int cx = txtRect.right-txtRect.left;
	int cy = txtRect.bottom-txtRect.top;
	int x = ((rect.right-rect.left)-cx)/2; if (x<0) x=0;
	int y = ((rect.bottom-rect.top)-cy)/2; if (y<0) y=0;

	// draw text
	::SetRect(&rect, x, y, x+cx, y+cy);
	::DrawText (hdc, text, -1, &rect, DT_CENTER|DT_WORDBREAK);

	::SetBkColor(hdc, oldbgcol);
	::SetTextColor(hdc, oldtxtcol);
}



void VrClient::DrawInitialScreen()
{
	RLMutexLock l(m_bitmapdcMutex);
	
	// Put a "please wait" message up initially
	RECT rect;
	SetRect(&rect, 0,0, m_desktop_cx, m_desktop_cy);
	COLORREF bgcol = RGB(187, 200, 255); //RGB(0xcc, 0xcc, 0xcc);
	RLWnd::FillSolidRect(m_hBitmapDC, &rect, bgcol);
	
	{
		COLORREF oldbgcol  = ::SetBkColor(m_hBitmapDC, bgcol);
		COLORREF oldtxtcol = ::SetTextColor(m_hBitmapDC, RGB(0,0,32));
		//rect.right = m_desktop_cx / 2;
		//rect.bottom = m_desktop_cy / 2;
	
		::DrawText (m_hBitmapDC, "Please wait - initial screen loading", -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		::SetBkColor(m_hBitmapDC, oldbgcol);
		::SetTextColor(m_hBitmapDC, oldtxtcol);
	}

	::InvalidateRect(m_hwnd2, NULL, FALSE);
}

void VrClient::SendEncoderAndPointer(bool encoder, bool refresh, bool pointer)
{
	RLStream buffer(512);

	if (encoder)
	{
		_log2.Print(LL_INF, VTCLOG("Requesting new pixel format") );

		TrDesktopCapture capture;
		capture.CaptureInitPixelFormatOnly();

		// Set encoder
		aaSetEncoderMsg msg;
		msg.type			= aaSetEncoder;
		msg.pixelFrm 		= capture.m_frmL;
		msg.colorQuality	= m_encoder.colorQuality;
		msg.encoder			= m_encoder.encoder;
		msg.compressLevel	= m_encoder.compressLevel;
		msg.qualityLevel	= m_encoder.jpegQualityLevel;
		msg.copyRect		= true;

		buffer.AddRaw(&msg, sizeof(msg));

		if (refresh)
			buffer.AddUINT8(aaScreenUpdateRequest);
	}

	if (pointer)
	{
		// Request cursor updates if enabled by user

		buffer.AddUINT8(aaSetPointer);
		buffer.AddUINT8(m_opts.m_cursorRemoteRequest);
	}
	
	WriteExact(buffer, TRUE);
}


void VrClient::SendSessionInfoToServer()
{
	// just check to avoid sending info if no connection was established	
	if (m_transport->m_bytesRead==0 && m_transport->m_bytesSent==0) return;

	// send session information to web-server
	if (m_transport->m_direct && m_remoteId!=0)
	{	
		try {
			m_cmdSessionEnded.m_source   = 3;
			m_cmdSessionEnded.m_span   = (UINT64)(m_tmStart.GetElapsedSeconds()*1000);
			m_cmdSessionEnded.m_sent_t = m_transport->m_bytesRead;
			m_cmdSessionEnded.m_sent_v = m_transport->m_bytesSent;

			_CmdSessionEndedQueue.SendFromQueue();
			_CmdSessionEndedQueue.SendOneCmd(m_cmdSessionEnded, true);
		}
		catch(RLException& ex){
			_CmdSessionEndedQueue.AddToQueue(m_cmdSessionEnded);
			_log.WriteError(ex.GetDescription());
		}
	}
}

// Closing down the connection.
void VrClient::CloseTransport(bool sendTermMsg)
{
	omni_mutex_lock l(m_transport_close_lock);

	if (m_transport->IsOpened()) 
	{
		m_audioIn.StopSound();

		sendTermMsg = sendTermMsg && m_running && (!m_killing) && (m_rdp.m_pid==0);

		// set event to terminate working thread
		m_killing = true; //m_audioIn.SetStoppingStatus();

		if (sendTermMsg) {
			aaTerminateMsg msg;
			msg.type = aaError;
			msg.code = aaCloseSession;

			try {
				m_transport->SendExact(&msg, sizeof(msg), TRUE);
			}
			catch(RLException& ex) {
				_log.WriteError("CloseTransport()#2 %s", ex.GetDescription());
			}

			//_log.WriteInfo("sent aaTerminateMsg");
		}
		
		m_transport->Close(sendTermMsg);

		m_running = false;
	}
		
	_log2.Print(LL_INF, VTCLOG("CloseTransport()#3"));
}


void VrClient::OnTransportError(TransportException& ex, LPCSTR where)
{
	// if not killing && error not set yet
	if (!m_killing && !m_statusError)
	{
		//if (ex.IsNormalClose()) {
		//	SetStatus("Connection closed!", true);
		//	_log.WriteInfo ("%s in %s", (LPCSTR)ex.GetDescription(), where);
		//}
		//else {
			SetStatus(ex.GetDescription(), true);
			_log.WriteError("%s in %s", (LPCSTR)ex.GetDescription(), where);
		//}	
	}

	CloseTransport(false);
}


void VrClient::OnBtnClose()
{
	m_bCloseWindow = true;

	if (m_workThreadFinished) {
		_log2.Print(LL_INF, VTCLOG("WM_CLOSE WndProc1()#1"));
		ASSERT(m_hwnd1!=0);
		VERIFY(::DestroyWindow(m_hwnd1));
		m_hwnd1 = 0;
		m_hwnd2 = 0;
	}
	else {
		_log2.Print(LL_INF, VTCLOG("WM_CLOSE WndProc1()#2"));
		CloseTransport(true);
		SetStatus("Closing...");

		// close m_dlgSpeedTest
		if ((HWND)m_dlgSpeedTest!=NULL) 
			::PostMessage((HWND)m_dlgSpeedTest, WM_CLOSE, 0, 0);
	}
}

void VrClient::OnExitProcess()
{
	m_rdp.CloseProcess();
	::PostMessage(m_hwnd1, WM_CLOSE, 0, 0);
}

VrClient::~VrClient()
{
	// We are currently in the main thread.
	// The worker thread should be about to finish if it hasn't already. Wait for it.
	/*
	try {
		void *p;
		_this->join(&p);  // After joining, _this is no longer valid, same as "delete this"
	} 
	catch (omni_thread_invalid) { // The thread probably hasn't been started yet,
	}
	*/

	// TODO: m_hWorkerThread should be stoped here always...
	if (m_hWorkerThread) {
		if (::WaitForSingleObject(m_hWorkerThread, INFINITE) != WAIT_OBJECT_0)
			throw RLException("Error %d in WaitForSingleObject()", ::GetLastError());
		::CloseHandle(m_hWorkerThread);
	}

	//_log2.Print(LL_INF, VTCLOG("~VrClient()#0"));

	omni_mutex_lock l(m_objectsLock);

	{
		int count = m_objects.size();
		for (int i=0; i<count; i++) {
			if (this==m_objects[i]) {
				m_objects.erase (m_objects.begin()+i);
				break;
			}
		}
		if (i>=count)
			throw RLException("~VrClient() not found object");
	}

	m_rdp.CloseProcess();

	CRDP_KillTimer();

	{
		ASSERT(m_transport!=NULL);
		ASSERT(!m_transport->IsOpened());
		delete m_transport;
		m_transport = NULL;
	}

	this->FreeScreenBuffer();
	::DeleteDC(m_hBitmapDC);
}


// You can specify a dx & dy outside the limits; the return value will tell you whether it actually scrolled.
//
bool VrClient::ScrollScreen(int dx, int dy) 
{
	dx = max(dx, -m_hScrollPos);
	//dx = min(dx, m_hScrollMax-(m_clientCX-1)-m_hScrollPos);
	dx = min(dx, m_hScrollMax-m_clientCX-m_hScrollPos);
	dy = max(dy, -m_vScrollPos);
	//dy = min(dy, m_vScrollMax-(m_clientCY-1)-m_vScrollPos);
	dy = min(dy, m_vScrollMax-m_clientCY-m_vScrollPos);
	if (dx || dy) {
		m_hScrollPos += dx;
		m_vScrollPos += dy;
		RECT clirect;
		GetClientRect(m_hwnd2, &clirect);
		::ScrollWindowEx(m_hwnd2, -dx, -dy, NULL, &clirect, NULL, NULL,  SW_INVALIDATE);
		this->UpdateScrollbars();
		UpdateWindow(m_hwnd2);
		return true;
	}
	return false;
}

// Process windows messages
LRESULT CALLBACK VrClient::WndProcScroll(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{	// This is a static method, so we don't know which instantiation we're 
	// dealing with.  But we've stored a 'pseudo-this' in the window data.
	VrClient *_this = (VrClient *)::GetWindowLongW(hwnd, GWL_USERDATA);
		
	switch (iMsg) {
	case WM_HSCROLL:
		{				
			int dx = 0;
			int pos = HIWORD(wParam);
			switch (LOWORD(wParam)) {
				case SB_LINEUP:			dx = -2; break;
				case SB_LINEDOWN:		dx =  2; break;
				case SB_PAGEUP:			dx = _this->m_clientCX * -1/4; break;
				case SB_PAGEDOWN:		dx = _this->m_clientCX *  1/4; break;
				case SB_THUMBPOSITION:	dx = pos - _this->m_hScrollPos;
				case SB_THUMBTRACK:		dx = pos - _this->m_hScrollPos;
			}
			if (!_this->m_opts.m_FitWindow) _this->ScrollScreen(dx,0);
			return 0;
		}
	case WM_VSCROLL:
		{
			int dy = 0;
			int pos = HIWORD(wParam);
			switch (LOWORD(wParam)) {
				case SB_LINEUP:			dy = -2; break;
				case SB_LINEDOWN:		dy =  2; break;
				case SB_PAGEUP:			dy = _this->m_clientCY * -1/4; break;
				case SB_PAGEDOWN:		dy = _this->m_clientCY *  1/4; break;
				case SB_THUMBPOSITION:	dy = pos - _this->m_vScrollPos;
				case SB_THUMBTRACK:		dy = pos - _this->m_vScrollPos;
			}
			if (!_this->m_opts.m_FitWindow) _this->ScrollScreen(0,dy);
			return 0;
		}
	}
	return ::DefWindowProcW(hwnd, iMsg, wParam, lParam);
}


VrClient::CRDP::CRDP()
{
	m_pid = 0;
	m_handle = NULL;
	m_timer = 0;
}

VrClient::CRDP::~CRDP()
{
}

void VrClient::CRDP::CloseProcess()
{
	if (m_handle) {
		::TerminateProcess(m_handle, 0); // if didn't close self
		VERIFY(::CloseHandle(m_handle)!=0);
		m_handle = 0;
	}
}

void VrClient::CRDP_KillTimer()
{
	if (m_rdp.m_timer) {
		if(::KillTimer(m_hwnd1, 3)==0) {
			_log2.Print(LL_ERR, VTCLOG("CRDP::KillTimer() error=%d"), ::GetLastError());
		}
		m_rdp.m_timer = 0;
	}
}


BOOL VrClient::CRDP_EnumWindowsFn(HWND hwnd, LPARAM arg)
{	
	DWORD pid = 0;
	::GetWindowThreadProcessId(hwnd, &pid);

	VrClient* pClient = (VrClient*)arg;
	VrClient::CRDP* pRdp = &pClient->m_rdp;

	if (pRdp->m_pid==pid) {
		char buffer[256];
		if (::GetClassName(hwnd, buffer, sizeof(buffer))> 0) {
			if (strcmp(buffer, "TSSHELLWND")==0) {
				if (::GetWindowText(hwnd, buffer, sizeof(buffer))>0) {
					if (strstr(buffer, "127.0.0.1:")!=NULL) 
					{	
						CStringA str;
						str.Format("%u - %s", pClient->m_pInterop->GetRemoteId(), pClient->m_computerName);
						::SetWindowText(hwnd, (LPCSTR)str);

						pClient->CRDP_KillTimer();
					}
				}
			}
		}
	}

	return TRUE;
}


void VrClient::OnTimer(UINT timerID)
{
	if (timerID==3) {
		::EnumWindows(CRDP_EnumWindowsFn, (LPARAM)this);
	}
}

LRESULT CALLBACK VrClient::WndProc1_static(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) 
{
	// This is a static method, so we don't know which instantiation we're 
	// dealing with.  But we've stored a 'pseudo-this' in the window data.
	VrClient *_this = (VrClient *)::GetWindowLongW(hwnd, GWL_USERDATA);

	return _this->WndProc1(hwnd, iMsg, wParam, lParam);
}


void VrClient::OnCtlOrAlt(UINT uId, UINT32 key)
{
	if (!m_desktop_on) return;
	
	if (GetMenuState(m_hMenu, uId, MF_BYCOMMAND) == MF_CHECKED) {
		SendKeyEvent(key, false);
		::CheckMenuItem(m_hMenu, uId, MF_BYCOMMAND|MF_UNCHECKED);
		SetStateToolbarButton(uId, true, false);
	} else {
		::CheckMenuItem(m_hMenu, uId, MF_BYCOMMAND|MF_CHECKED);
		SetStateToolbarButton(uId, true, true);
		SendKeyEvent(key, true);
	}
}


inline LRESULT VrClient::WndProc1(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) 
{	
	try
	{
		
	switch (iMsg) 
	{
	//case WM_PAINT:         { 
		//this->OnPaint(1); return 0; }
	case WM_NOTIFY:
	{
		LPNMTTDISPINFOW TTStr = (LPNMTTDISPINFOW)lParam;
		if (TTStr->hdr.code != TTN_NEEDTEXTW) return 0;

		switch (TTStr->hdr.idFrom) {
			case IDC_OPTION1_BUTTON: TTStr->lpszText = L"Connection settings...";	break;
			case IDC_OPTION2_BUTTON: TTStr->lpszText = L"Connection encoder...";	break;
			case ID_FULLSCREEN:		TTStr->lpszText = L"Full screen";				break;
			case ID_REQUEST_REFRESH:TTStr->lpszText = L"Request screen refresh";	break;
			case ID_AUDIO_CHAT:		TTStr->lpszText = L"Voice chat";				break;
			case ID_CONN_CTLALTDEL:	TTStr->lpszText = L"Send Ctrl+Alt+Del";		break;
			case ID_CONN_CTLESC:	TTStr->lpszText = L"Send Ctrl+Esc";			break;
			case ID_CONN_CTLDOWN:	TTStr->lpszText = L"Send Ctrl key press/release";	break;
			case ID_CONN_ALTDOWN:	TTStr->lpszText = L"Send Alt key press/release";	break;
			case IDD_FILE_MANAGER:	TTStr->lpszText = L"File Manager";					break;
			case ID_DISCONNECT:		TTStr->lpszText = L"Disconnect";					break;
		}
		return 0;
	}
	case WM_SETFOCUS:	
		vrMain.m_pHotkeys->SetWindow(hwnd);
		SetFocus(m_hwnd2);
		return 0;
	case WM_TIMER:	this->OnTimer((UINT)wParam); return 0; 
	case WM_COMMAND:
	case WM_SYSCOMMAND:
		switch (LOWORD(wParam)) {
		case SC_MINIMIZE:	if (m_running) this->SetDormant(true);  break;
		case SC_RESTORE:	if (m_running) this->SetDormant(false); break;
		case ID_DISCONNECT:
			::SendMessage(hwnd, WM_CLOSE, 0, 0);
			return 0;
		case ID_TOOLBAR:
			{
				UINT uCheck = (GetMenuState(m_hMenu, ID_TOOLBAR,MF_BYCOMMAND) == MF_CHECKED) ?
						MF_UNCHECKED : MF_CHECKED;
				
				CheckMenuItem(m_hMenu, ID_TOOLBAR, MF_BYCOMMAND|uCheck);
				this->SizeWindow(false);
				return 0;
			}

		case IDC_DESKTOP_BUTTON: { OnBtnDesktop();     return 0; }
		case ID_AUDIO_CHAT:	     { OnBtnAudioChat();   return 0; }
		case IDC_OPTION1_BUTTON: { OnBtnOption1();     return 0; }
		case IDC_OPTION2_BUTTON: { OnBtnOption2();     return 0; }
		case IDD_FILE_MANAGER:   { OnBtnFileManager(); return 0; }
				
		case ID_FULLSCREEN:
			// Toggle full screen mode
			m_opts.m_FullScreen = !m_opts.m_FullScreen;
			DoFullScreenMode(false);
			return 0;

		case ID_REQUEST_REFRESH:
			if (m_desktop_on) {
				this->DrawInitialScreen();
				this->SendScreenUpdateRequest();	// Request a full-screen update
			}
			return 0;

		case ID_CONN_CTLESC:
			if (m_desktop_on) {
				this->SendKeyEvent(XK_Control_L, true);
				this->SendKeyEvent(XK_Escape,    true);
				this->SendKeyEvent(XK_Escape,    false);
				this->SendKeyEvent(XK_Control_L, false);
				this->WriteExact(0,0,TRUE);
			}
			return 0;
		
		case ID_CONN_CTLALTDEL:
			if (m_desktop_on) {
				this->SendKeyEvent(XK_Control_L, true);
				this->SendKeyEvent(XK_Alt_L,     true);
				this->SendKeyEvent(XK_Delete,    true);
				this->SendKeyEvent(XK_Delete,    false);
				this->SendKeyEvent(XK_Alt_L,     false);
				this->SendKeyEvent(XK_Control_L, false);
				this->WriteExact(0,0,TRUE);
			}
			return 0;
		
		case ID_CONN_CTLDOWN: { OnCtlOrAlt(ID_CONN_CTLDOWN, XK_Control_L); 	return 0; }		
		case ID_CONN_ALTDOWN: { OnCtlOrAlt(ID_CONN_ALTDOWN, XK_Alt_L);		return 0; }
		}
		break;		
	
	case WM_KILLFOCUS: { SwitchOffKey(); return 0; }
	case WM_SIZE:  { PositionChildWindow(); return 0; }	
	case WM_CLOSE: { OnBtnClose();          return 0; }
	case WM_DESTROY: 			
		{
			// Remove us from the clipboard viewer chain
			BOOL res = ChangeClipboardChain(m_hwnd2, m_hwndNextViewer);
				
			::PostQuitMessage(0);

			return 0;
		}
		
	case WM_RDP_RUN:
		{
			::ShowWindow(hwnd, SW_HIDE); //hide main window
			m_rdp.m_timer = ::SetTimer(hwnd, 3, 500, NULL);
			return 0;

		}
	}
	}
	catch(TransportException& ex) {
		OnTransportError(ex, "Vr::WndProc1");
		return 0;
	}

	return ::DefWindowProcW(hwnd, iMsg, wParam, lParam);
}

void VrClient::OnBtnDesktop()
{
	try {
		m_desktop_on = IsToolbarButtonPressed(IDC_DESKTOP_BUTTON);
		if (m_desktop_on) {
			m_dwUncommitedScreenUpdates = 0;

			// only first time we can do it, in any case we can get unhandled exeption
			if (m_hBitmap==NULL) 
				CreateScreenBuffer();
			DrawInitialScreen();
			SendEncoderAndPointer(true, false, true);
		}
		else {
			m_transport->SendByte(aaDesktopOFF, TRUE);
		}

		EnableFullControlOptions();

		SetCursorLocal((m_desktop_on) ? m_opts.m_cursorLocal : 2);

		if (!m_desktop_on) {
			::ShowScrollBar(m_hwndscroll, SB_HORZ, FALSE);
			::ShowScrollBar(m_hwndscroll, SB_VERT, FALSE);
		}

		PositionChildWindow();
	}
	catch(RLException& ex) {
		::MessageBox(m_hwnd1, ex.GetDescription(), "Ammyy Admin", MB_ICONWARNING);
	}
}

void VrClient::OnBtnAudioChat()
{
	try {
		m_audiochat_on = IsToolbarButtonPressed(ID_AUDIO_CHAT);
		if (m_audiochat_on) {
			// Send initial packet
			const int samplesPerSecond = 16000;

			m_audioOu.PreStart(samplesPerSecond);

			RLStream data;
			data.AddUINT8(aaSound);
			data.AddUINT16(aaSoundInit);
			data.AddUINT32(samplesPerSecond);
			WriteExact((char *)data.GetBuffer(), data.GetLen(), TRUE);

			m_audioIn.StartSound(samplesPerSecond);
		}
		else {
			m_audioIn.StopSound();

			RLStream data;
			data.AddUINT8(aaSound);
			data.AddUINT16(aaSoundClose);
			WriteExact((char *)data.GetBuffer(), data.GetLen(), TRUE);

			m_audioOu.StopSound();
		}

		if (!m_desktop_on) {
			::InvalidateRect(m_hwnd2, NULL, FALSE);
			::UpdateWindow(m_hwnd2);
		}
	}
	catch(RLException& ex) {
		::MessageBox(m_hwnd1, ex.GetDescription(), "Ammyy Admin", MB_ICONWARNING);
	}
}


void VrClient::OnBtnFileManager()
{
	if (((HWND)m_FmMainWindow)==NULL) {
		m_FmMainWindow.Create(m_pInterop->m_caption);
	}

	::ShowWindow(m_FmMainWindow, SW_SHOWNORMAL);
	::SetForegroundWindow(m_FmMainWindow);
}

void VrClient::SetCursorLocal(int cursorLocal)
{
	// NORMALCURSOR by default
	HINSTANCE hInstance1  = NULL;
	LPCTSTR lpCursorName = IDC_ARROW;

	switch (cursorLocal) {
		case 0:	hInstance1 = vrMain.m_hInstance; lpCursorName = MAKEINTRESOURCE(IDC_DOTCURSOR); break; // DOTCURSOR
		case 1:	hInstance1 = vrMain.m_hInstance; lpCursorName = MAKEINTRESOURCE(IDC_SMALLDOT);  break; // SMALLCURSOR
		case 3:	hInstance1 = vrMain.m_hInstance; lpCursorName = MAKEINTRESOURCE(IDC_NOCURSOR);  break; // NOCURSOR
	}

	HCURSOR cursor = ::LoadCursor(hInstance1, lpCursorName);

	::SetClassLongW(m_hwnd2, GCL_HCURSOR, (LONG)cursor);
}


void VrClient::OnBtnOption1()
{
	VrOptions dlg;
	dlg = m_opts;

	CStringA str1 = TrTranslator::GetPixelFormatAsString(m_frmR_org);
	CStringA str2 = TrTranslator::GetPixelFormatAsString(m_frmR);
	CStringA strVer = Settings::ConvertVersionINT2Str(m_appVersion);

	UINT b1 = m_transport->m_bytesRead/1024;
	UINT b2 = m_transport->m_bytesSent/1024;

	// fill connection info string
	dlg.m_connectionInfo.Format(
		"Remote ID: %u\r\n"
		"Ammyy: v%s  -  build %s\r\n"
		"Computer OS: %s\r\n"		
		"Computer name: %s\r\n"
		"Desktop size: %d x %d\r\n"
		"Desktop format: %u bits - %s\r\n"
		"Network format: %u bits - %s\r\n"
		"Transport: %s\r\n"
		"Traffic: %u Kb / %u Kb\r\n",

		m_pInterop->GetRemoteId(),
		(LPCSTR)strVer, (LPCSTR)m_appBuild,
		(LPCSTR)m_os_description,
		(LPCSTR)m_computerName,
		m_desktop_cx, m_desktop_cy,
		m_frmR_org.bitsPerPixel, (LPCSTR)str1,
		m_frmR.bitsPerPixel, (LPCSTR)str2,
		(LPCSTR)m_transport->GetDescription(),
		b1, b2
	);
				
	if (dlg.DoModal(m_hwnd1)!=IDOK) return;

	bool bFullScreenChanged    = (dlg.m_FullScreen!=m_opts.m_FullScreen);
	bool bScaleChanged         = (dlg.m_scale_num != m_opts.m_scale_num || dlg.m_scale_den != m_opts.m_scale_den);
	bool bCursorRequestChanged = (dlg.m_cursorRemoteRequest != m_opts.m_cursorRemoteRequest);
	
	m_opts = dlg;	

	if (bFullScreenChanged) this->DoFullScreenMode(false);

	if (m_opts.m_FitWindow) {
		this->PositionChildWindow();
	} else {
		// Resize the window if scaling factors were changed
		if (bScaleChanged)
		{
			this->SizeWindow(false);
			::InvalidateRect(m_hwnd2, NULL, FALSE);
		}
	}

	this->EnableFullControlOptions();

	if (m_desktop_on) {
		SetCursorLocal(m_opts.m_cursorLocal);
		if (bCursorRequestChanged)
			SendEncoderAndPointer(false, false, true);
	}
}

void VrClient::OnBtnOption2()
{
	DlgEncoder dlg;
	dlg.m_showDescr = false;
	dlg.m_item = m_encoder;

	if (dlg.DoModal(m_hwnd1)!=IDOK) return;

	m_encoder = dlg.m_item;

	if (m_running)
	{
		if (m_desktop_on) {
			DrawInitialScreen();
			SendEncoderAndPointer(true, true, false);
		}
		else {
			this->SetStateToolbarButton(IDC_DESKTOP_BUTTON, true, true);
			OnBtnDesktop();
		}
	}	
}



void VrClient::OnMouseMsg(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	if (!m_running || !m_desktop_on) return;

	_log2.Print(LL_INF, VTCLOG("message from mouse msg=%X wParam=%X, lParam=%X"), iMsg, wParam, lParam);

	HWND hwndFocused = ::GetFocus();
	if (hwndFocused != hwnd && hwndFocused != m_hwnd1) return;
	::SetFocus(hwnd);

	POINT coords;
	coords.x = LOWORD(lParam);
	coords.y = HIWORD(lParam);

	if (iMsg == WM_MOUSEWHEEL) {
		// Convert coordinates to position in our client area,
		// make sure the pointer is inside the client area.
		if ( WindowFromPoint(coords) != hwnd ||
			 !ScreenToClient(hwnd, &coords) ||
			 coords.x < 0 || coords.y < 0 ||
			 coords.x >= m_clientCX ||
			 coords.y >= m_clientCY ) {
			return;
		}
	} else {
		// Make sure the high-order word in wParam is zero.
		wParam = MAKEWPARAM(LOWORD(wParam), 0);
	}

	if (this->AutoScroll(coords.x, coords.y)) return;

	if (iMsg==WM_MOUSEMOVE) 
	{
		if (m_mouseMove.x==coords.x && m_mouseMove.y==coords.y) return;	// the same point as previous mouse move event
		m_mouseMove = coords;
	}
			
	this->SendPointerEvents(coords.x, coords.y, wParam);
}



LRESULT CALLBACK VrClient::WndProcDefW(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	return ::DefWindowProcW(hwnd, iMsg, wParam, lParam);
}

LRESULT CALLBACK VrClient::WndProc2_static(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) 
{	
	// This is a static method, so we don't know which instantiation we're 
	// dealing with.  But we've stored a 'pseudo-this' in the window data.
	VrClient *_this = (VrClient *)::GetWindowLongW(hwnd, GWL_USERDATA);

	return _this->WndProc2(hwnd, iMsg, wParam, lParam);
}


inline LRESULT VrClient::WndProc2(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) 
{	
	switch (iMsg) {
	case WM_REGIONUPDATED: { this->OnPaint(); return 0; }
	case WM_PAINT:         { this->OnPaint(); return 0; }
	case WM_TIMER:
		return 0; 
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
	case WM_MOUSEWHEEL:
		{
			this->OnMouseMsg(hwnd, iMsg, wParam, lParam);
			return 0;
		}

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		{		
			this->OnKeyEvent((int) wParam, (DWORD) lParam);		
			return 0;
		}
	case WM_CHAR:
	case WM_SYSCHAR:
	case WM_DEADCHAR:
	case WM_SYSDEADCHAR:
	  return 0;
	case WM_SETFOCUS:
		if (m_opts.m_FullScreen)
			SetWindowPos(hwnd, HWND_TOPMOST, 0,0,100,100, SWP_NOMOVE | SWP_NOSIZE);
		return 0;
	
	// Cacnel modifiers when we lose focus
	case WM_KILLFOCUS:
		{
			if (!m_running) break;
			if (m_opts.m_FullScreen) {
				// We must top being topmost, but we want to choose our
				// position carefully.
				HWND foreground = GetForegroundWindow();
				HWND hwndafter = NULL;
				if ((foreground == NULL) || 
					(GetWindowLong(foreground, GWL_EXSTYLE) & WS_EX_TOPMOST)) {
					hwndafter = HWND_NOTOPMOST;
				} else {
					hwndafter = GetNextWindow(foreground, GW_HWNDNEXT); 
				}

				SetWindowPos(m_hwnd1, hwndafter, 0,0,100,100, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
			}
			_log2.Print(LL_INF, VTCLOG("Losing focus - cancelling modifiers"));
			return 0;
		}	
		
	case WM_SETCURSOR:
		{
			// if we have the focus, let the cursor change as normal
			if (GetFocus() == hwnd) 
				break;

			// if not, set to default system cursor
			SetCursor( LoadCursor(NULL, IDC_ARROW));
			return 0;
		}

	case WM_DRAWCLIPBOARD:
		{
			this->OnLocalClipboardChange();			

			// Pass the message to the next window in clipboard viewer chain.  
			if (m_hwndNextViewer != NULL)
				return ::SendMessage(m_hwndNextViewer, WM_DRAWCLIPBOARD, 0,0);
			
			return 0;
		}

	case WM_CHANGECBCHAIN:
		{
			// The clipboard chain is changing
			HWND hWndRemove = (HWND) wParam;     // handle of window being removed 
			HWND hWndNext = (HWND) lParam;       // handle of next window in chain 
			// If next window is closing, update our pointer.
			if (hWndRemove == m_hwndNextViewer)  
				m_hwndNextViewer = hWndNext;  
			// Otherwise, pass the message to the next link.  
			else if (m_hwndNextViewer != NULL) 
				::SendMessage(m_hwndNextViewer, WM_CHANGECBCHAIN, (WPARAM)hWndRemove, (LPARAM)hWndNext);
			return 0;
		}
	
	}

	return ::DefWindowProcW(hwnd, iMsg, wParam, lParam);
}



// takes windows positions and flags and converts  them into VNC ones.
//
inline void VrClient::SendPointerEvents(int x, int y, DWORD keyflags)
{
	if (!m_opts.m_allowRemoteControl) return;

	int mask = ( ((keyflags & MK_LBUTTON) ? aaButton1Mask : 0) |
				 ((keyflags & MK_MBUTTON) ? aaButton2Mask : 0) |
			     ((keyflags & MK_RBUTTON) ? aaButton3Mask : 0) );

	short v = (short)HIWORD(keyflags);
	     if (v > 0) { mask |= aaButton4Mask; } 
	else if (v < 0) { mask |= aaButton5Mask; }

	//_log2.Print(LL_INF, VTCLOG("SendPointerEvents() x=%d y=%d keyflags=%X"), x, y, keyflags);
	
	try {				
		x = (x + m_hScrollPos) * m_opts.m_scale_den / m_opts.m_scale_num; // scaled
		y = (y + m_vScrollPos) * m_opts.m_scale_den / m_opts.m_scale_num; // scaled
	    if (x<0) x = 0;
		if (y<0) y = 0;

		aaPointerEventMsg events[2];

		for (int i=0; i<2; i++) {
			events[i].type = aaPointerEvent;
			events[i].buttonMask = mask;
			events[i].x = x;
			events[i].y = y;
		}

		int count = 1;

		if (HIWORD(keyflags) != 0) {
			// Immediately send a "button-up" after mouse wheel event.
			mask &= !(aaButton4Mask | aaButton5Mask);
			events[1].buttonMask = mask;
			count++;
		}

		_log2.Print(LL_INF, VTCLOG("SendPointerEvents() x=%d y=%d buttonMasks=%X %X, n=%d"), x, y, 
			events[0].buttonMask, events[1].buttonMask, count);
		
		m_counterAaPointerEvent += count;

		SoftCursorMove(x, y);

		WriteExact((char*)events, count*sizeof(aaPointerEventMsg), TRUE);
	} 
	catch (TransportException& ex) {
		OnTransportError(ex, "Vr::SendPointerEvents");
	}
}


//
// OnKeyEvent
//
// Normally a single Windows key event will map onto a single RFB
// key message, but this is not always the case.  Much of the stuff
// here is to handle AltGr (=Ctrl-Alt) on international keyboards.
// Example cases:
//
//    We want Ctrl-F to be sent as:
//      Ctrl-Down, F-Down, F-Up, Ctrl-Up.
//    because there is no keysym for ctrl-f, and because the ctrl
//    will already have been sent by the time we get the F.
//
//    On German keyboards, @ is produced using AltGr-Q, which is
//    Ctrl-Alt-Q.  But @ is a valid keysym in its own right, and when
//    a German user types this combination, he doesn't mean Ctrl-@.
//    So for this we will send, in total:
//
//      Ctrl-Down, Alt-Down,   
//                 (when we get the AltGr pressed)
//
//      Alt-Up, Ctrl-Up, @-Down, Ctrl-Down, Alt-Down 
//                 (when we discover that this is @ being pressed)
//
//      Alt-Up, Ctrl-Up, @-Up, Ctrl-Down, Alt-Down
//                 (when we discover that this is @ being released)
//
//      Alt-Up, Ctrl-Up
//                 (when the AltGr is released)

inline void VrClient::OnKeyEvent(int virtkey, DWORD keyData)
{
	if (!m_running || !m_desktop_on || !m_opts.m_allowRemoteControl) return;
			
	bool down = ((keyData & 0x80000000l) == 0);
	UINT uCheck   = (down) ? MF_BYCOMMAND|MF_CHECKED : MF_BYCOMMAND|MF_UNCHECKED;
	if (virtkey == 0x11)
	{
		::CheckMenuItem(m_hMenu, ID_CONN_CTLDOWN, uCheck);
		this->SetStateToolbarButton(ID_CONN_CTLDOWN, true, down);
	}
	if (virtkey == 0x12) {
		::CheckMenuItem(m_hMenu, ID_CONN_ALTDOWN, uCheck);
		this->SetStateToolbarButton(ID_CONN_ALTDOWN, true, down);
	}
				
    // if virtkey found in mapping table, send X equivalent
    // else
    //   try to convert directly to ascii
    //   if result is in range supported by X keysyms,
    //      raise any modifiers, send it, then restore mods
    //   else
    //      calculate what the ascii would be without mods
    //      send that

#ifdef _DEBUG
    char keyname[32];
    if (GetKeyNameText(  keyData,keyname, 31)) {
        _log2.Print(LL_INF, VTCLOG("Process key: %s (keyData %04x): "), keyname, keyData);
    };
#endif

	try {
		KeyActionSpec kas = m_keymap.PCtoX(virtkey, keyData);    
		
		if (kas.releaseModifiers & KEYMAP_LCONTROL) {
			SendKeyEvent(XK_Control_L, false );
			_log2.Print(LL_INF, VTCLOG("fake L Ctrl raised"));
		}
		if (kas.releaseModifiers & KEYMAP_LALT) {
			SendKeyEvent(XK_Alt_L, false );
			_log2.Print(LL_INF, VTCLOG("fake L Alt raised"));
		}
		if (kas.releaseModifiers & KEYMAP_RCONTROL) {
			SendKeyEvent(XK_Control_R, false );
			_log2.Print(LL_INF, VTCLOG("fake R Ctrl raised"));
		}
		if (kas.releaseModifiers & KEYMAP_RALT) {
			SendKeyEvent(XK_Alt_R, false );
			_log2.Print(LL_INF, VTCLOG("fake R Alt raised"));
		}
		
		for (int i = 0; kas.keycodes[i] != XK_VoidSymbol && i < MaxKeysPerKey; i++) {
			SendKeyEvent(kas.keycodes[i], down );
			_log2.Print(LL_INF, VTCLOG("Sent keysym %04x (%s)"), kas.keycodes[i], down ? "press" : "release");
		}
		
		if (kas.releaseModifiers & KEYMAP_RALT) {
			SendKeyEvent(XK_Alt_R, true );
			_log2.Print(LL_INF, VTCLOG("fake R Alt pressed"));
		}
		if (kas.releaseModifiers & KEYMAP_RCONTROL) {
			SendKeyEvent(XK_Control_R, true );
			_log2.Print(LL_INF, VTCLOG("fake R Ctrl pressed"));
		}
		if (kas.releaseModifiers & KEYMAP_LALT) {
			SendKeyEvent(XK_Alt_L, false );
			_log2.Print(LL_INF, VTCLOG("fake L Alt pressed"));
		}
		if (kas.releaseModifiers & KEYMAP_LCONTROL) {
			SendKeyEvent(XK_Control_L, false );
			_log2.Print(LL_INF, VTCLOG("fake L Ctrl pressed"));
		}

		WriteExact(0, 0, TRUE);

	} catch (TransportException& ex) {
		OnTransportError(ex, "Vr::OnKeyEvent");
	}
}



void VrClient::SendKeyEvent(UINT32 key, bool down)
{
	if (!m_opts.m_allowRemoteControl) return;

    aaKeyEventMsg ke;

    ke.type = aaKeyEvent;
    ke.down = down ? 1 : 0;
    ke.key = key;

	WriteExact((char *)&ke, sizeof(ke));
    //_log2.Print(LL_INF, VTCLOG("SendKeyEvent: key = x%04x status = %s"), key, down ? "down" : "up");
}



void VrClient::SendClientCutText(LPCSTR str, int len)
{
    aaCutTextMsg cct;
    cct.type = aaCutText;
    cct.length = len;

	RLStream buffer(sizeof(cct)+len);
	buffer.AddRaw(&cct, sizeof(cct));
	buffer.AddRaw(str, len);
	WriteExact(buffer, TRUE);

	_log2.Print(LL_INF, VTCLOG("Sent %d bytes of clipboard"), len);
}

// Copy any updated areas from the bitmap onto the screen.

inline void VrClient::OnPaint() 
{
	// No other threads can use bitmap DC
	RLMutexLock l(m_bitmapdcMutex);

	CStringA status;

	{
		RLMutexLock l(m_status_lock); // to protect m_status
		status = m_status;
		if (status.GetLength()==0) {
			if (!m_desktop_on) {
				status.Format("Desktop - off\nAudio chat - %s", m_audiochat_on ? "on" : "off");
			}
			else {
				if (m_desktop_unavailable) status="Remote desktop is unavailable";
			}
		}		
	}

	if (status.GetLength()>0)
	{
		// Do painting here
		PAINTSTRUCT psPaint;
		HDC hdc = ::BeginPaint(m_hwnd2, &psPaint);
		COLORREF c = (m_statusError) ? RGB(255,0,0) : RGB(0,0,64);
		this->DrawMessage(hdc, status, c);
		::EndPaint (m_hwnd2, &psPaint);
		return;
	}

	if (m_hBitmap == NULL) return;
	//if (!m_running) return;
				
	PAINTSTRUCT ps;
	HDC hdc = ::BeginPaint(m_hwnd2, &ps);

	/*
	int delay=100;
	if (delay) {
		// Display the area to be updated for debugging purposes
		COLORREF oldbgcol = SetBkColor(hdc, RGB(0,0,0));
		::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &ps.rcPaint, NULL, 0, NULL);
		SetBkColor(hdc,oldbgcol);
		::Sleep(delay);
	}
	*/
	
	if (m_opts.m_scaling) {
		int n = m_opts.m_scale_num;
		int d = m_opts.m_scale_den;
		
		// We're about to do some scaling on these values in the StretchBlt
		// We want to make sure that they divide nicely by n so we round them
		// down and up appropriately.
		ps.rcPaint.left =   ((ps.rcPaint.left   + m_hScrollPos) / n * n)         - m_hScrollPos;
		ps.rcPaint.right =  ((ps.rcPaint.right  + m_hScrollPos + n - 1) / n * n) - m_hScrollPos;
		ps.rcPaint.top =    ((ps.rcPaint.top    + m_vScrollPos) / n * n)         - m_vScrollPos;
		ps.rcPaint.bottom = ((ps.rcPaint.bottom + m_vScrollPos + n - 1) / n * n) - m_vScrollPos;
		
		// This is supposed to give better results.  I think my driver ignores it?
		SetStretchBltMode(hdc, HALFTONE);
		// The docs say that you should call SetBrushOrgEx after SetStretchBltMode, 
		// but not what the arguments should be.
		SetBrushOrgEx(hdc, 0,0, NULL);
		
		if (!StretchBlt(
			hdc, 
			ps.rcPaint.left, 
			ps.rcPaint.top, 
			ps.rcPaint.right -ps.rcPaint.left,
			ps.rcPaint.bottom-ps.rcPaint.top, 
			m_hBitmapDC, 
			(ps.rcPaint.left+m_hScrollPos)      * d / n,
			(ps.rcPaint.top +m_vScrollPos)      * d / n,
			(ps.rcPaint.right -ps.rcPaint.left) * d / n,
			(ps.rcPaint.bottom-ps.rcPaint.top)  * d / n,
			SRCCOPY)) 
		{
			throw RLException("Error in OnPaint()#1!");
		}
	} else {
		RECT& r = ps.rcPaint;
		if (!::BitBlt(hdc, r.left, r.top, r.right-r.left, r.bottom-r.top, 
			m_hBitmapDC, r.left+m_hScrollPos, r.top+m_vScrollPos, SRCCOPY)) 
		{
			throw RLException("Error in OnPaint()#2!");
		}
	}
	
	::EndPaint(m_hwnd2, &ps);
}

inline void VrClient::UpdateScrollbars() 
{
	// We don't update the actual scrollbar info in full-screen mode
	if (m_opts.m_FullScreen) return;

	SCROLLINFO scri;
	scri.cbSize = sizeof(scri);
	scri.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
	scri.nMin = 0;
	scri.nMax = m_hScrollMax; 
	scri.nPage= m_clientCX;
	scri.nPos = m_hScrollPos; 
	::SetScrollInfo(m_hwndscroll, SB_HORZ, &scri, TRUE);
	
	scri.cbSize = sizeof(scri);
	scri.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
	scri.nMin = 0;
	scri.nMax = m_vScrollMax;
	scri.nPage= m_clientCY;
	scri.nPos = m_vScrollPos; 	
	::SetScrollInfo(m_hwndscroll, SB_VERT, &scri, TRUE);
}

// ********************************************************************
//  Methods after this point are generally called by the worker thread.
//  They finish the initialisation, then chiefly read data from the server.
// ********************************************************************

UINT VrClient::run_undetached_static(void* param)
{
	ASSERT(param != NULL);
	((VrClient*) param)->run_undetached();
	_endthreadex(0);
	return 0;
}


void VrClient::run_undetached() 
{
	_log2.Print(LL_INF, VTCLOG("Update-processing thread started"));

	try 
	{
		if (this->m_remoteId!=0) {
			SetStatus("Connecting to router");
			this->ConnectToRouter();
			m_cmdSessionEnded.m_id_t = m_remoteId;
			m_cmdSessionEnded.m_id_v = TheApp.m_ID;
		}
		else {
			CStringA msg;
			msg.Format("Connecting to host %s", m_host);
			SetStatus(msg);
			TCP tcp;
			tcp.Create();
			tcp.Connect(m_host, DEFAULT_INCOME_PORT, true);
			TCP::SetSocketOptions(tcp);

			this->SendInitMsg(tcp);

			m_transport->SetTCPSocket(tcp.Detach(), true);
		}

		m_tmStart.Start();

		if (m_killing) goto exit;		
		
		m_audioIn.m_transport = m_transport;
		m_audioOu.m_transport = m_transport;
		m_FmMainWindow.m_R.m_transport = m_transport;

		{
			UINT32 myId, remoteId;
			InteropCommon::ReadInitMsg((m_remoteId==0), myId, remoteId, m_appVersion);

			// check client version
			if (m_appVersion != Settings::GetVersionINT()) 
			{
				LPCSTR string = rlLanguages.GetValueA(D_WARN_DIFF_VERSIONS);

				if (string!=NULL) {
					CStringA msg;
					msg.Format((LPCSTR)string, (LPCSTR)Settings::ConvertVersionINT2Str(m_appVersion));
					::MessageBoxA(m_hwnd1, msg, "Ammyy Admin", MB_ICONWARNING);
				}
			}

			_log2.Print(LL_INF, VTCLOG("Connected to target"));
		}

		{
			LPCSTR status = rlLanguages.GetValueA(D_WAITING_AUTHORIZATION);
			SetStatus(status);
		}

		if (m_killing) goto exit;

		DoCrypt();

		if (m_killing) goto exit;

		DoAuthorization();

		if (m_killing) goto exit;

		ReadOtherInfo();
		CheckPermissions();

		if (m_killing) goto exit;

		DoDirectConnect();

		if (m_killing) goto exit;

		if (m_pInterop->m_profile==InteropViewer::VrSpeedTestOnly)
		{
			::ShowWindow(m_hwnd1, SW_HIDE); //hide main window
			this->SendByte(aaSpeedTest, TRUE);

			m_dlgSpeedTest.m_transport = m_transport;
			m_dlgSpeedTest.DoModal();

			this->SendByte(aaStExit, TRUE);

			m_running = true; // to send close command
			m_bCloseWindow = true; // to close window

			goto exit;
		}
		
		if (m_pInterop->m_profile==InteropViewer::VrRDP) // if "Microsoft RDP" selected
		{
			SetStatus("Run RDP client");

			UINT8 msg = aaRDP;
			m_transport->SendExact(&msg, sizeof(msg), TRUE);

			UINT16 result;
			ReadExact(&result, sizeof(result));

			if (result==aaErrorNone) {	
				_log2.Print(LL_INF, VTCLOG("RunRDPclient()#0"));
				m_pInterop->RunRDPclient(m_rdp.m_handle, m_rdp.m_pid, m_hwnd1, WM_RDP_RUN);
				_log2.Print(LL_INF, VTCLOG("RunRDPclient()#1"));
				m_bCloseWindow = true; // fine, so close window
			}
			else {
				ThrowError(result);
			}
			goto exit;
		}

		if (m_pInterop->m_profile==InteropViewer::VrFileManagerOnly) // if "File Manager" selected
		{	
			m_FmMainWindow.m_wndMain = m_hwnd1;
			::SendMessage(m_hwnd1, WM_SYSCOMMAND, IDD_FILE_MANAGER, 0);
			//OnBtnFileManager(); // doesn't work from this thread
			::ShowWindow(m_hwnd1, SW_HIDE); //hide main window
			m_running = true;
		}
		else 
		{
			DoFullScreenMode(false);

			SetStatus("");
			m_running = true;

			EnableFullControlOptions();
			SetCursorLocal(2);

			if (m_profile == InteropViewer::VrDesktop) {
				this->SetStateToolbarButton(IDC_DESKTOP_BUTTON, true, true);
				OnBtnDesktop();
			}
			else if (m_profile == InteropViewer::VrAudioChat) {
				this->SetStateToolbarButton(ID_AUDIO_CHAT, true, true);
				OnBtnAudioChat();				
			}

			::UpdateWindow(m_hwnd1);
		}

		while (!m_killing)
		{					
			UINT8 msgType;
			ReadExact(&msgType, 1);
				
			switch (msgType) {
				case aaError:			ThrowError();		break;
				case aaCutText:			OnAaCutText();		break;
				case aaScreenUpdate:	OnAaScreenUpdate();	break;
				case aaPointerMove:		OnAaPointerMove();	break;
				case aaSound:			OnAaSound();		break;
				case aaSetColourMapEntries:
					throw RLException("Unhandled SetColormap message type received!");
					break;
								
				case aaNop:					continue; // do nothing
				case aaDesktopUnavailable:	OnAaDesktopUnavailable(); break;

				case aaPingRequest:
					{
						m_transport->SendByte(aaPingReply, TRUE);
						break;
					}

				default: 
					{
						if (m_FmMainWindow.m_R.OnAaMsg(msgType)) break;
						_log2.Print(LL_INF, VTCLOG("Unknown message type x%02x"), (int)msgType);
						throw RLException("Unhandled message type=%d received!", (int)msgType);
					}
			}

		}            
	} 
	catch (TransportException& ex) {
		OnTransportError(ex, "Vr::run_un");
	}
	catch(RLException& ex) {
		SetStatus(ex.GetDescription(), true);
	}

	//before terminating this thread
exit:
	_log2.Print(4, VTCLOG("Update-processing thread finishing %d"), (int)m_killing);

	CloseTransport(false);
	SendSessionInfoToServer();

	m_workThreadFinished = true;

	// only close window if: was action WM_CLOSE, called OnExitProcess() or rdp session
	if (m_bCloseWindow) {
		_log2.Print(4, VTCLOG("Sending WM_CLOSE"));
		::PostMessage(m_hwnd1, WM_CLOSE, 0, 0);
	}
	else {
		if ((HWND)m_FmMainWindow!=NULL) 
			::ShowWindow(m_FmMainWindow, SW_HIDE);

		EnableFullControlOptions();
		::ShowWindow(m_hwnd1, SW_SHOW); //show main window
		::SetForegroundWindow(m_hwnd1);
	}
}


//
// Requesting screen updates from the server
//

inline void VrClient::SendScreenUpdateRequest()
{
	UINT8 msg = aaScreenUpdateRequest;
    WriteExact((char *)&msg, sizeof(msg), TRUE);
}

inline void VrClient::SendScreenUpdateCommit(DWORD count)
{
	RLStream buffer;

	aaScreenUpdateCommitMsg msg;
	msg.type = aaScreenUpdateCommit;

	while(count>0) {
		msg.countCommits = min(count, 255);
		buffer.AddRaw(&msg, sizeof(msg));
		count -= msg.countCommits;
	}

	WriteExact(buffer, TRUE);
}


void VrClient::OnAaSound()
{
	UINT16 count;

	this->ReadExact(&count, sizeof(count));

	ASSERT(count!=aaSoundInit);
	ASSERT(count!=aaSoundClose);

	m_audioOu.OnData(count);
}


// A ScreenUpdate message has been received

void VrClient::OnAaScreenUpdate() 
{
	if (m_desktop_unavailable) {
		m_desktop_unavailable = false; // reset if remote desktop was unavailable was
		::InvalidateRect(m_hwnd2, NULL, FALSE);
		::UpdateWindow(m_hwnd2);
	}
	
	while (true)
	{
		UINT8 type;
		
		ReadExact((char *) &type, sizeof(type));
		
		if (type == aaEncoderLastRect) break;

		if (type == aaEncoderChanged) {
			OnAaEncoderChanged();
			continue;
		}

		if (type == aaEncoderCopyRect) {
			ReadCopyRect();
			SoftCursorUnlockScreen();
			continue;
		}

		aaFramebufferUpdateRectHeader surh;
		surh.encoder = type;
		ReadExact(((char *)&surh)+1, sizeof(surh)-1);

		if ( surh.encoder == aaEncoderCursorMono ||
			 surh.encoder == aaEncoderCursorRich ) {
			ReadCursorShape(&surh);
			continue;
		}

		// If *Cursor encoding is used, we should prevent collisions
		// between framebuffer updates and cursor drawing operations.
		SoftCursorLockArea(surh.r.x, surh.r.y, surh.r.w, surh.r.h);

		switch (surh.encoder) {
			case aaEncoderRaw:			ReadRawRect(&surh);		break;
			//case aaEncoderRRE:		ReadRRERect(&surh);		break;
			//case aaEncoderCoRRE:		ReadCoRRERect(&surh);	break;
			case aaEncoderAAFC:			ReadHextileRect(&surh);	break;
			case aaEncoderAAC:			ReadRectAAC(&surh);		break;
			//case aaEncoderZlib:		ReadZlibRect(&surh);	break;
			//case aaEncoderTight:		ReadTightRect(&surh);	break;
			//case aaEncoderZlibHex:	ReadZlibHexRect(&surh);	break;
			default:
				throw RLException("Unknown encoder=%u", (int)surh.encoder);
		}

		// Tell the system to update a screen rectangle. Note that
		// InvalidateScreenRect member function knows about scaling.
		if (surh.encoder!=aaEncoderAAFC) // aaEncoderAAFC do it by self
			InvalidateScreenRect(&surh.r);

		// Now we may discard "soft cursor locks".
		SoftCursorUnlockScreen();
	}	

	// Inform the other thread that an update is needed.
	::PostMessage(m_hwnd2, WM_REGIONUPDATED, NULL, NULL);

	if (m_desktop_on) { // no need to send if desktop is off here
		if (m_dormant)		
			::InterlockedIncrement(&m_dwUncommitedScreenUpdates);
		else
			SendScreenUpdateCommit(1);
	}
}


void VrClient::OnAaDesktopUnavailable()
{
	m_desktop_unavailable = true;

	::InvalidateRect(m_hwnd2, NULL, FALSE);
	::UpdateWindow(m_hwnd2);
}



void VrClient::SetDormant(bool newstate)
{
	_log2.Print(5, VTCLOG("%s dormant mode"), newstate ? "Entering" : "Leaving");
	m_dormant = newstate;
	if (!m_dormant && m_desktop_on) {
		DWORD count = m_dwUncommitedScreenUpdates;

		if (count>0) {
			SendScreenUpdateCommit(count);
			for (LONG i=0; i<count; i++) ::InterlockedDecrement(&m_dwUncommitedScreenUpdates);
		}
	}
}


// The server has copied some text to the clipboard - put it in the local clipboard too.
//
void VrClient::OnAaCutText() 
{
	aaCutTextMsg msg;
	_log2.Print(6, VTCLOG("Read remote clipboard change"));
	ReadExact(((char*)&msg)+1, sizeof(msg)-1 );
	int len = msg.length;
	
	char* buffer1 = (char*)m_netBuffer.GetBuffer1(len+1);
	ReadString(buffer1, len);
	UpdateLocalClipboard(buffer1, len);
}



// General utilities -------------------------------------------------


// Read the number of bytes and return them zero terminated in the buffer 
inline void VrClient::ReadString(char *buf, int length)
{
	if (length > 0)
		ReadExact(buf, length);
	buf[length] = '\0';
    _log2.Print(10, VTCLOG("Read a %d-byte string"), length);
}


// Sends the number of bytes specified from the buffer
inline void VrClient::WriteExact(char *buf, int bytes, BOOL block)
{
	//if (m_transport == NULL) return;
	if (bytes == 0 && !block) return;

	_log2.Print(10, VTCLOG("  writing %d bytes, block=%d"), bytes, (int)block);

	try {
		m_transport->SendExact(buf, bytes, block);
	}
	catch(TransportException& ex) {
		m_running = false;
		_log2.Print(LL_INF, VTCLOG("%s"), ex.GetDescription());
		throw;
	}
}

void VrClient::WriteExact(RLStream& buffer, BOOL block)
{
	WriteExact((char*)buffer.GetBuffer(), buffer.GetLen(), block);
}

//
// Invalidate a screen rectangle respecting scaling set by user.
//

void VrClient::InvalidateScreenRect(int x, int y, int cx, int cy)
{
	RECT rect;
	::SetRect(&rect, x, y, x+cx, y+cy);
	this->InvalidateScreenRect(&rect);
}

void VrClient::InvalidateScreenRect(const aaRectangle *r)
{
	RECT rect;
	::SetRect(&rect, r->x, r->y, r->x + r->w, r->y + r->h);
	this->InvalidateScreenRect(&rect);
}

void VrClient::InvalidateScreenRect(const RECT *pRect) 
{
	RECT rect;

	// If we're scaling, we transform the coordinates of the rectangle
	// received into the corresponding window coords, and invalidate
	// *that* region.

	if (m_opts.m_scaling) {
		// First, we adjust coords to avoid rounding down when scaling.
		int n = m_opts.m_scale_num;
		int d = m_opts.m_scale_den;
		int left   = (pRect->left / d) * d;
		int top    = (pRect->top  / d) * d;
		int right  = (pRect->right  + d - 1) / d * d; // round up
		int bottom = (pRect->bottom + d - 1) / d * d; // round up

		// Then we scale the rectangle, which should now give whole numbers.
		rect.left   = (left   * n / d) - m_hScrollPos;
		rect.top    = (top    * n / d) - m_vScrollPos;
		rect.right  = (right  * n / d) - m_hScrollPos;
		rect.bottom = (bottom * n / d) - m_vScrollPos;
	} else {
		rect.left   = pRect->left   - m_hScrollPos;
		rect.top    = pRect->top    - m_vScrollPos;
		rect.right  = pRect->right  - m_hScrollPos;
		rect.bottom = pRect->bottom - m_vScrollPos;
	}

	::InvalidateRect(m_hwnd2, &rect, FALSE);
}

//
// Create new framebuffer with the new size and change the window size correspondingly.
//

void VrClient::OnAaEncoderChanged()
{
	UINT8 flags;
	ReadExact(&flags, sizeof(flags));

	this->ResetEncoders();

	if (flags & aaEncoderChangedEncoder) // encoder or remote format was changed
	{
		char buffer[2+2*sizeof(aaPixelFormat)];

		ReadExact(buffer, sizeof(buffer));

		this->ResetTranslators();

		UINT8 newEncoder  = (UINT8)buffer[0];
		m_jpeg.quality    = (UINT8)buffer[1];
		m_frmR     = *(aaPixelFormat*)&buffer[2];
		m_frmR_org = *(aaPixelFormat*)&buffer[2+sizeof(aaPixelFormat)];

		m_bytesppR = m_frmR.bitsPerPixel / 8;

		m_translator     = TrTranslator::Init(m_frmR, m_frmL);

		m_ddbColors[0] = 0xFFFFFF; // white
		m_ddbColors[1] = 0x000000; // black
	}
	
	if (flags & aaEncoderChangedSize) // desktop size was changed
	{
		char buffer[4];
	
		ReadExact(buffer, sizeof(buffer));

		m_desktop_cx = *((UINT16*)&buffer[0]);
		m_desktop_cy = *((UINT16*)&buffer[2]);

		CreateScreenBuffer();
		DrawInitialScreen();

		SizeWindow(!m_size_is_set);
		DoFullScreenMode(true);

		m_size_is_set = true;

		_log2.Print(LL_INF, VTCLOG("Geometry %d x %d"), m_desktop_cx);
	}
}


void VrClient::SetStatus(LPCSTR text, bool error)
{
	{
		if (error && text[0] == 0) text = "ERROR";

		RLMutexLock l(m_status_lock); // to protect m_status
		m_status = text;
		m_statusError = error;
	}

	::InvalidateRect(m_hwnd2, NULL, FALSE);
	::UpdateWindow(m_hwnd2);
}

