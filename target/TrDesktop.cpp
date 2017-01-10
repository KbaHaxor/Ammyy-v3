#include "stdafx.h"
#include "../common/omnithread/omnithread.h"
#include "TrMain.h"
#include "TrRegion.h"
#include "TrDesktop.h"
#include "TrService.h"
#include "TrKeymap.h"
#include "TrEncoder.h"
//#include "TrEncoderRRE.h"
//#include "TrEncoderCoRRE.h"
#include "TrEncoderHexT.h"
//#include "TrEncoderTight.h"
//#include "TrEncoderZlib.h"
//#include "TrEncoderZlibHex.h"
#include "TrEncoderAAC.h"
#include "../main/Common.h"
#include "../main/AmmyyApp.h"


#if (_MSC_VER>= 1300)
#include <fstream>
#else
#include <fstream.h>
#endif


DWORD TrDesktop::LoopMessagesAndWaiting(DWORD nCount, HANDLE *pHandles, DWORD dwMilliseconds)
{
	MSG msg;
	while (::PeekMessage(&msg, m_hwnd, NULL, NULL, PM_REMOVE)!=0)
	{
		::DispatchMessage(&msg);
	}

	// TODO: need to test with QS_ALLINPUT, this avoid unblock WM_DRAWCLIPBOARD when operator is minimazed
	return ::MsgWaitForMultipleObjects(nCount, pHandles, FALSE, dwMilliseconds, QS_ALLEVENTS);
}


void* TrDesktop::run_undetached(void *arg)
{
	// START PROCESSING DESKTOP MESSAGES
	// We set a flag inside the desktop handler here, to indicate it's now safe to handle clipboard messages
	m_clipboard_active = TRUE;

	//maximp: before it's called from Startup() and cause bug, bacause it changes desktop inside CheckUpdate()
	// and CaptureFullScreen() could't do job
	this->KillScreenSaver();

	HANDLE hEvents[3];
	hEvents[0] = m_eventQuit;
	hEvents[1] = m_eventRemoteKey;
	hEvents[2] = m_eventUpdate;


	while (true)
	{
		{
			omni_mutex_lock l(m_client->m_regionLock); // to protect m_eventUpdate

			if (m_dwUncommitedScreenUpdates==0)
				m_eventUpdate.Set();	// ready for desktop update
			else
				m_eventUpdate.Reset();	// not ready
		}
			
		DWORD dwEvent = LoopMessagesAndWaiting(3, hEvents, INFINITE);

		if (dwEvent==WAIT_OBJECT_0 + 3) continue; // message
		if (dwEvent==WAIT_OBJECT_0) break;	// quit event

	update:
		//do update
		m_eventRemoteKey.Reset();

		if (TrMain::IsOutConsole()) {
			_log2.Print(LL_INF, VTCLOG("This session is out of console"));
			this->Shutdown();
			_TrMain.KillClients(aaErrorSessionInactive);
			return FALSE;
		}

		try {
			this->CheckUpdates();
		}
		catch(TransportException& ex) {
			m_client->OnTransportError(ex, "CheckUpdates()");
			break;
		}
		catch(RLException& ex) {
			_log2.Print(LL_ERR, VTCLOG("CheckUpdates() %s"), ex.GetDescription());
			m_client->KillClient(aaErrorInternalError);
			break;
		}

		//wait 50ms or urgent update event
		dwEvent = LoopMessagesAndWaiting(2, hEvents, 50);

		if (dwEvent==WAIT_OBJECT_0+1) goto update;
		if (dwEvent==WAIT_OBJECT_0  ) break;	// quit event
	}

	m_clipboard_active = FALSE;
	
	_log2.Print(LL_INF, VTCLOG("quitting desktop server thread"));

	// Clear all the hooks and close windows, etc.
	try {
		this->Shutdown();
	}
	catch(RLException& ex) {
		_log.WriteError("TrDesktopThread() %s", ex.GetDescription());
	}

	// Clear the shift modifier keys, now that there are no remote clients
	TrKeymap::ClearShiftKeys();

	return NULL;
}


TrDesktop::TrDesktop(TrClient* client)
: m_pointer_event_timer(false)
{
	m_dwUncommitedScreenUpdates = 0;
	m_client = client;
	m_hwnd = NULL;
	m_hnextviewer = NULL;
	m_initialClipboardSeen = false;
	m_check_update_called = false;
	m_palettechanged = false;

	m_chngbuff = NULL;

	m_clipboard_active = FALSE;

	m_mouseDividerX = m_mouseDividerY = 0;

	m_aaSetEncoderMsg.type = 0; // mark as clear

	m_full_refresh = false;

	m_encoder = NULL;
	m_hcursor = NULL;
	memset(&m_bmrect, 0, sizeof(m_bmrect));

	// mark it as invalid
	m_cursor_pos.x = m_cursor_pos.y = -1;
	m_desktop_last_size.cx = m_desktop_last_size.cy = -1;
	m_desktop_last_frml.bitsPerPixel = -1;

	try {
		m_eventQuit.Create();
		m_eventRemoteKey.Create();
		m_eventUpdate.Create();
	}
	catch(RLException& ex) {
		_log2.Print(LL_ERR, VTCLOG("trgDesktop() '%s'."), ex.GetDescription());
	}
}

TrDesktop::~TrDesktop()
{
	_log2.Print(LL_INF, VTCLOG("killing desktop server"));

	// If we created a thread then here we stop it
	if (this->state() != omni_thread::state_t::STATE_NEW)
	{
		_log2.Print(LL_INF, VTCLOG("setting quit event"));

		m_eventQuit.Set();

		_log2.Print(LL_INF, VTCLOG("waiting finish working thread"));

		// Wait desktop thread if it was started
		this->wait_thread();

		_log2.Print(LL_INF, VTCLOG("finished working thread"));
	}

	// Let's call Shutdown just in case something went wrong...
	Shutdown();

	m_eventRemoteKey.Close();
	m_eventUpdate.Close();
	m_eventQuit.Close();

	FreeEncoder();
}


