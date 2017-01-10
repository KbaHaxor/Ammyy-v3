#ifndef _VRCLIENT_H__INCLUDED_
#define _VRCLIENT_H__INCLUDED_

#include <vector>
#include "../main/aaProtocol.h"
#include "../main/aaDesktop.h"
#include "vrOptions.h"
#include "vrKeyMap.h"
#include "VrFm1.h"
#include "../main/sound/AudioIn.h"
#include "../main/sound/AudioOut.h"
#include "../main/InteropViewer.h"
#include "../common/omnithread/omnithread.h"
#include "../main/CmdSessionEnded.h"
#include "VrDlgSpeedTest.h"

#include "VrEncoder.h"


class VrClient : public VrEncoders
{
	friend class VrOptions;

public:
	VrClient();
	~VrClient();
	void Run();
	void OnExitProcess();

	CStringA m_host; // used only in case of direct connection

private:
	virtual void Thread01();
	static LRESULT CALLBACK WndProc1_static(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);	
	static LRESULT CALLBACK WndProc2_static(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	inline LRESULT          WndProc1       (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	inline LRESULT          WndProc2       (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK WndProcDefW(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK WndProcScroll(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
	void OnPaint();

	inline bool IsToolbarButtonPressed(WPARAM id)
	{
		LRESULT l = ::SendMessage(m_hToolbar, TB_GETSTATE, id, 0);
		//if (l!=4 && l!=5) throw RLException("IsToolbarButtonPressed()#1 Error");
		return (l & TBSTATE_CHECKED);
	}


	VrFmMainWindow  m_FmMainWindow;
	
private:
	HMENU m_hMenu;
	HWND m_hwnd1;	   // main window
	HWND m_hwndscroll;
	HWND m_hwnd2;	   // child window
	HWND m_hToolbar, m_hwndToolBarLine;

	void OnTransportError(TransportException& ex, LPCSTR where);
		
	void CloseTransport(bool sendTermMsg);

	void CreateDisplay();
	void CreateToolbar();
	void ReadInitMsg();
	void DoCrypt();
	void DoAuthorization();
	void CheckPermissions();

	void ReadOtherInfo();
	void DoDirectConnect();
	void SendSessionInfoToServer();
	void SetCursorLocal(int cursorLocal);
	void OnCtlOrAlt(UINT uId, UINT32 key);

private:

	struct DirectConnect 
	{
		UINT GetTimeOut()
		{
			if (fast) return 200;
			if (roundTrip<0) throw RLException("Invalid roundtrip");
			UINT t = (roundTrip<100) ? (roundTrip+50) : (roundTrip * 3 /2);
			return t;
		}

		DWORD dwTicksToStop;
		int roundTrip;		// round trip in milliseconds
		bool tryed;
		bool fast;          // fast connection in the same network
	};

	int  DoDirectConnectOutgoing(DirectConnect& dc, UINT32 ip, UINT16 port);
	int  DoDirectConnectIncoming(DirectConnect& dc, UINT32 ip, UINT16 port);
	static bool IsIPv4Included(const RLStream& ips, UINT32 ip);
	static void PushBackPort(RLStream& ports, UINT16 port);
	static void MakeConnectionKey(RLStream& bufferOut, UINT64 time1);
public:

	void FreeScreenBuffer();
	void CreateScreenBuffer();
	void DrawInitialScreen();
	
	void SendEncoderAndPointer(bool encoder, bool pointer, bool refresh);

	void SendScreenUpdateRequest();
	void SendScreenUpdateCommit(DWORD count);
	
	void SendPointerEvents(int x, int y, DWORD keyflags);
    void OnKeyEvent(int virtkey, DWORD keyData);
	void SendKeyEvent(UINT32 key, bool down);
	void SwitchOffKey();

	inline void OnAaSound();
	inline void OnAaScreenUpdate();
	       void OnAaPointerMove();
	inline void OnAaDesktopUnavailable();
	void ThrowError(int code);
	void Update(RECT *pRect);
	void SizeWindow(bool centered);
	void PositionChildWindow();
	bool ScrollScreen(int dx, int dy);
	void UpdateScrollbars();
	void EnableFullControlOptions();
	void EnableAction(int id, bool enable, bool checked=false);
	void SetStateToolbarButton(int id, bool enable, bool checked=false);

	virtual void InvalidateScreenRect(int x, int y, int cx, int cy); // called from Vrncoders
	void InvalidateScreenRect(const RECT *pRect);
	void InvalidateScreenRect(const aaRectangle *pRect);

	void ReadCopyRect();    

	void OnAaEncoderChanged();

private:

	// vrClientCursor.cpp
	class Cursor
	{
	public:
		bool IsInLockedArea();
	
	public:
		int x, y;
		int cx; // rcWidth
		int cy; // rcHeight

		bool  set;
		bool  lockset;
		bool  hidden;
		RLStream source; // in DDB format
		RLStream mask1;   //mask array bool[]
		int   hotX, hotY;
		int   lockX, lockY, lockCX, lockCY;

		RLStream screen; // screen area under cursor

		omni_mutex mutex;
	};	

	Cursor m_cursor;

	void ReadCursorShape(aaFramebufferUpdateRectHeader *pfburh);	
	void SoftCursorLockArea(int x, int y, int w, int h);
	void SoftCursorUnlockScreen();
	void SoftCursorMove(int x, int y);
	void SoftCursorFree();
	void SoftCursorSaveArea();
	void SoftCursorRestoreArea();
	void SoftCursorDraw();
	void SoftCursorToScreen(RECT *screenArea);

	// ClientConnectionFullScreen.cpp	
	void DoFullScreenMode(bool suppressPrompt);
	bool AutoScroll(int x, int y);

	// ClientConnectionClipboard.cpp
	void OnLocalClipboardChange();
	void UpdateLocalClipboard(char *buf, int len);
	void SendClientCutText(LPCSTR str, int len);
	void OnAaCutText();
	
	void ReadString(char *buf, int length);
	void WriteExact(char *buf, int bytes, BOOL block=FALSE);
	void WriteExact(RLStream& buffer, BOOL block=FALSE);

	// This is what controls the thread
	static UINT __stdcall run_undetached_static(void *);
	void	run_undetached();
	BOOL	m_killing;	// terminate thread inside run_undetached()
	bool	m_bCloseWindow; // destroy window after working thread is finished


    // Buffer for network operations	
	omni_mutex m_clipMutex;	
	omni_mutex m_transport_close_lock;
 
	// Keyboard mapper
	VrKeyMap m_keymap;

public:
	DlgEncoderList::Item m_encoder;
	VrOptions m_opts;

private:
	Permission m_prm;
	UINT32	 m_appVersion;   // remote Ammyy version
	CStringA m_appBuild;     // remote Ammyy build datetime

	CStringA m_computerName; // remote computer name
	CStringA m_os_description; // remote computer OS type & version

	bool m_running;				// true, if we're in the main loop of run_detached
	bool m_workThreadFinished;
	
	// Window may be scrollable - these control the scroll position
	int m_hScrollPos, m_hScrollMax, m_vScrollPos, m_vScrollMax;
	int m_clientCX, m_clientCY; // The size of the current client area

	bool m_autoScroll;
	
	// The size of a window needed to hold entire screen without scrollbars
	int m_fullwinCX, m_fullwinCY;

	// Dormant basically means minimized; updates will not be requested  while dormant.
	void SetDormant(bool newstate);
	bool m_dormant;

	// Next window in clipboard chain
	HWND m_hwndNextViewer; 
	
private:
	void DrawMessage(HDC hdc, LPCSTR text, COLORREF color);
	void SetTittle(LPCSTR suffix);
		
	void OnBtnAudioChat();
	void OnBtnDesktop();
	void OnBtnFileManager();
	void OnBtnOption1();
	void OnBtnOption2();
	void OnBtnClose();
	void OnMouseMsg(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

	void SetStatus(LPCSTR text, bool error=false);
	void OnTimer(UINT timerID);
	static BOOL CALLBACK CRDP_EnumWindowsFn(HWND hwnd, LPARAM arg);
	void CRDP_KillTimer();

public:
	class CRDP {
	public:
		CRDP();
		~CRDP();

		void CloseProcess();

		DWORD		m_pid;
		HANDLE		m_handle; // process handle
		UINT_PTR	m_timer;
	};

private:
	bool	m_size_is_set; // set by target
	CRDP	m_rdp;

	InteropViewer* m_pInterop;

	POINT			m_mouseMove;	// last point of mouse move

	CStringA	m_status;
	bool		m_statusError;
	RLMutex		m_status_lock;

	volatile LONG	m_dwUncommitedScreenUpdates;

	AudioIn		m_audioIn;
	AudioOut	m_audioOu;
	
	bool		m_desktop_unavailable;
	HANDLE		m_hWorkerThread;
	
	WINDOWPLACEMENT m_plcmnt_BeforeFullScreen;

	RLTimer		m_tmStart; // start when connected to router
	CmdSessionEnded m_cmdSessionEnded;
	VrDlgSpeedTest m_dlgSpeedTest;
	bool		m_desktop_on;
	bool		m_audiochat_on;
	
public:
	static std::vector<void*>	m_objects;		// better VrClient*, but can make more exe
	static omni_mutex			m_objectsLock;

	static int GetClientsCount()
	{
		omni_mutex_lock l(m_objectsLock);
		return m_objects.size();
	}
};

// Some handy classes for temporary GDI object selection
// These select objects when constructed and automatically release them when destructed.
class ObjectSelector {
public:
	ObjectSelector(HDC hdc, HGDIOBJ hobj) { m_hdc = hdc; m_hOldObj = SelectObject(hdc, hobj); }
	~ObjectSelector() { m_hOldObj = SelectObject(m_hdc, m_hOldObj); }
	HGDIOBJ m_hOldObj;
	HDC m_hdc;
};

class TempDC {
public:
	TempDC(HWND hwnd) { m_hdc = ::GetDC(hwnd); m_hwnd = hwnd; }
	~TempDC() { ::ReleaseDC(m_hwnd, m_hdc); }
	operator HDC() {return m_hdc;};
	HDC m_hdc;
	HWND m_hwnd;
};

#endif // _VRCLIENT_H__INCLUDED_

