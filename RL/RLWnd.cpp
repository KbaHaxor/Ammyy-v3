#include "stdafx.h"
#include "RLWnd.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


RLWnd::RLWnd(HWND hwnd):
m_hWnd(hwnd)
{
}

RLWnd::~RLWnd()
{
}

RLWnd::operator HWND() const
{
	return m_hWnd;
}

void RLWnd::AttachDlgItem(HWND hDlg, int nIDDlgItem)
{
	m_hWnd = ::GetDlgItem(hDlg, nIDDlgItem);
}

void RLWnd::CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle,
    int x, int y, int cx, int cy, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	ASSERT(m_hWnd==NULL);
	m_hWnd = ::CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, cx, cy, hWndParent, hMenu, hInstance, lpParam);
	ASSERT(m_hWnd!=NULL);
}

LRESULT RLWnd::SendMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	return ::SendMessage(m_hWnd, Msg, wParam, lParam);
}

void RLWnd::SetFont(HFONT hFont, BOOL bRedraw)
{
	::SendMessage(m_hWnd, WM_SETFONT, (WPARAM)hFont, (LPARAM)bRedraw);
}

HFONT RLWnd::GetFont()
{
	return (HFONT)::SendMessage(m_hWnd, WM_GETFONT, 0, 0);
}

void RLWnd::SetTextA(LPCSTR text)
{
	::SendMessageA(m_hWnd, WM_SETTEXT, 0, (LPARAM)text);
}

void RLWnd::SetTextW(LPCWSTR text)
{
	::SendMessageW(m_hWnd, WM_SETTEXT, 0, (LPARAM)text);
}

SIZE RLWnd::GetClientSize() const
{
	SIZE size;
	RECT rect;
	::GetClientRect(m_hWnd, &rect);
	size.cx = rect.right  - rect.left;
	size.cy = rect.bottom - rect.top;
	return size;
}

SIZE RLWnd::GetWindowSize() const
{
	SIZE size;
	RECT rect;
	::GetWindowRect(m_hWnd, &rect);
	size.cx = rect.right  - rect.left;
	size.cy = rect.bottom - rect.top;
	return size;
}

void RLWnd::GetClientRect(LPRECT lpRect) const
{
	::GetClientRect(m_hWnd, lpRect);
}

void RLWnd::Invalidate(BOOL bErase)
{
	::InvalidateRect(m_hWnd, NULL, bErase);
}

void RLWnd::UpdateWindow()
{ 
	::UpdateWindow(m_hWnd); 
}


BOOL RLWnd::RedrawWindow(CONST RECT* lpRectUpdate, HRGN rgnUpdate, UINT flags)
{
	return ::RedrawWindow(m_hWnd, lpRectUpdate, rgnUpdate, flags);
}

HICON RLWnd::SetIcon(HICON hIcon, BOOL bBigIcon)
{
	return (HICON)::SendMessage(m_hWnd, WM_SETICON, bBigIcon, (LPARAM)hIcon); 
}

UINT RLWnd::SetTimer(UINT nIDEvent, UINT nElapse, void (CALLBACK* lpfnTimer)(HWND, UINT, UINT, DWORD))
{ 
	return ::SetTimer(m_hWnd, nIDEvent, nElapse, (TIMERPROC)lpfnTimer); 
}

void RLWnd::SetFocus()
{
	::SetFocus(m_hWnd);
}

void RLWnd::SetForegroundWindow()
{
	::SetForegroundWindow(m_hWnd);
}

HMENU RLWnd::GetSystemMenu(BOOL bRevert) const
{ 
	return ::GetSystemMenu(m_hWnd, bRevert);
}

CStringA RLWnd::GetTextA()
{
	CStringA text;

	UINT len = ::SendMessage(m_hWnd, WM_GETTEXTLENGTH, 0, 0);

	if (len>0) {
		len++;
		LPSTR buffer = text.GetBuffer(len);
		len = ::SendMessageA(m_hWnd, WM_GETTEXT, len, (LPARAM)buffer);
		text.ReleaseBuffer(len);
	}

	return text;
}