// temp function, need to get info about metrics of desktop, make only part Startup
void TrDesktop::FillGeometry()
{
	CreateBuffers();
	FillMouseDividers();

	Shutdown();
}


void TrDesktop::FillMouseDividers()
{
	// take only primary display rect, it's need to set mouse dividers
	RECT coord;
	::GetWindowRect(::GetDesktopWindow(), &coord);

	m_mouseDividerX = coord.right  - coord.left - 1;
	m_mouseDividerY = coord.bottom - coord.top  - 1;

	if (m_mouseDividerX<=0 || m_mouseDividerY<=0) {
		_log.WriteError("Mouse dividers %d %d", m_mouseDividerX, m_mouseDividerY);
		m_mouseDividerX = m_mouseDividerY = 0;
	}
}

// Routine to startup and install all the hooks and stuff
void TrDesktop::Startup()
{	
	ASSERT(m_desktopNameForOptimizer.GetLength()==0);
	m_desktopNameForOptimizer = this->TrDesktopSelector::m_name;
	if (m_desktopNameForOptimizer.GetLength()!=0) {
		_TrMain.m_desktopOptimizer.OnDesktopOpen(m_desktopNameForOptimizer);
	}

		 CreateBuffers();
	if (!m_capture.SetPalette())		throw RLException("ERROR in  SetPalette()");
		 InitWindow();

	m_eventUpdate.Reset();
	m_eventRemoteKey.Reset();

	// this member must be initialized: we cant assume the absence of clients when desktop is created.
	m_cursorpos.left = 0;
	m_cursorpos.top = 0;
	m_cursorpos.right = 0;
	m_cursorpos.bottom = 0;

	FillMouseDividers();
}

// Routine to shutdown all the hooks and stuff
void TrDesktop::Shutdown()
{
	// If we created a window then kill it and the hooks
	if (m_hwnd != NULL)
	{	
		// The window is being closed - remove it from the viewer list
		::ChangeClipboardChain(m_hwnd, m_hnextviewer);

		// Close the hook window
		::DestroyWindow(m_hwnd);
		m_hwnd = NULL;
		m_hnextviewer = NULL;
	}

	// Free changes buffer
	if (m_chngbuff != NULL) {
		delete [] m_chngbuff;
		m_chngbuff = NULL;
	}

	//it was in desctuctor
	if (m_desktopNameForOptimizer.GetLength()!=0) {
		_TrMain.m_desktopOptimizer.OnDesktopClose(m_desktopNameForOptimizer);
		m_desktopNameForOptimizer = "";
	}

	m_capture.CaptureFree();
}


// Routine used to close the screen saver, if it's active...
/*
BOOL CALLBACK TrDesktop::KillScreenSaverFunc(HWND hwnd, LPARAM lParam)
{
	CStringA wndClass;

	// - ONLY try to close Screen-saver windows!!!
	if (::GetClassName(hwnd, wndClass.GetBuffer(512), 512) != 0) 
	{
		wndClass.ReleaseBuffer();

		// WinXP "3D Text", "3D Pipe" -> "D3DSaverWndClass"  // WM_CLOSE doesn't help here
		// WinXP "Windows XP"         -> "WindowsScreenSaverClass"

		if (wndClass == "WindowsScreenSaverClass") {
			::PostMessage(hwnd, WM_CLOSE, 0, 0);
		}
	}

	return TRUE;
}
*/

void TrDesktop::KillScreenSaver()
{
	_log2.Print(LL_INF, VTCLOG("KillScreenSaver..."));

	LPCSTR DESKTOP_NAME = "Screen-saver";
	
	/* tmp_code

	// Find the screensaver desktop
	HDESK hDesk = ::OpenDesktop(DESKTOP_NAME, 0, FALSE, DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS);
	if (hDesk != NULL)
	{
		_log2.Print(LL_INF, VTCLOG("Killing ScreenSaver"));

		//tmp_code
		_log.WriteError("tmp_code#2 KillScreenSaver() '%s' %s", TrDesktopSelector::GetInputDesktopName(), TrDesktopSelector::GetThreadDesktopName());
		HDESK desk_old = ::GetThreadDesktop(::GetCurrentThreadId());

		TrDesktopSelector tt;
		tt.SetInputDesktop();
		

		_log.WriteError("tmp_code#3 KillScreenSaver() '%s' %s", TrDesktopSelector::GetInputDesktopName(), TrDesktopSelector::GetThreadDesktopName());

		//for (int i=0; i<100; i++) 
		//::mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, i*100, i*50, 0, 0); // Do the pointer event

		::mouse_event(32769, 17211, 23869, 0, 0);
		::mouse_event(32769, 17211, 24051, 0, 0);
		::mouse_event(32769, 17211, 24233, 0, 0);
		::mouse_event(32769, 17348, 24416, 0, 0);
		::mouse_event(32769, 17416, 24476, 0, 0);
		::mouse_event(32769, 7485, 24598, 0, 0);

		// Close all windows on the screen saver desktop
		::EnumDesktopWindows(hDesk, (WNDENUMPROC) &KillScreenSaverFunc, 0);

		//tmp_code
		::SetThreadDesktop(desk_old);

		::CloseDesktop(hDesk);
		
		// Pause long enough for the screen-saver to close
		//::Sleep(200);
		for (int i=0; i<100; i++) {
			CStringA desktopName = TrDesktopSelector::GetInputDesktopName();
			//_log.WriteError("-- '%s'", desktopName);
			if (desktopName!=DESKTOP_NAME) break; // other desktop already
			::Sleep(10);
		}

		// maximp: SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, FALSE) - doesn't close desktop...
		
		// Reset the screen saver so it can run again, maximp: I don't know why we need to call...
		//::SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, TRUE, 0, SPIF_SENDWININICHANGE); 
	}

	*/
}


