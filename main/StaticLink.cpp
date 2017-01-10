#include "stdafx.h"
#include "StaticLink.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


CStaticLink::CStaticLink()
{
	m_color = RGB(0,0,255); // blue
	m_cursor = NULL;
}

CStaticLink::~CStaticLink()
{
	if (m_cursor!=NULL) ::DestroyIcon(m_cursor);
}


HBRUSH CStaticLink::OnCtlColor(HDC hdc, HWND hwnd)
{
	HFONT hFont = this->GetFont();

    // set up font and colors 
    if (!(HFONT)m_font) { 
		// first time init: create font 
        LOGFONT lf;
		HFONT hFont = this->GetFont();
		::GetObject(hFont, sizeof(lf), &lf);

		lf.lfHeight = (LONG)(lf.lfHeight*1.3);
        
		lf.lfUnderline = TRUE; 
		m_font.CreateFontIndirect(&lf); 
		
		m_cursor = ::LoadCursor(0, IDC_HAND);
	}

	::SelectObject(hdc, (HFONT)m_font);
	::SetTextColor(hdc, m_color);
	::SetBkMode(hdc, TRANSPARENT);

	// return hollow brush to preserve parent background color
	return (HBRUSH)::GetStockObject(HOLLOW_BRUSH);
}


void CStaticLink::OnLButton()
{
	CStringA url = this->GetTextA();
	::ShellExecuteA(0, "open", (LPCSTR)url, 0, 0, SW_SHOWNORMAL); 
}


LRESULT CStaticLink::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg==WM_LBUTTONDOWN) {
		OnLButton();
		return TRUE;
	}

	if (msg==WM_SETCURSOR) {		
		if (m_cursor!=NULL) { 
			::SetCursor(m_cursor); 
			return TRUE; 
		}
	}

	return RLWndEx::WindowProc(hwnd, msg, wParam, lParam);
}