#ifdef __CSTRING_W_H__INCLUDED__
CStringW RLWnd::GetTextW()
{
	CStringW text;

	UINT len = ::SendMessage(m_hWnd, WM_GETTEXTLENGTH, 0, 0);

	if (len>0) {
		len++;
		LPWSTR buffer = text.GetBuffer(len);
		len = ::SendMessageW(m_hWnd, WM_GETTEXT, len, (LPARAM)buffer);
		text.ReleaseBuffer(len);
	}

	return text;
}
#endif


void RLWnd::FillSolidRect(HDC hdc, LPCRECT lpRect, COLORREF clr)
{
	::SetBkColor(hdc, clr);
	::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, lpRect, NULL, 0, NULL);
}

bool RLWnd::IsInsideWindow(POINT p)
{
	RECT r; // screen coordinates
	::GetWindowRect(m_hWnd, &r);
	return (p.x>=r.left && p.x<=r.right && p.y>=r.top && p.y<=r.bottom);
}



//__________________________________________________________________________________________________________________

RLWndEx::RLWndEx(HWND hwnd)
:RLWnd(hwnd)
{
	m_lpPrevWndFunc = NULL;
}


void RLWndEx::SubclassWindow(bool unicode)
{
	if (m_lpPrevWndFunc==NULL) {
		if (unicode) {
			::SetWindowLongW(m_hWnd, GWL_USERDATA, (LONG)this);
			m_lpPrevWndFunc = (WNDPROC)::SetWindowLongW(m_hWnd, GWL_WNDPROC,  (LONG)RLWndEx::WindowProcStatic);
		}
		else {
			::SetWindowLongA(m_hWnd, GWL_USERDATA, (LONG)this);
			m_lpPrevWndFunc = (WNDPROC)::SetWindowLongA(m_hWnd, GWL_WNDPROC,  (LONG)RLWndEx::WindowProcStatic);
		}
		m_unicode = (::IsWindowUnicode(m_hWnd) != FALSE);
	}
}

LRESULT RLWndEx::WindowProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	RLWndEx* _this = (RLWndEx*)::GetWindowLong(hwnd, GWL_USERDATA);

	if (_this)
		return _this->WindowProc(hwnd, msg, wParam, lParam);
	else {
		// here we'll be after CreateWindow but before SetWindowLong(GWL_USERDATA)
		//

		//ASSERT(msg==WM_GETMINMAXINFO || msg==WM_NCCREATE || msg==WM_NCCALCSIZE || msg==WM_CREATE || msg==WM_ACTIVATEAPP);
		bool unicode = (::IsWindowUnicode(hwnd) != FALSE);
		if (unicode)
			return ::DefWindowProcW(hwnd, msg, wParam, lParam);
		else
			return ::DefWindowProcA(hwnd, msg, wParam, lParam);
	}
	
		
}

LRESULT RLWndEx::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return (m_unicode) ? ::CallWindowProcW(m_lpPrevWndFunc, m_hWnd, msg, wParam, lParam) : 
						 ::CallWindowProcA(m_lpPrevWndFunc, m_hWnd, msg, wParam, lParam);
}


//__________________________________________________________________________________________________________________



void RLWndList::SetExtStyle(DWORD dwExStyle)
{
	::SendMessage(m_hWnd, LVM_SETEXTENDEDLISTVIEWSTYLE, dwExStyle, dwExStyle);
}

int  RLWndList::GetSelectedCount() const
{
	return (int)::SendMessage(m_hWnd, LVM_GETSELECTEDCOUNT, 0, 0);
}

int  RLWndList::GetSelectionMark() const
{
	return (int)::SendMessage(m_hWnd, LVM_GETSELECTIONMARK, 0, 0);
}

void RLWndList::DeleteItem(int index)
{
	::SendMessage(m_hWnd, LVM_DELETEITEM, (WPARAM)index, 0);
}

void RLWndList::DeleteAllItems()
{
	::SendMessage(m_hWnd, LVM_DELETEALLITEMS, 0, 0);
}