void TrDesktop::InitWindow()
{
	LPCSTR szClassName = "Ammyy_fake_wnd";
	static ATOM m_wndClass = 0;

	if (m_wndClass == 0) {
		// Create and Register the window class
		WNDCLASSEX wndclass;

		wndclass.cbSize			= sizeof(wndclass);
		wndclass.style			= 0;
		wndclass.lpfnWndProc	= &TrDesktop::WndProc;
		wndclass.cbClsExtra		= 0;
		wndclass.cbWndExtra		= 0;
		wndclass.hInstance		= TheApp.m_hInstance;
		wndclass.hIcon			= NULL;
		wndclass.hCursor		= NULL;
		wndclass.hbrBackground	= NULL;
		wndclass.lpszMenuName	= NULL;
		wndclass.lpszClassName	= szClassName;
		wndclass.hIconSm		= NULL;

		m_wndClass = ::RegisterClassEx(&wndclass);
	}

	m_hwnd = ::CreateWindow(szClassName, "", WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, CW_USEDEFAULT, 100, 100, NULL, NULL, TheApp.m_hInstance, NULL);

	if (m_hwnd == NULL) throw RLException("in InitWindow(), CreateWindow() error=%d", ::GetLastError());

	// Set the "this" pointer for the window
	::SetWindowLong(m_hwnd, GWL_USERDATA, (long)this);

	// Enable clipboard hooking
	m_hnextviewer = ::SetClipboardViewer(m_hwnd);
}


void TrDesktop::CreateBuffers()
{
	m_capture.CaptureInit();

	m_bmrect.left	= m_capture.m_screenX;
	m_bmrect.top	= m_capture.m_screenY;
	m_bmrect.right	= m_bmrect.left + m_capture.m_screenCX;
	m_bmrect.bottom	= m_bmrect.top  + m_capture.m_screenCY;

	_log2.Print(LL_INF, VTCLOG("attempting to create main and back buffers"));

	// Create changes buffer 
	int chnageBuffSize = m_capture.m_screenCX * m_capture.m_screenCY;
	if ((m_chngbuff = new char[chnageBuffSize]) == NULL)
		throw RLException("unable to allocate changes buffer[%d]", chnageBuffSize);
}

UINT16 TrDesktop::InitDesktop()
{
	_log2.Print(LL_INF, VTCLOG("InitDesktop()#0"));
	
	try {

		// Currently, we just check whether we're in the console session, and fail if not
		if (TrMain::IsOutConsole()) {
			_log.WriteError("Current session has not console");
			return aaErrorSessionInactive;
		}

		this->FillGeometry();	// just to check that all is fine

		// Spawn a thread to handle that window's message queue
		this->start_undetached(); // Start the thread
	}
	catch(RLException& ex) 
	{
		if (!TheApp.m_CmgArgs.noGUI) {
			CStringA msg = ex.GetDescription();
			msg += "\nAMMYY Target cannot be used with this graphic device driver";
			::MessageBox(NULL, (LPCSTR)msg, TheApp.m_appName, MB_ICONSTOP | MB_OK);
		}
		else {
			_log.WriteError(ex.GetDescription());
		}

		return aaDesktopInitError;
	}

	_log2.Print(LL_INF, VTCLOG("InitDesktop()#2"));

	return aaErrorNone;
}

void TrDesktop::SetLocalClipboard(LPSTR text)
{
	try {
		TrDesktop::SetLocalClipboard(m_hwnd, text, strlen(text));
	}
	catch(RLException& ex) {
		_log.WriteError(ex.GetDescription()); // it's not critical error just log to file
	}
}


void TrDesktop::SetLocalClipboard(HWND hwnd, LPCSTR textUTF8, int textLen)
{
	// Copy to wincontents replacing LF with CR-LF
	RLStream contentWin(textLen*2 + 1);

	LPSTR  pContentWin  = (LPSTR)contentWin.GetBuffer();;
	LPCSTR pContentWinBegin = pContentWin;
	LPCSTR pContentUnix = textUTF8;

	while(true) {
		char c = *pContentUnix++;
        if (c == '\x0a') {
			*pContentWin++ = '\x0d';
        }
		*pContentWin++ = c;
		if (c==0) break;
	}

	CStringW text = CCommon::ConvToUTF16(pContentWinBegin);

	// Open the system clipboard
	if (::OpenClipboard(hwnd)==0)
		throw RLException("Failed to open clipboard");
        
	if (::EmptyClipboard()==0) {
		::CloseClipboard();
		throw RLException("Failed to empty clipboard");
	}

    // Allocate a global memory object for the text. 
	int len = (text.GetLength() + 1) * sizeof(WCHAR);
    HGLOBAL hglbCopy = ::GlobalAlloc(GMEM_DDESHARE, len);
    if (hglbCopy != NULL) { 
		// Lock the handle and copy the text to the buffer.  
		LPVOID lptstrCopy = ::GlobalLock(hglbCopy);
	    memcpy(lptstrCopy, (LPCWSTR)text, len);
		::GlobalUnlock(hglbCopy);          // Place the handle on the clipboard.  
		::SetClipboardData(CF_UNICODETEXT, hglbCopy);
    }

    if (::CloseClipboard()==0)
        throw RLException("Failed to close clipboard");
}







