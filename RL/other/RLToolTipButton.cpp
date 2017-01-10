#include "stdafx.h"
#include "RLToolTipButton.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

/*
CRLImageList::CRLImageList()
{
	m_handle = NULL;
}

CRLImageList::~CRLImageList()
{
	Close();
}

void CRLImageList::Close()
{
	if (m_handle) ::ImageList_Destroy(m_handle);
	
}

void CRLImageList::Create(int cx, int cy, UINT flags, int cInitial, int cGrow)
{
	Close();
	m_handle = ::ImageList_Create(cx, cy, flags, cInitial, cGrow);
	ASSERT(m_handle!=NULL);
}
*/


void RLToolTipButton::MakeBWBitmap(HBITMAP hBitmap)
{
	BITMAP bitmap;
	::GetObject(hBitmap, sizeof(BITMAP), &bitmap);

	ASSERT(bitmap.bmBitsPixel == 24 || bitmap.bmBitsPixel == 32);

	int bytesPerPixel = bitmap.bmBitsPixel/8;

	int countPixels = bitmap.bmHeight * bitmap.bmWidth;
	int countBytes  = countPixels * bytesPerPixel;

	byte* data = new byte[countBytes];
	ASSERT( data!=NULL );

	ASSERT(::GetBitmapBits(hBitmap, countBytes, data) == countBytes);

	byte* p = data;

	for (int i=0; i<countPixels; i++, p+=bytesPerPixel) {
		int b = p[0];
		int g = p[1];
		int r = p[2];

		byte gray = (byte)((r*30 + g*59 + b*11) / 100);

		p[0] = p[1] = p[2] = gray;
	}

	ASSERT(::SetBitmapBits(hBitmap, countBytes, data) == countBytes);
	delete[] data;
}


RLToolTipButton::RLToolTipButton()
{
	for (int i=0; i<COUNTOF(m_hBitmaps); i++) 
	{
		m_bBitmapsRelease[i] = false;
		m_hBitmaps[i] = NULL;
	}
	m_mouseOver  = false;
}

RLToolTipButton::~RLToolTipButton()
{
//	this->SubclassWindowUndo();

	for (int i=0; i<COUNTOF(m_hBitmaps); i++) 
	{
		if (m_bBitmapsRelease[i] && m_hBitmaps[i] != NULL) ::DeleteObject(m_hBitmaps[i]);
	}

}

LRESULT RLToolTipButton::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg==WM_MOUSEMOVE) 	return OnMouseMove(wParam, lParam);	
	if (msg==WM_MOUSELEAVE) return OnMouseLeave(wParam, lParam);

	return RLWndEx::WindowProc(hwnd, msg, wParam, lParam);
}

BOOL RLToolTipButton::Create(int x, int y, int cx, int cy, HWND hwndParent, UINT nID)
{
	DWORD dwStyle = WS_CHILD|WS_VISIBLE|BS_TEXT;

	// can't use for desktop or pop-up windows (use CreateEx instead)
	ASSERT(hwndParent != NULL);
	//ASSERT((dwStyle & WS_POPUP) == 0);
	
	m_hWnd = ::CreateWindowExA(0, "BUTTON", "", dwStyle, x, y, cx, cy, hwndParent, (HMENU)nID, NULL, NULL);
	if (m_hWnd == NULL) return FALSE;
	
	this->SubclassWindow();

	return TRUE;
}


void RLToolTipButton::SetToolTipTextW(LPCWSTR lpText)
{
	if (m_ToolTip.m_hWnd == NULL) {
		m_ToolTip.Create(this->m_hWnd);

		{
			RECT rectBtn; 
			::GetClientRect(m_hWnd, &rectBtn);
			m_ToolTip.AddToolW(m_hWnd, lpText, &rectBtn, 1);
		}

		m_ToolTip.Activate(TRUE);
	}
	else {
		m_ToolTip.UpdateTipTextW(lpText, m_hWnd, 1);
	}
}

void RLToolTipButton::SetButtonStyle(UINT nStyle, BOOL bRedraw)
{ 
	ASSERT(::IsWindow(m_hWnd)); 
	::SendMessage(m_hWnd, BM_SETSTYLE, nStyle, (LPARAM)bRedraw);
}