int RLWndList::GetItemCount()
{
	return ::SendMessage(m_hWnd, LVM_GETITEMCOUNT, 0, 0);
}

void RLWndList::SetCheckState(int index, bool fCheck)
{
	LVITEM lvItem;
	lvItem.mask = LVIF_STATE;
	lvItem.pszText = NULL;
	lvItem.state = (fCheck?2:1) << 12;
	lvItem.stateMask = LVIS_STATEIMAGEMASK;
	::SendMessage(m_hWnd, LVM_SETITEMSTATE, index, (LPARAM)&lvItem);
}


void RLWndList::GetItemTextA(int index, int subItem, LPSTR text, UINT textMax) const
{
	LVITEM lvItem;
	lvItem.iSubItem = subItem;
	lvItem.cchTextMax = textMax;
	lvItem.pszText = text;		
	::SendMessage(m_hWnd, LVM_GETITEMTEXTA, index, (LPARAM)&lvItem);
}

void RLWndList::GetItemTextW(int index, int subItem, LPWSTR text, UINT textMax) const
{
	LVITEMW lvItem;
	lvItem.iSubItem = subItem;
	lvItem.cchTextMax = textMax;
	lvItem.pszText = text;
 	::SendMessageW(m_hWnd, LVM_GETITEMTEXTW, index, (LPARAM)&lvItem);
}

void RLWndList::SetItemTextA(int index, int subItem, LPCSTR text)
{
	LVITEMA lvItem;
	lvItem.iSubItem = subItem;
	lvItem.pszText = (LPSTR)text;
	::SendMessageA(m_hWnd, LVM_SETITEMTEXTA, index, (LPARAM) &lvItem);
}

void RLWndList::SetItemTextW(int index, int subItem, LPCWSTR text)
{
	LVITEMW lvItem;
	lvItem.iSubItem = subItem;
	lvItem.pszText = (LPWSTR)text;	
	::SendMessageW(m_hWnd, LVM_SETITEMTEXTW, index, (LPARAM) &lvItem);
}


void RLWndList::EnsureVisible(int index, BOOL fPartialOK)
{
	::SendMessage(m_hWnd, LVM_ENSUREVISIBLE, index, (LPARAM)fPartialOK);
}

void RLWndList::SetSelectionMark(int index)
{
	::SendMessage(m_hWnd, LVM_SETSELECTIONMARK, 0, index);

	LVITEM lvItem;
	lvItem.iItem = index;
	lvItem.iSubItem = 0;
	lvItem.stateMask = lvItem.state = LVIS_SELECTED | LVIS_FOCUSED;
	lvItem.pszText = NULL;
		
	::SendMessage(m_hWnd, LVM_SETITEMSTATE, index, (LPARAM) &lvItem);
}


void RLWndEditBox::SetReadOnly(BOOL bReadOnly)
{
	::SendMessage(m_hWnd, EM_SETREADONLY, bReadOnly, 0);
}


void RLWndComboBox::InsertString(int nIndex, LPCTSTR lpszString)
{
	::SendMessage(m_hWnd, CB_INSERTSTRING, (WPARAM)nIndex, (LPARAM)lpszString);
}

void RLWndComboBox::InsertStringW(int nIndex, LPCWSTR lpszString)
{
	::SendMessageW(m_hWnd, CB_INSERTSTRING, (WPARAM)nIndex, (LPARAM)lpszString);
}

int  RLWndComboBox::SetCurSel(int nSelect)
{
	return (int)::SendMessage(m_hWnd, CB_SETCURSEL, nSelect, 0);
}

int  RLWndComboBox::GetCurSel() const
{
	return (int)::SendMessage(m_hWnd, CB_GETCURSEL, 0, 0);
}



int  RLWndButton::GetCheck() const
{
	return (int)::SendMessage(m_hWnd, BM_GETCHECK, 0, 0);
}

bool RLWndButton::GetCheckBool() const
{
	return ((int)::SendMessage(m_hWnd, BM_GETCHECK, 0, 0)==BST_CHECKED);
}