// Window procedure for the Desktop window
LRESULT TrDesktop::WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	TrDesktop *_this = (TrDesktop*)GetWindowLong(hwnd, GWL_USERDATA);

	switch (iMsg)
	{
	case WM_SYSCOLORCHANGE:
	case WM_PALETTECHANGED:
		// The palette colours have changed, so tell the server

		// Get the system palette
		if (!_this->m_capture.SetPalette())
			PostQuitMessage(0);
		// Update any palette-based clients, too
		_this->UpdatePalette();
		return 0;

	// CLIPBOARD MESSAGES

	case WM_CHANGECBCHAIN:
		// The clipboard chain has changed - check our nextviewer handle
		if ((HWND)wParam == _this->m_hnextviewer)
			_this->m_hnextviewer = (HWND)lParam;
		else
			if (_this->m_hnextviewer != NULL)
				SendMessage(_this->m_hnextviewer, WM_CHANGECBCHAIN, wParam, lParam);

		return 0;

	case WM_DRAWCLIPBOARD:
		{
			_this->OnLocalClipboardChange();

			// Pass the message to the next window in clipboard viewer chain.  
			if (_this->m_hnextviewer != NULL)
				return SendMessage(_this->m_hnextviewer, WM_DRAWCLIPBOARD, 0,0);
			
			return 0;
		}

	default:
		return DefWindowProc(hwnd, iMsg, wParam, lParam);
	}
}


void TrDesktop::OnLocalClipboardChange()
{
	HWND hOwner = ::GetClipboardOwner();
	if (hOwner == m_hwnd)
		return;
		
	if (!m_initialClipboardSeen) {	// Don't send initial clipboard!
		m_initialClipboardSeen = true;
		return;
	} 

	if (m_clipboard_active)
	{				
		if (::OpenClipboard(m_hwnd)!=0) 
		{
			HGLOBAL hglb2 = ::GetClipboardData(CF_UNICODETEXT); 

			if (hglb2 == NULL) {
				::CloseClipboard();
			} else {
				LPCWSTR lpstr = (LPCWSTR)::GlobalLock(hglb2);

				RLStream utf8;
				CCommon::ConvToUTF8(lpstr, utf8);
				
				::GlobalUnlock(hglb2);
				::CloseClipboard();
				
				// Translate to Unix-format lines before sending
				char* pContent     = (char*)utf8.GetBuffer();
				char* pContentWin  = pContent;
				char* pContentUnix = pContent;
				while(true) {
					char c = *pContentWin++;
					if (c != '\x0d') {
						*pContentUnix++ = c;
						if (c==0) break;
					}
				}
				//int contentLen = pContentUnix-pContent-1;
				
				m_client->SendClipboardText(pContent); // Now send the unix text
			}
		}
	}
}


HDESK TrDesktopSelector::GetThreadDesktop()
{
	HDESK hDesktop = ::GetThreadDesktop(::GetCurrentThreadId());
	if (hDesktop==NULL)
		_log.WriteError("GetThreadDesktop() Error=%d", ::GetLastError());

	return hDesktop;
}

// to prevent error 170 on SetThreadDesktop() on Lihvan's computers
// we need to call this method to avoid SetThreadDesktop to the same desktop
//
void TrDesktopSelector::SetNameOfDesktop(HDESK hDesktop)
{
	if (hDesktop==NULL) hDesktop = TrDesktopSelector::GetThreadDesktop();

	char desktopName[256];
	this->GetDesktopName(hDesktop, desktopName, sizeof(desktopName));
	m_name = desktopName;
}

void TrDesktopSelector::GetDesktopName(HDESK hdesk, LPSTR buffer, DWORD bufferLen)
{
	DWORD dummy;
	if (!GetUserObjectInformation(hdesk, UOI_NAME, buffer, bufferLen, &dummy))
		throw RLException("GetUserObjectInformation() error=%d", ::GetLastError());

	if (dummy>=bufferLen) throw RLException("GetDesktopName() ERROR %d %d", dummy, bufferLen);	
	if (buffer[0]==0) throw RLException("GetDesktopName() ERROR#2");
}



void TrDesktopSelector::CloseDesktopHandle()
{
	if (m_handle==0) return;
	if (!::CloseDesktop(m_handle))
		_log2.Print(LL_ERR, VTCLOG("CloseDesktop(%d) '%s' error=%d"), m_handle, (LPCSTR)m_name, ::GetLastError());
	m_handle = 0;
}
	

// if changed return true
//
void TrDesktopSelector::SetInputDesktop()
{
	// Get the input desktop
	HDESK hDesktop = ::OpenInputDesktop(0, FALSE,
			DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW | DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
			DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS | DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);

	m_available = (hDesktop!=NULL);

	// Get name of input desktop
	if (m_available)
	{
		char desktopName[256];
		this->GetDesktopName(hDesktop, desktopName, sizeof(desktopName));
		if (m_name==desktopName) {
			if (!::CloseDesktop(hDesktop)) 
				_log2.Print(LL_ERR, VTCLOG("CloseDesktop()#1 error=%d"), ::GetLastError());
			return; // not changed
		}
		
		this->OnBeforeSetThreadDesktop(desktopName);
			
		if(::SetThreadDesktop(hDesktop)==0) 	// Switch to the new desktop
			throw RLException("SetThreadDesktop() error=%d, desktop='%s'", ::GetLastError(), desktopName);

		_log2.Print(LL_INF, VTCLOG("Desktop changed to '%s', from='%s'"), desktopName, m_name);
		this->CloseDesktopHandle();
		m_name = desktopName;
		m_handle = hDesktop;
		return; // changed
	}
	else {
		_log2.Print(LL_INF, VTCLOG("OpenInputDesktop() failed err=%d"), ::GetLastError());
	}
}


