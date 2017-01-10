#include "stdafx.h"
#include "vrMain.h"
#include "vrClient.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


void VrClient::DoFullScreenMode(bool suppressPrompt)
{
	LONG style  = ::GetWindowLong(m_hwnd1, GWL_STYLE);
	LONG styleChanging = WS_DLGFRAME | WS_THICKFRAME | WS_BORDER; //maximp: added WS_BORDER

	if (m_opts.m_FullScreen) {
		if (!suppressPrompt && settings.m_warnFullScreen) {
			::MessageBoxA(m_hwnd1,
				"To exit from full-screen mode, press Ctrl+Alt+Shift+F.\r\n"
				"Alternatively, press Ctrl+Esc and then right-click\r\n"
				"on the Ammyy Admin taskbar icon to see the menu.",
				"Ammyy Admin full-screen mode",
				MB_OK | MB_ICONINFORMATION | MB_TOPMOST | MB_SETFOREGROUND);
		}		

		m_plcmnt_BeforeFullScreen.length = sizeof(m_plcmnt_BeforeFullScreen);
		::GetWindowPlacement(m_hwnd1, &m_plcmnt_BeforeFullScreen);
				
		::EnableMenuItem(m_hMenu, ID_TOOLBAR,    MF_BYCOMMAND|MF_GRAYED); // hide toolbar
		::CheckMenuItem (m_hMenu, ID_FULLSCREEN, MF_BYCOMMAND|MF_CHECKED);
		int cx = ::GetSystemMetrics(SM_CXSCREEN);
		int cy = ::GetSystemMetrics(SM_CYSCREEN);
		//::SetWindowPos(m_hwnd1, HWND_TOPMOST, 0, 0, cx, cy, SWP_FRAMECHANGED | SWP_SHOWWINDOW);

		::SetWindowLong(m_hwnd1, GWL_STYLE, style & ~styleChanging);
		
		WINDOWPLACEMENT plcmnt = {0};
		plcmnt.length = sizeof(plcmnt);
		plcmnt.showCmd = SW_SHOWNORMAL;
		plcmnt.rcNormalPosition.right  = cx;
		plcmnt.rcNormalPosition.bottom = cy;
		::SetWindowPlacement(m_hwnd1, &plcmnt);
	} 
	else {
		//ShowWindow(m_hToolbar, SW_SHOW);
		::EnableMenuItem(m_hMenu, ID_TOOLBAR,    MF_BYCOMMAND|MF_ENABLED); // show toolbar
		::CheckMenuItem (m_hMenu, ID_FULLSCREEN, MF_BYCOMMAND|MF_UNCHECKED);
		//::SetWindowPos(m_hwnd1, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		::SetWindowLong(m_hwnd1, GWL_STYLE, style | styleChanging);

		if (m_plcmnt_BeforeFullScreen.length>0)
			::SetWindowPlacement(m_hwnd1, &m_plcmnt_BeforeFullScreen);
	}
}

bool VrClient::AutoScroll(int x, int y)
{
	if (!m_autoScroll) return false; // no scroll bars
	if ((!m_opts.m_FullScreen) && (!m_opts.m_autoScrollWndMode)) return false; // deny by settings

	const int BORDER = 40; // (InFullScreenMode()) ? 4 : 50;

	int dx=0, dy=0;

	if      (x < BORDER)             dx = -1;
	else if (x >= m_clientCX-BORDER) dx = +1;

	if      (y < BORDER)             dy = -1;
	else if (y >= m_clientCY-BORDER) dy = +1;

	if (dx==0 && dy==0) return false; // no need to scroll

	const int k = 10*m_opts.m_scale_num/m_opts.m_scale_den; // 	const int AMOUNT = 10;
		
	if (ScrollScreen(dx*k,dy*k))
	{
		::Sleep(8);

		POINT p;
		::GetCursorPos(&p);

		RECT rect;
		::GetWindowRect(m_hwnd2, &rect); // in full-screen mode window can be not on primary display

		x += rect.left;
		y += rect.top;		

		if (p.x == x && p.y == y) // If we haven't physically moved the cursor, artificially
			::SetCursorPos(x, y); // generate another mouse event so we keep scrolling.

		return true;
	}
	return false;
}
