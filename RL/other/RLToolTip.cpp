#include "stdafx.h"
#include "RLToolTip.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

RLToolTip::RLToolTip()
{
	m_hWnd = NULL;

}

RLToolTip::~RLToolTip()
{
	if (m_hWnd!=NULL) ::DestroyWindow(m_hWnd);
}


//void RLToolTip::RelayEvent(LPMSG lpMsg)
//{ 
//	::SendMessage(m_hWnd, TTM_RELAYEVENT, 0, (LPARAM)lpMsg);
//}

//int RLToolTip::GetToolCount() const
//{ 
//	ASSERT(::IsWindow(m_hWnd));
//	return (int) ::SendMessage(m_hWnd, TTM_GETTOOLCOUNT, 0, 0L); 
//}


void RLToolTip::Activate(BOOL bActivate)
{ 
	::SendMessage(m_hWnd, TTM_ACTIVATE, bActivate, 0L);
}


void RLToolTip::FillInToolInfo(TOOLINFOW& ti, HWND hwnd, UINT nIDTool) const
{
	memset(&ti, 0, sizeof(ti));
	ti.cbSize = sizeof(ti);
	if (nIDTool == 0)
	{
		ti.hwnd = ::GetParent(hwnd);
		ti.uFlags = TTF_IDISHWND;
		ti.uId = (UINT)hwnd;
	}
	else
	{
		ti.hwnd = hwnd;
		ti.uFlags = 0;
		ti.uId = nIDTool;
	}

	ti.uFlags |= TTF_SUBCLASS;
}


void RLToolTip::UpdateTipTextW(LPCWSTR lpszText, HWND hwnd, UINT nIDTool)
{
	ASSERT(::IsWindow(m_hWnd));
	ASSERT(hwnd != NULL);

	TOOLINFOW ti;
	FillInToolInfo(ti, hwnd, nIDTool);
	ti.lpszText = (LPWSTR)lpszText;
	::SendMessage(m_hWnd, TTM_UPDATETIPTEXTW, 0, (LPARAM)&ti);
}


BOOL RLToolTip::AddToolW(HWND hwnd, LPCWSTR lpszText, LPCRECT lpRectTool, UINT nIDTool)
{
	ASSERT(::IsWindow(m_hWnd));
	ASSERT(hwnd != NULL);
	ASSERT(lpszText != NULL);
	// the toolrect and toolid must both be zero or both valid
	ASSERT((lpRectTool != NULL && nIDTool != 0) ||
		   (lpRectTool == NULL) && (nIDTool == 0));

	TOOLINFOW ti;
	FillInToolInfo(ti, hwnd, nIDTool);
	if (lpRectTool != NULL)
		memcpy(&ti.rect, lpRectTool, sizeof(RECT));
	ti.lpszText = (LPWSTR)lpszText;
	return (BOOL) ::SendMessage(m_hWnd, TTM_ADDTOOLW, 0, (LPARAM)&ti);
}


BOOL RLToolTip::Create(HWND hwndParent)
{
	DWORD dwStyle = WS_POPUP;

	m_hWnd = ::CreateWindowEx(0, TOOLTIPS_CLASS, NULL, dwStyle, 
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hwndParent, NULL, NULL, NULL);

	return (m_hWnd == NULL) ? FALSE : TRUE;
}