// send desktop changes, mouse move
//
void TrDesktop::CheckUpdates()
{
	bool firstCall = !m_check_update_called;
	m_check_update_called = true;

	//if (firstCall) this->SetNameOfDesktop(NULL);	// in Lihvan's computer works without it, I don't know why...
	bool available_prev = this->TrDesktopSelector::m_available;
	this->SetInputDesktop();

	// will try later if no desktop
	if (!this->TrDesktopSelector::m_available) {
		if (firstCall || available_prev) {
			UINT8 msg = aaDesktopUnavailable;
			m_client->m_transport->SendExact(&msg, sizeof(msg), TRUE);
		}
		return;
	}

	bool displaychanged = (m_hwnd==NULL); // if first call, so displaychanged=trues

	if (!displaychanged) {	
		RECT rect;
		m_capture.GetScreenRect(rect);
		displaychanged = !::EqualRect(&rect, &m_bmrect);
	}

	if (!displaychanged) {
		TrDesktopCapture capture;
		capture.CaptureInitPixelFormatOnly();
		displaychanged = !(capture.m_frmL == m_capture.m_frmL);
	}

	// it's first call or the display resolution changed
	if (displaychanged) {
		_log2.Print(LL_INF, VTCLOG("display resolution or desktop changed %X"), (int)m_hwnd);

		Shutdown();
		Startup();
			
		_log2.Print(LL_INF, VTCLOG("SCR: new screen format %dx%dx%d"), m_capture.m_screenCX, m_capture.m_screenCY, m_capture.m_frmL.bitsPerPixel);

		this->UpdatePalette();
	}

	bool bFullRgnRequested;
	
	{
		RLMutexLock l(m_full_refresh_lock);
		bFullRgnRequested = m_full_refresh || displaychanged;
		m_full_refresh = false;
	}

	m_changed_rgn.Clear();

	// If we have full region update request
	if (bFullRgnRequested)
	{		
		m_capture.CaptureFullScreen(); // Capture screen to main buffer

		//RLTimer t1;
			
		m_changed_rgn.AddRect(0, 0, m_capture.m_screenCX, m_capture.m_screenCY); // add full screen for sending
			
		//t1.Stop();			
		//_log.WriteInfo("--FullRgnRequested----------  %0.2Lfms", t1.GetTime());
	}
	else {	// If we have incremental update request		
		//RLTimer t1;
		//RLTimer t2;
		//RLTimer t3;

		if (m_copyrect_use) {
			m_copyRects.FindWindowsMovement(m_bmrect);
		}

		//t1.Stop();

		m_capture.CaptureFullScreen(); // Capture screen to main buffer
		m_copyRects.m_bytes_per_pixel = m_capture.m_frmL.bitsPerPixel / 8;
		m_copyRects.m_screenCX = m_capture.m_screenCX;
		m_copyRects.m_screenCY = m_capture.m_screenCY;
		m_copyRects.m_mainbuff = m_capture.GetBuffer(true);  // main buffer
		m_copyRects.m_backbuff = m_capture.GetBuffer(false); // back buffer
		m_copyRects.CheckSimple();

		//t2.Stop();

		TrDesktopComparator comparator;
		comparator.m_screenCX = m_capture.m_screenCX;
		comparator.m_screenCY = m_capture.m_screenCY;
		comparator.m_mainbuff = m_copyRects.m_mainbuff;
		comparator.m_backbuff = m_copyRects.m_backbuff;
		comparator.m_chngbuff = m_chngbuff;
		comparator.m_frmL.bitsPerPixel = m_capture.m_frmL.bitsPerPixel;
		comparator.DoCompare(m_changed_rgn);
	}
		
	this->SendUpdates();
}


void TrDesktop::SendUpdates()
{
	_log2.Print(LL_INF, VTCLOG("SendUpdates()#1"));

	Transport* m_transport = m_client->m_transport;

	{
		// Lock the updates stored so far
		omni_mutex_lock l(m_client->m_regionLock);

		// Check if cursor shape update has to be sent
		bool cursor_update_pending = this->IsCursorUpdatePending();

		// Send an update if one is waiting
		if (!m_changed_rgn.IsEmpty() || m_copyRects.m_movementsOut.GetCount()>0 || cursor_update_pending)
		{
			// if the palette changed
			/*
			if (m_palettechanged) {
				m_palettechanged = false;
				SendPalette();
			}
			*/

			try {
				m_transport->SendLock();

				//SendScreenUpdateMsg();
				{
					m_transport->SendByte(aaScreenUpdate);
					::InterlockedIncrement(&m_dwUncommitedScreenUpdates);
				}
				
				SendNewDesktopFormatAndSize();

				// Send mouse cursor shape, if changes in cursor
				if (cursor_update_pending) SendCursorShape();

				SendCopyRects();		// Send the copyrects
				SendRectangles();	// Encode & send the actual rectangles

				// Send LastRect marker indicating that there are no more rectangles
				m_transport->SendByte(aaEncoderLastRect);

				m_transport->SendUnlock();
			}
			catch (...)
			{
				m_transport->SendUnlock();
				throw;
			}
		}
		SendCursorPosition();
	}

	_log2.Print(LL_INF, VTCLOG("SendUpdates()#2"));
	
	//wait while all data are sent
	m_transport->SendExact(NULL, 0, TRUE);
}

void TrDesktop::UpdatePalette()
{
	omni_mutex_lock l(m_client->m_regionLock);
	m_palettechanged = true;
}




//______________________________________________________________________________________________

void TrDesktop::FreeEncoder()
{
	if (m_encoder != NULL)
	{
		m_encoder->LogStats();
		delete m_encoder;		
		m_encoder = NULL;
	}
}