void RLWndButton::SetCheck(int nCheck)
{
	::SendMessage(m_hWnd, BM_SETCHECK, (WPARAM)nCheck, 0);
}

void RLWndButton::SetCheckBool(bool checked)
{
	this->SetCheck( checked ? BST_CHECKED : BST_UNCHECKED);
}

BOOL RLWndTab::SetItem(int nItem, TCITEM* pTabCtrlItem)
{
	return (BOOL)::SendMessage(m_hWnd, TCM_SETITEM, nItem, (LPARAM)pTabCtrlItem); 
}

BOOL RLWndTab::SetItemW(int nItem, TCITEMW* pTabCtrlItem)
{
	return (BOOL)::SendMessage(m_hWnd, TCM_SETITEMW, nItem, (LPARAM)pTabCtrlItem); 
}

int RLWndTab::GetCurSel() 
{
	return (int)::SendMessage(m_hWnd, TCM_GETCURSEL, 0, 0L);
}

int RLWndTab::SetCurSel(int nItem)
{
	return (int)::SendMessage(m_hWnd, TCM_SETCURSEL, nItem, 0L); 
}


BOOL RLWndTab::InsertItem(int nItem, LPCTSTR lpszItem)
{
	TCITEM item;
	item.mask = TCIF_TEXT;
	item.iImage = 0;
	item.lParam = 0;
	item.pszText = (LPTSTR) lpszItem;

	return (BOOL) ::SendMessage(m_hWnd, TCM_INSERTITEM, nItem, (LPARAM) &item);
}






//___________________________________________________________________________________________________________

int RLDlgBase::DoModal(HWND hWndParent)
{
	return ::DialogBoxParam((HINSTANCE)0x400000, m_lpTemplateName, hWndParent, (DLGPROC)WindowProcStatic, (LPARAM)this);
}

void RLDlgBase::DoModeless(HWND hWndParent)
{
	::CreateDialogParamW((HINSTANCE)0x400000, (WCHAR*)m_lpTemplateName, hWndParent, (DLGPROC)WindowProcStatic, (LPARAM)this);
}


int RLDlgBase::DoModalIndirect(HWND hWndParent, LPCDLGTEMPLATEA hDialogTemplate, HINSTANCE hInstance)
{
	return ::DialogBoxIndirectParam((HINSTANCE)0x400000, hDialogTemplate, hWndParent, (DLGPROC)WindowProcStatic, (LPARAM)this);
}

BOOL RLDlgBase::OnInitDialog()
{
	return TRUE;
}

BOOL RLDlgBase::OnEndDialog(BOOL ok)
{
	return TRUE;
}

INT_PTR RLDlgBase::WindowProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return 0;
}


INT_PTR RLDlgBase::WindowProcStatic(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg==WM_INITDIALOG) {
        ::SetWindowLong(hwnd, GWL_USERDATA, lParam);
        RLDlgBase* _this = (RLDlgBase*)lParam;
		_this->m_hWnd = hwnd;
		return _this->OnInitDialog();
	}
	
	RLDlgBase* _this = (RLDlgBase*) ::GetWindowLong(hwnd, GWL_USERDATA);

	if (_this==NULL) return 0;

	if (uMsg==WM_COMMAND) 
	{
		switch(LOWORD(wParam)) 
		{				
			case IDOK:
			{
				if (_this->OnEndDialog(TRUE)) {
					::EndDialog(hwnd, IDOK);
				}
				return TRUE;
			};
	
			case IDCANCEL:
			{
				if (_this->OnEndDialog(FALSE)) {
					::EndDialog(hwnd, IDCANCEL);
				}
				return TRUE;
			};			

		}
	}
	else if (uMsg==WM_DESTROY) 
	{
		// Window is being destroyed!  (Should never happen)
		::EndDialog(hwnd, IDOK);
		return TRUE;
	}
	else if (uMsg==WM_CTLCOLORSTATIC) {
		return (INT_PTR)_this->OnCtlColor((HDC)wParam, (HWND)lParam);
	}

	return _this->WindowProc(hwnd, uMsg, wParam, lParam);
}
