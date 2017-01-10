// The TrDesktop object handles retrieval of data from the display buffer.

#if !defined(_TR_DESKTOP_H__INCLUDED_)
#define _TR_DESKTOP_H__INCLUDED_

class TrDesktopSelector
{
public:
	TrDesktopSelector()  { m_handle=0;}
	~TrDesktopSelector() { CloseDesktopHandle(); }
	void SetInputDesktop();
	void SetNameOfDesktop(HDESK hDesktop);

	static HDESK GetThreadDesktop();

	/*
	static CStringA GetInputDesktopName()
	{
		// Get the input desktop
		HDESK hDesktop = ::OpenInputDesktop(0, FALSE,
				DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW | DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
				DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS | DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);

		if (hDesktop==NULL) return "";

		char desktopName[256];
		GetDesktopName(hDesktop, desktopName, sizeof(desktopName));

		::CloseDesktop(hDesktop);

		return CStringA(desktopName);
	}
	*/

	/*
	static CStringA GetThreadDesktopName()
	{
		HDESK hDesktop = ::GetThreadDesktop(::GetCurrentThreadId());
		if (hDesktop==NULL) return "";

		char desktopName[256];
		GetDesktopName(hDesktop, desktopName, sizeof(desktopName));
		return CStringA(desktopName);
	}
	*/


protected:
	virtual void OnBeforeSetThreadDesktop(LPCSTR newDesktopName) {}

private:
	void CloseDesktopHandle();
	static void GetDesktopName(HDESK hdesk, LPSTR buffer, DWORD bufferLen);

	HDESK	m_handle;
protected:
	CStringA m_name;	  // name of desktop for m_handle
	bool     m_available; // desktop is unavailable
};


#include "TrDesktopCapture.h"
#include "TrDesktopComparator.h"
#include "TrDesktopCopyRect.h"
#include "TrClient.h"
#include "TrRegion.h"
#include "TrEncoder.h"
#include "../common/omnithread/omnithread.h"
#include "../RL/RLEvent.h"


class TrDesktop : public TrDesktopSelector, public omni_thread
{

// from TrDesktopThread
private:
	virtual void *run_undetached(void *arg);
	DWORD LoopMessagesAndWaiting(DWORD nCount, HANDLE *pHandles, DWORD dwMilliseconds);

// Methods
public:
	TrDesktop(TrClient* client);
	~TrDesktop();

	virtual void OnBeforeSetThreadDesktop(LPCSTR newDesktopName) // from TrDesktopSelector
	{
		//_log.WriteInfo("OnBeforeSetThreadDesktop() %s %s", newDesktopName, m_desktopNameForOptimizer);
		Shutdown();
	}

	UINT16 InitDesktop();

	// created for debug purposes. not used in normal builds, comment by maxim
	void DebugDumpSurfBuffers(const RECT &rcl, bool main, bool back);

	void SetLocalClipboard(LPSTR text);
	static void SetLocalClipboard(HWND hwnd, LPCSTR textUTF8, int textLen);
	static aaPixelFormat& GetNetworkFormat(aaPixelFormat& frmT, aaPixelFormat& frmV, UINT8 frmSettings);

private:
	void SetEncoder(aaSetEncoderMsg* msg); // CONFIGURING Encoder
	bool IsCursorUpdatePending(); // SENDING CURSOR SHAPE UPDATES
	void SendUpdates();
	void SendCursorShape();
	void SendNewDesktopFormatAndSize();
	void SendRectangles();
	void SendCopyRects();
	void SendCursorPosition();
	void SendPalette();
	void FreeEncoder();
	void Startup();
	void Shutdown();
	void FillGeometry();
	void FillMouseDividers();
	void UpdatePalette();

	// Init routines called by the child thread
	static void KillScreenSaver();
	static BOOL CALLBACK KillScreenSaverFunc(HWND hwnd, LPARAM lParam);	

	void InitWindow();
	
	void CreateBuffers();
	void FixCursorMask(BYTE *mbits, BYTE *cbits, int width, int height, int width_bytes, RLStream& newMask, RLStream& colors);
	void SendCursorShape(bool isColor, RLStream& mask, RLStream& color, int xhot, int yhot, int cx, int cy);
	
private:
	// Detecting updates
	void CheckUpdates();
	void OnLocalClipboardChange();

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);	

public:
	aaSetEncoderMsg m_aaSetEncoderMsg;	// pending encoder

	TrRegion	m_changed_rgn;	// for sending
	RLEvent		m_eventRemoteKey;
	RLEvent		m_eventUpdate;
	RLEvent		m_eventQuit;
	volatile LONG	m_dwUncommitedScreenUpdates;
	RLTimer			m_pointer_event_timer;
	POINT			m_cursor_pos;
	int				m_mouseDividerX, m_mouseDividerY;
	TrDesktopCapture	m_capture;
	bool				m_full_refresh;		// if full screen update is requested
	RLMutex				m_full_refresh_lock;

private:
	// Generally useful stuff
	TrClient*		m_client;
	HWND			m_hwnd;
	HWND			m_hnextviewer;
	BOOL			m_clipboard_active;
	bool			m_check_update_called; // true after first real call CheckUpdate()
	SIZE			m_desktop_last_size;
	aaPixelFormat	m_desktop_last_frml; // last sent format local
	TrDesktopCopyRect m_copyRects;
	RECT			m_cursorpos;	// The current mouse position
	char*			m_chngbuff;	// buffer of changes, true - if pixel was changed
	CStringA		m_desktopNameForOptimizer;
	TrEncoder*		m_encoder;
	RECT			m_bmrect; // frame buffer relative to the entire (virtual) desktop, NOTE: non-zero offset possible	
	bool			m_palettechanged;	// When the local display is palettized, it sometimes changes...
	HCURSOR			m_hcursor;		// Used to track cursor shape changes
	bool			m_copyrect_use;
	bool			m_initialClipboardSeen;
};

#endif // _TR_DESKTOP_H__INCLUDED_