void TrDesktop::SetEncoder(aaSetEncoderMsg* msg)
{
	// Delete the old encoder
	this->FreeEncoder();

	_log2.Print(LL_INF, VTCLOG("Encoder %u requested"), msg->encoder);

	switch(msg->encoder)
	{
		case aaEncoderRaw:		m_encoder = new TrEncoder;			break;
		//case aaEncoderRRE:	m_encoder = new TrEncoderRRE;		break;
		//case aaEncoderCoRRE:	m_encoder = new TrEncoderCoRRE;		break;
		case aaEncoderAAFC:		m_encoder = new TrEncoderHexT;		break;
		case aaEncoderAAC:		m_encoder = new TrEncoderAAC;		break;
		//case aaEncoderZlib:	m_encoder = new TrEncoderZlib();	break;
		//case aaEncoderTight:	m_encoder = new TrEncoderTight();	break;
		//case aaEncoderZlibHex:	m_encoder = new TrEncoderZlibHex();	break;
	}

	if (m_encoder == NULL) 
		throw RLException("Unable to set encoder %u", msg->encoder);


	aaPixelFormat& frmNetwork = GetNetworkFormat(m_capture.m_frmL, msg->pixelFrm, msg->colorQuality);

	if (msg->qualityLevel<0 || msg->qualityLevel > aaJpegOFF)
		throw RLException("Invalid Quality Level=%d", msg->qualityLevel);

	if (msg->compressLevel<-1 || msg->compressLevel > 9) 
		throw RLException("Invalid Compress Level=%d", msg->compressLevel);

	// Initialise it and give it the pixel format
	m_encoder->m_compresslevel = msg->compressLevel;
	m_encoder->m_qualitylevel  = msg->qualityLevel;
	m_encoder->SetFormats(m_capture.m_frmL, frmNetwork, m_capture.m_screenCX, m_capture.m_screenCY);
	m_encoder->m_pOutBuffer = m_client->m_transport;
}

// Check if cursor shape update should be sent
bool TrDesktop::IsCursorUpdatePending()
{
	if (m_client->m_send_cursor_shape)
	{		
		CURSORINFO curinfo;
		curinfo.cbSize = sizeof(CURSORINFO);
		curinfo.hCursor = 0;

		if (::GetCursorInfo(&curinfo)==0)
			_log.WriteError("GetCursorInfo() error=%d", ::GetLastError());

		// sometimes occur when did "Switch user" this changes desktop
		//if (curinfo.hCursor==NULL) 	_log.WriteError("Cursor is NULL");
		
		if (m_hcursor !=curinfo.hCursor) {
			m_hcursor = curinfo.hCursor;
			return true;
		}
	}
	return false;
}

// ______________________________________________________________________________________________

//
// New code implementing cursor shape updates.
//


void TrDesktop::SendCursorShape()
{
	HCURSOR hcursor = m_hcursor;

	ICONINFO cursorInfo = {0};
	RLStream mbitsWrapper; // bits of mask & color for mono
	RLStream cbitsWrapper; // bits for color
	int cx, cy;
	int maskRowBytes;

	try {
		if (hcursor == NULL) {
			SendCursorShape(false, mbitsWrapper, cbitsWrapper, 0, 0, 0, 0);
			return; // sent empty cursor
		}

		// Get cursor info
		if (!::GetIconInfo(hcursor, &cursorInfo))
			throw RLException("GetIconInfo() failed %u", ::GetLastError());

		if (cursorInfo.hbmMask == NULL)
			throw RLException("cursor bitmap handle is NULL");

		// Check bitmap info for the cursor
		BITMAP bmMask;
		if (!::GetObject(cursorInfo.hbmMask, sizeof(BITMAP), (LPVOID)&bmMask))
			throw RLException("GetObject() for bitmap failed");

		if (bmMask.bmPlanes != 1 || bmMask.bmBitsPixel != 1)
			throw RLException("incorrect data in cursor bitmap");
		
		cx = bmMask.bmWidth;
		cy = bmMask.bmHeight;
		maskRowBytes = bmMask.bmWidthBytes;

		// Get monochrome bitmap data for cursor
		// NOTE: they say we should use GetDIBits() instead of GetBitmapBits().		
		BYTE* p_mbits = (BYTE *)mbitsWrapper.GetBuffer1(bmMask.bmWidthBytes * cy);

		//int v = ::GetBitmapBits(cursorInfo.hbmMask, bmMask.bmWidthBytes * cy, p_mbits);

		if (!::GetBitmapBits(cursorInfo.hbmMask, bmMask.bmWidthBytes * cy, p_mbits))
			throw RLException("GetBitmapBits() failed");

		if (cursorInfo.hbmColor == NULL) {
			cy /= 2; // mono cursor
		}
		else {
			m_capture.CaptureBitmap(cursorInfo.hbmColor, cx, cy, cbitsWrapper);
			//CDebug2::SaveBMP("c:\\tmp\\cursor.bmp", cx, cy, cbitsWrapper.GetBuffer(), 32);
		}
	}
	catch(RLException& ex) {
		_log.WriteError("SendCursorShape() %s",  ex.GetDescription());
		mbitsWrapper.Free(); // mark that was error
	}

	if (cursorInfo.hbmMask!=NULL)  ::DeleteObject(cursorInfo.hbmMask);
	if (cursorInfo.hbmColor!=NULL) ::DeleteObject(cursorInfo.hbmColor);


	// SENDING

	RLStream newMask;
	RLStream newColors;

	bool isColor = false;

	if (mbitsWrapper.GetCapacity()==0) {
		// need send empty cursor to reset previous cursor on operator side if error of sending real cursor
		cx = cy = cursorInfo.xHotspot = cursorInfo.yHotspot = 0;
	}
	else {
		BYTE* p_mbits = (BYTE *)mbitsWrapper.GetBuffer();
		isColor = (cbitsWrapper.GetCapacity()!=0);
		BYTE* cbits = (isColor) ? (BYTE*)cbitsWrapper.GetBuffer() : NULL;
		this->FixCursorMask(p_mbits, cbits, cx, cy, maskRowBytes, newMask, newColors);
	}
	
	this->SendCursorShape(isColor, newMask, newColors, cursorInfo.xHotspot, cursorInfo.yHotspot, cx, cy);
}