void RLToolTipButton::DrawItem( LPDRAWITEMSTRUCT lpDrawIS )
{
	bool pushed		= ((lpDrawIS->itemState & ODS_SELECTED) != 0);
	bool disabled	= ((lpDrawIS->itemState & ODS_DISABLED) != 0);
	bool hasFocus	= ((lpDrawIS->itemState & ODS_FOCUS)    != 0);

	//if (m_hBitmap != NULL) {
		int x = (pushed) ? 3 : 2;
		int i = (disabled) ? 1 : 0;
		::DrawState(lpDrawIS->hDC, NULL, NULL, (LONG)m_hBitmaps[i], 0, x, x, 0, 0, DST_BITMAP /*| (disabled ? DSS_DISABLED : DSS_NORMAL)*/);
		UINT edge = (pushed) ? BDR_SUNKENOUTER : BDR_RAISEDOUTER;
		::DrawEdge(lpDrawIS->hDC, &lpDrawIS->rcItem, edge, BF_RECT | BF_SOFT);
		return;
	/*
	}
	else {
		int index;

		if (!disabled) {
			if (pushed) {
				index = 2;
			}
			else if (m_mouseOver)
				index = 1;
			else
				index = 0;

		}
		else {
			index = 3;
		}

		::ImageList_Draw(m_imageList, index, lpDrawIS->hDC, 0, 0, ILD_TRANSPARENT);
	}
	*/

}

/*
void RLToolTipButton::InitImage1(DWORD nResID)
{	
 	HBITMAP hBitmapMain = (HBITMAP)LoadImage((HINSTANCE)0x400000, MAKEINTRESOURCE(nResID), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
 	ASSERT( hBitmapMain!=NULL );
 
 	m_imageList.Create(32, 32, ILC_COLOR24, 4, 1);
 
 	COLORREF backgroundColor = GetSysColor(COLOR_BTNFACE); // RGB(236, 233, 216)
 
 	for (int i=0; i<4; i++) {
 		DWORD resID = (i==0 || i==3) ? IDB_BTN_COMMON_UP : (i==1) ? IDB_BTN_COMMON_HOVER : IDB_BTN_COMMON_DOWN;
 		
 		HBITMAP hBitmapBW = (HBITMAP)LoadImage((HINSTANCE)0x400000, MAKEINTRESOURCE(resID), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
 		HBITMAP hBitmapColor = CCommon::ChangeImageColour(hBitmapBW, RGB(0xCC, 0xCC, 0xCC), backgroundColor);
 
 		int x = 10; if (i==2) x++;
 
 		if (i==3)
 			CCommon::MakeBWBitmap(hBitmapMain);
 		
 		CCommon::DrawBitmap(hBitmapColor, hBitmapMain, x, x);
 
 		VERIFY ( -1!= ::ImageList_Add(m_imageList, hBitmapColor, NULL));
 
 		::DeleteObject(hBitmapColor);
 		::DeleteObject(hBitmapBW);
 
 	}
 
 	::DeleteObject(hBitmapMain);
 
 	this->SetButtonStyle(BS_OWNERDRAW, FALSE);
}
*/

void RLToolTipButton::InitImage1(DWORD nResID)
{			
	m_hBitmaps[0] = (HBITMAP)::LoadImage((HINSTANCE)0x400000, MAKEINTRESOURCE(nResID), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	m_hBitmaps[1] = (HBITMAP)::LoadImage((HINSTANCE)0x400000, MAKEINTRESOURCE(nResID), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

	ASSERT( m_hBitmaps[0]!=NULL );
	ASSERT( m_hBitmaps[1]!=NULL );

	MakeBWBitmap(m_hBitmaps[1]);

	//m_imageList.Create(24, 24, ILC_COLOR24, 4, 1);
	//VERIFY ( -1!= ::ImageList_Add(m_imageList, m_hBitmaps[0], NULL));
	//VERIFY ( -1!= ::ImageList_Add(m_imageList, m_hBitmaps[1], NULL));
 
	//::DeleteObject(hBitmapColor);
	//::DeleteObject(hBitmapBW);

	m_bBitmapsRelease[0] = true;
	m_bBitmapsRelease[1] = true;
 
 	this->SetButtonStyle(BS_OWNERDRAW, FALSE);
}

void RLToolTipButton::InitImage2(HBITMAP hBitmap0, HBITMAP hBitmap1, bool bBitmap0, bool bBitmap1)
{			
	m_hBitmaps[0] = hBitmap0;
	m_hBitmaps[1] = hBitmap1;

	m_bBitmapsRelease[0] = bBitmap0;
	m_bBitmapsRelease[1] = bBitmap1;

	//::SendMessage(m_hWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap0);
 
 	this->SetButtonStyle(BS_OWNERDRAW, FALSE);
}



LRESULT RLToolTipButton::OnMouseMove(WPARAM WParam, LPARAM LParam)
{
	if (!m_mouseOver) {
		m_mouseOver = true;

		// need for OnMouseLeave()
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.hwndTrack = this->m_hWnd;
		tme.dwFlags = TME_LEAVE;
		_TrackMouseEvent(&tme);

		Invalidate(FALSE);
		UpdateWindow();
	}
	return TRUE; //Default();
}


LRESULT RLToolTipButton::OnMouseLeave(WPARAM WParam, LPARAM LParam)
{
	m_mouseOver = false;
	Invalidate(FALSE);
	UpdateWindow();
	return TRUE; //Default();
}

