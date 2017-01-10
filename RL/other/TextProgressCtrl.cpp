#include "stdafx.h"
#include "TextProgressCtrl.h"
#include "../FontWTL.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

CTextProgressCtrl::CTextProgressCtrl()
{
    m_nPos      = 0;
    m_nMax      = 100;
    m_nMin      = 0;
    m_crBarClr  = CLR_DEFAULT;
    m_crBgClr   = GetSysColor(COLOR_3DFACE);
    m_crTextClr = CLR_DEFAULT;
    m_nBarWidth = 0;
	m_vertical = false;
	m_fontParent = NULL;
	m_dwTextStyle = DT_LEFT | DT_VCENTER | DT_SINGLELINE; // | DT_CENTER;
	m_useParentFont = true;
}

CTextProgressCtrl::~CTextProgressCtrl()
{
}

void CTextProgressCtrl::OnPaint() 
{
	PAINTSTRUCT ps;
	HDC hdc = ::BeginPaint(m_hWnd, &ps);


    RECT rect_1, rect_2, ClientRect;
    GetClientRect(&ClientRect);
    
	if (m_useParentFont) {
		if (m_fontParent==NULL)
		{		
			RLWnd parent(::GetParent(m_hWnd));
			m_fontParent = parent.GetFont();
		}

		::SelectObject(hdc, m_fontParent);
	}

    rect_1 = rect_2 = ClientRect;

	if (m_vertical)
	{
		rect_1.top = rect_1.bottom - m_nBarWidth;
		rect_2.bottom = rect_1.top;
	}
    else
	{
        rect_1.right = rect_1.left + m_nBarWidth;
        rect_2.left = rect_1.right;
    }

    COLORREF crBarColour = (m_crBarClr == CLR_DEFAULT)? ::GetSysColor(COLOR_HIGHLIGHT) : m_crBarClr;
    COLORREF crBgColour  = (m_crBgClr  == CLR_DEFAULT)? ::GetSysColor(COLOR_WINDOW)    : m_crBgClr;

	// Draw Bar
	RLWnd::FillSolidRect(hdc, &rect_1, crBarColour);
    RLWnd::FillSolidRect(hdc, &rect_2, crBgColour);

    // Draw Text
    if (!m_strText.IsEmpty())
    {
		::SetBkMode(hdc, TRANSPARENT);

        // If we are drawing vertical, then create a new verticla font
        // based on the current font (only works with TrueType fonts)

		/*
        CFontWTL font;
		HFONT hOldFont = NULL;
		
        if (m_vertical)
        {
			RLWnd _this(m_hWnd);
			
			LOGFONT lf;
			::GetObject(_this.GetFont(), sizeof(LOGFONT), &lf);

            lf.lfEscapement = lf.lfOrientation = 900;
            font.CreateFontIndirect(&lf);
			
			hOldFont = (HFONT)::SelectObject(hdc, (HFONT)font);

            m_dwTextStyle = DT_VCENTER|DT_CENTER|DT_SINGLELINE;
        }
		*/

		
		HRGN rgn = ::CreateRectRgn(rect_1.left, rect_1.top, rect_1.right, rect_1.bottom);
		::SelectClipRgn(hdc, rgn);
		::SetTextColor(hdc, (m_crTextClr==CLR_DEFAULT) ? crBgColour : m_crTextClr);
		::DrawText(hdc, m_strText, m_strText.GetLength(), &ClientRect, m_dwTextStyle);
		::DeleteObject(rgn);
       
		     rgn = ::CreateRectRgn(rect_2.left, rect_2.top, rect_2.right, rect_2.bottom);
		::SelectClipRgn(hdc, rgn);
        ::SetTextColor(hdc, (m_crTextClr==CLR_DEFAULT) ? crBarColour : m_crTextClr);
        ::DrawText(hdc, m_strText, m_strText.GetLength(), &ClientRect, m_dwTextStyle);
		::DeleteObject(rgn);

		/*
        if (hOldFont!=NULL)
        {
			::SelectObject(hdc, hOldFont);
            font.DeleteObject();
        }
		*/
    }

	::EndPaint(m_hWnd, &ps);
}

/////////////////////////////////////////////////////////////////////////////
// CTextProgressCtrl operations



LRESULT CTextProgressCtrl::OnSetBarColor(WPARAM, LPARAM Colour)
{
    return (LRESULT)SetBarColour((COLORREF)Colour);
}

COLORREF CTextProgressCtrl::SetBarColour(COLORREF crBarClr /*= CLR_DEFAULT*/)
{
    if (::IsWindow(m_hWnd))
        Invalidate();

    COLORREF crOldBarClr = m_crBarClr;
    m_crBarClr = crBarClr;
    return crOldBarClr;
}

COLORREF CTextProgressCtrl::GetBarColour() const
{ 
    return m_crBarClr;
}

LRESULT CTextProgressCtrl::OnSetBkColor(WPARAM, LPARAM Colour)
{
    return (LRESULT)SetBgColour((COLORREF)Colour);
}

COLORREF CTextProgressCtrl::SetBgColour(COLORREF crBgClr /*= CLR_DEFAULT*/)
{
    if (::IsWindow(m_hWnd))
        Invalidate();

    COLORREF crOldBgClr = m_crBgClr;
    m_crBgClr = crBgClr;
    return crOldBgClr;
}

COLORREF CTextProgressCtrl::GetBgColour() const
{ 
    return m_crBgClr;
}


LRESULT CTextProgressCtrl::OnSetText(UINT, LPCTSTR szText)
{
	SetWindowText(szText);
    return TRUE;
}

void CTextProgressCtrl::SetWindowText(LPCSTR text)
{
	CStringA newValue;

	if (text!=NULL) {
		newValue = CStringA(" ") + text;	// add space for padding
	}

    if (m_strText != newValue) {
        m_strText  = newValue;
        Invalidate();
    }
}

void CTextProgressCtrl::SetRange32(int nMin, int nMax)
{
	m_nMin = nMin;
	m_nMax = nMax;
}

void CTextProgressCtrl::SetPos(int pos)
{
	if (m_nPos==pos) return;

    int nOldPos = m_nPos;
    m_nPos = pos;

    RECT rect;
    GetClientRect(&rect);

    double fraction = (double)(m_nPos - m_nMin) / ((double)(m_nMax - m_nMin));
	int size = (m_vertical) ? (rect.bottom - rect.top) : (rect.right - rect.left);
    int nBarWidth = (int) (fraction * size);

    if (nBarWidth != m_nBarWidth)
    {
        m_nBarWidth = nBarWidth;
        RedrawWindow();
    }	
}


LRESULT CTextProgressCtrl::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg==WM_PAINT) {
		OnPaint();
		return 0;
	}
	if (msg==PBM_SETBARCOLOR) {
		return OnSetBarColor(wParam, lParam);
	}
	if (msg==PBM_SETBKCOLOR)  {
		return OnSetBkColor(wParam, lParam);
	}
	if (msg==WM_SETTEXT) {
		return OnSetText(0, (LPCTSTR)lParam);
	}
	if (msg==WM_ERASEBKGND) {
		return FALSE; // don't need erase
	}	

	return RLWndEx::WindowProc(m_hWnd, msg, wParam, lParam);
}