void TrDesktop::FixCursorMask(BYTE *mbits, BYTE *cbits, int cx, int cy, int width_bytes, RLStream& newMask,  RLStream& colors)
{
	int pixels = cx * cy;

	int mask_len = (pixels+7)/8;
	BitArray bitMW(newMask.GetBuffer1(mask_len)); // mask writer
	newMask.SetLen(mask_len);

	if (cbits!=NULL)
	{
		BYTE* pColors = (BYTE*)colors.GetBuffer1(pixels*4);
		int bytesPP = m_encoder->m_bytesppL;

		for (int y=0; y<cy; y++) 
		{
			BitArray bitMR(mbits + y*width_bytes); // Mask Reader

			for (int x=0; x<cx; x++) {
				bool b = !bitMR.Read(); // invert it
				bitMW.Write(b);

				if (b) {
					TrEncoder::CopyPixelDDB(pColors, cbits, bytesPP);
					pColors += bytesPP;
				}

				cbits += bytesPP;
			}
		}

		colors.SetLen(pColors - (BYTE*)colors.GetBuffer());
	}
	else {
		BitArray bitCW(colors.GetBuffer1(mask_len));	// color Writter

		for (int y=0; y<cy; y++) 
		{
			BitArray bitMR(mbits + width_bytes*y);		// Mask Reader
			BitArray bitCR(mbits + width_bytes*(y+cy)); // Color Reader

			for (int x=0; x<cx; x++) 
			{
				bool m1 = bitMR.Read(); // invert bit
				bool c1 = bitCR.Read(); // invert bit

				// I don't know why, but I found it in v2.13, and it's work!
				bool m = !m1 | (m1 & c1);
				bool c = !c1 | (m1 & c1);

				bitMW.Write(m);

				if (m) {
					bitCW.Write(c);
				}
			}
		}
		colors.SetLen(bitCW.GetSize());
	}
}

void TrDesktop::SendCursorShape(bool isColor, RLStream& mask, RLStream& color, int xhot, int yhot, int cx, int cy)
{
	aaFramebufferUpdateRectHeader hdr;
	hdr.encoder = (isColor) ? aaEncoderCursorRich : aaEncoderCursorMono;
	hdr.r.x = xhot;
	hdr.r.y = yhot;
	hdr.r.w = cx;
	hdr.r.h = cy;
	
	RLStream buffer(sizeof(hdr) + mask.GetLen() + cx*cy*4);
	buffer.AddRaw(&hdr, sizeof(hdr));
	buffer.AddRaw(mask.GetBuffer(),  mask.GetLen()); // mask

	if (isColor)
	{
		int pixels = color.GetLen() / m_encoder->m_bytesppL;
		// Translate colors to client pixel format
		m_encoder->m_translator->Translate(buffer.GetBufferWr(), color.GetBuffer(), pixels);
		buffer.AddRaw(NULL, pixels*m_encoder->m_bytesppR);
	}
	else {
		buffer.AddRaw(color.GetBuffer(), color.GetLen()); // color
	}

	m_client->m_transport->SendExact(buffer.GetBuffer(), buffer.GetLen());
}

// Send New desktop pseudo-rectangle to notify the client about framebuffer size change
void TrDesktop::SendNewDesktopFormatAndSize()
{
	RLStream stream(256);

	UINT8 flags = 0;

	bool bSize = false;

	{
		int cx = m_capture.m_screenCX;
		int cy = m_capture.m_screenCY;
	
		if (m_desktop_last_size.cx!=cx || m_desktop_last_size.cy!=cy)
		{
			flags |= aaEncoderChangedSize;
			bSize = true;
			m_desktop_last_size.cx = cx;
			m_desktop_last_size.cy = cy;
		}
	}


	bool bFrmL = !(m_desktop_last_frml == m_capture.m_frmL);
				   m_desktop_last_frml =  m_capture.m_frmL;

	bool bNewRequest = (m_aaSetEncoderMsg.type!=0);

	if (bNewRequest) {
		m_aaSetEncoderMsg.type=0; // mark as clear
		// Enable CursorPos encoding only if cursor shape updates were requested by the client
		m_copyrect_use = (m_aaSetEncoderMsg.copyRect>0);
	}

	if (bNewRequest || bFrmL) flags |= aaEncoderChangedEncoder;

	if (flags==0) return; // no changes

	// Apply Encoder
	this->SetEncoder(&m_aaSetEncoderMsg);
		
	stream.AddUINT8(aaEncoderChanged);
	stream.AddUINT8(flags);
	if (flags&aaEncoderChangedEncoder) {
		stream.AddUINT8(m_aaSetEncoderMsg.encoder);
		stream.AddUINT8(m_aaSetEncoderMsg.qualityLevel);
		stream.AddRaw(&(m_encoder->m_frmR), sizeof(aaPixelFormat));
		stream.AddRaw(&(m_encoder->m_frmL), sizeof(aaPixelFormat));
	}
	
	if (flags&aaEncoderChangedSize) {
		stream.AddUINT16(m_capture.m_screenCX);
		stream.AddUINT16(m_capture.m_screenCY);
	}	

	m_client->m_transport->SendExact(stream.GetBuffer(), stream.GetLen());
}

// Send a set of rectangles
void TrDesktop::SendRectangles()
{
	rectlist rects;		// list of rectangles to actually send

	if (!m_changed_rgn.Rectangles(rects)) return; // no any rectangles
	
	m_encoder->m_source = m_capture.GetBuffer(true);
	m_encoder->OnBegin();

	// Work through the list of rectangles, sending each one
	while(!rects.empty())
	{
		RECT rect = rects.front();
		
		// Call the encoder to encode the rectangle
		m_encoder->EncodeRectBase(rect);
				
		rects.pop_front();
	}
}

// Send a CopyRect messages
void TrDesktop::SendCopyRects()
{
	int countMovements = m_copyRects.m_movementsOut.GetCount();
	
	if (countMovements==0) return;

	RLStream buffer(countMovements*sizeof(aaCopyRect));

	for (int i=0; i<countMovements; i++) {
		TrDesktopCopyRect::Item item;
		m_copyRects.m_movementsOut.GetItem(i, item);

		// Create the message header
		aaCopyRect copyrecthdr;
		copyrecthdr.encoder = aaEncoderCopyRect;
		copyrecthdr.r.x = item.xd();
		copyrecthdr.r.y = item.yd();
		copyrecthdr.r.w = item.cx;
		copyrecthdr.r.h = item.cy;
		copyrecthdr.srcX = item.xs;
		copyrecthdr.srcY = item.ys;

		buffer.AddRaw(&copyrecthdr, sizeof(copyrecthdr));
	}

	m_client->m_transport->SendExact(buffer.GetBuffer(), buffer.GetLen());
}

void TrDesktop::SendCursorPosition()
{
	//_log2.Print(LL_INF, VTCLOG("SendCursorPosition()#0 %d %d"), m_cursor_pos.x, m_cursor_pos.y);

	if (!m_client->m_send_cursor_position) return; // operator didn't ask to send it
	
	if (m_pointer_event_timer.GetElapsedSeconds() <= 1) return; // less than 1 second

	// Prepare to send cursor position update if necessary
	POINT cursor_pos;
	if (!::GetCursorPos(&cursor_pos)) {
		cursor_pos.y = cursor_pos.x = 0;
	}

	//_log2.Print(LL_INF, VTCLOG("SendCursorPosition()#1 %d %d %f"), cursor_pos.x, cursor_pos.y, d);

	// if no changes, it means send ONLY if local user move cursor
	// if operator move cursor, we don't send
	if (cursor_pos.x == m_cursor_pos.x && cursor_pos.y == m_cursor_pos.y) return;

	//_log2.Print(LL_INF, VTCLOG("SendCursorPosition()#2 %d %d"), cursor_pos.x, cursor_pos.y);

	m_cursor_pos = cursor_pos;

	aaPointerMoveMsg msg;
	msg.type = aaPointerMove;
	msg.x = m_cursor_pos.x - m_capture.m_screenX;
	msg.y = m_cursor_pos.y - m_capture.m_screenY;
	msg.counter = m_client->m_counterAaPointerEvent;

	m_client->m_transport->SendExact(&msg, sizeof(msg));
}

/*
// Send the encoder-generated palette to the client
void TrDesktop::SendPalette()
{	
	const UINT ncolours = 256;
	RGBQUAD rgbquad[ncolours];	// Reserve space for the colour data
	
	// Try to get the RGBQUAD data from the encoder
	// This will only work if the remote client is palette-based,
	// in which case the encoder will be storing RGBQUAD data
	if (m_encoder == NULL)
	{
		throw RLException("SendPalette called but no encoder set");
	}
	
	aaSetColourMapEntriesMsg setcmap;
	setcmap.type = aaSetColourMapEntries;
	setcmap.firstColour = 0;
	setcmap.nColours = ncolours;

	RLStream buffer(sizeof(setcmap)+ncolours*6);
	buffer.AddRaw(&setcmap, sizeof(setcmap));

	// Now send the actual colour data...
	for (UINT i=0; i<ncolours; i++)
	{
		UINT16 r = ((UINT16)rgbquad[i].rgbRed)   << 8;
		UINT16 g = ((UINT16)rgbquad[i].rgbGreen) << 8;
		UINT16 b = ((UINT16)rgbquad[i].rgbBlue)  << 8;

		buffer.AddRaw(&r, sizeof(UINT16));
		buffer.AddRaw(&g, sizeof(UINT16));
		buffer.AddRaw(&b, sizeof(UINT16));
	}

	m_client->m_transport->SendExact(buffer.GetBuffer(), buffer.GetLen(), TRUE);
}
*/


aaPixelFormat& TrDesktop::GetNetworkFormat(aaPixelFormat& frmT, aaPixelFormat& frmV, UINT8 frmSettings)
{
	if (frmSettings==aaPixelFormat8_gray)	return TrTranslator::f_y8;
	if (frmSettings==aaPixelFormat8)		return TrTranslator::f_r3g3b2;

	aaPixelFormat& frmMin = (frmT.bitsPerPixel > frmV.bitsPerPixel) ? frmV : frmT;

	if (frmSettings==aaPixelFormat32) {
		return frmMin;
	}
	
	if (frmSettings==aaPixelFormat24) {
		return (frmMin.bitsPerPixel>24) ? TrTranslator::f_r8g8b8 : frmMin;
	}
	
	if (frmSettings==aaPixelFormat16) {
		return (frmMin.bitsPerPixel>16) ? TrTranslator::f_r5g6b5 : frmMin;
	}
	
	throw RLException("Error in GetNetworkFormat()");
}

