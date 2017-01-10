#if !defined(_RL_WND_H____INCLUDED_)
#define _RL_WND_H____INCLUDED_


class RLWnd  
{
public:
	RLWnd(HWND hwnd=0);
	virtual ~RLWnd();

	void AttachDlgItem(HWND hDlg, int nIDDlgItem);
	void CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle,
							int x, int y, int cx, int cy, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);

	LRESULT SendMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
	HFONT GetFont();
	void  SetFont(HFONT hFont, BOOL bRedraw = TRUE);
	void  SetTextA(LPCSTR text);
	void  SetTextW(LPCWSTR text);
	CStringA  GetTextA();
#ifdef __CSTRING_W_H__INCLUDED__
	CStringW GetTextW();
#endif
	
	void GetClientRect(LPRECT lpRect) const;
	SIZE GetClientSize() const;
	SIZE GetWindowSize() const;

	void Invalidate(BOOL bErase = TRUE);
	void UpdateWindow();

	BOOL RedrawWindow(CONST RECT* lpRectUpdate=NULL, HRGN rgnUpdate=NULL, UINT flags = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);

	HICON SetIcon(HICON hIcon, BOOL bBigIcon);

	void SetFocus();
	void SetForegroundWindow();

	UINT SetTimer(UINT nIDEvent, UINT nElapse, void (CALLBACK* lpfnTimer)(HWND, UINT, UINT, DWORD));

	HMENU GetSystemMenu(BOOL bRevert) const;
	bool RLWnd::IsInsideWindow(POINT p);

	operator HWND() const;

	static void FillSolidRect(HDC hdc, LPCRECT lpRect, COLORREF clr);

	static inline void SetWindowPos(HWND hwnd, int x, int y, int cx, int cy) 
	{
		::SetWindowPos(hwnd, 0, x, y, cx, cy, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER);
	}

	inline void SetWindowPos1(int x, int y, int cx, int cy) 
	{
		::SetWindowPos(m_hWnd, 0, x, y, cx, cy, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER);
	}

	inline void SetWindowSize(int cx, int cy) 
	{
		::SetWindowPos(m_hWnd, 0, 0, 0, cx, cy, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE);
	}

	inline void SetWindowPos2(int x, int y) 
	{
		::SetWindowPos(m_hWnd, 0, x, y, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOSIZE);
	}

	inline void ShowWindow1(bool b)
	{
		::ShowWindow(m_hWnd, (b) ? SW_SHOW : SW_HIDE);
	}

	static inline void GetWorkArea(RECT* workrect)
	{
		::SystemParametersInfo(SPI_GETWORKAREA, 0, workrect, 0);
	}

	// return true if r inside a
	static bool IsRectInside(RECT& a, RECT& r)
	{
		return (r.left>=a.left && r.right<=a.right && r.top>=a.top && r.bottom<=a.bottom);
	}



protected:
	HWND m_hWnd;
	bool m_unicode;
};

class RLWndEx : public RLWnd
{
public:
	RLWndEx(HWND hwnd=0);
	void SubclassWindow(bool unicode=false);

protected:
	virtual LRESULT WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	static  LRESULT CALLBACK WindowProcStatic(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	WNDPROC m_lpPrevWndFunc;
};


class RLWndList: public RLWnd 
{
public:
	void SetExtStyle(DWORD dwExStyle);
	
	int  GetSelectedCount() const;
	int  GetSelectionMark() const;
	void SetSelectionMark(int index);
	
	void DeleteItem(int index);
	void DeleteAllItems();
	int  GetItemCount();
	void GetItemTextA(int index, int subItem, LPSTR text, UINT textMax)  const;
	void GetItemTextW(int index, int subItem, LPWSTR text, UINT textMax) const;
	void SetItemTextA(int index, int subItem, LPCSTR text);
	void SetItemTextW(int index, int subItem, LPCWSTR text);
	void EnsureVisible(int index, BOOL fPartialOK=FALSE);
	void SetCheckState(int index, bool fCheck);	
};

class RLWndListBox: public RLWnd 
{
public:
	void AddString(LPCSTR str) { this->SendMessage(LB_ADDSTRING, 0, (LPARAM)str); }
	void DeleteAllItems() { this->SendMessage(LB_RESETCONTENT, 0, 0); }
	void SetSelectionMark(int index)
	{ 
		this->SendMessage(LB_SETCURSEL, index, 0);  // LB_SETSEL
	}

	int  GetSelectionMark()
	{
		return this->SendMessage(LB_GETCURSEL, 0, 0);
	}
};

class RLWndEditBox: public RLWnd 
{
public:
	void SetReadOnly(BOOL bReadOnly=TRUE);
};


class RLWndComboBox : public RLWnd
{
public:
	RLWndComboBox(HWND hwnd=0):RLWnd(hwnd) {}
	void DeleteAllItems() { this->SendMessage(CB_RESETCONTENT, 0, 0); }
	void InsertString (int nIndex, LPCTSTR lpszString);
	void InsertStringW(int nIndex, LPCWSTR lpszString);
	int  SetCurSel(int nSelect);
	int  GetCurSel() const;
};

class RLWndButton: public RLWnd
{
public:
	RLWndButton(HWND hwnd=0):RLWnd(hwnd) {}
	void SetCheck(int nCheck);
	void SetCheckBool(bool checked);
	int  GetCheck() const;
	bool GetCheckBool() const;
};


class RLWndTab: public RLWndEx
{
public:
	BOOL SetItem (int nItem, TCITEM*  pTabCtrlItem);
	BOOL SetItemW(int nItem, TCITEMW* pTabCtrlItem);
	int GetCurSel();
	int SetCurSel(int nItem);
	BOOL InsertItem(int nItem, LPCTSTR lpszItem);
};


class RLDlgBase : public RLWnd
{
public:
	int  DoModal(HWND hWndParent=NULL);
	void DoModeless(HWND hWndParent=NULL);
	int  DoModalIndirect(HWND hWndParent, LPCDLGTEMPLATEA hDialogTemplate, HINSTANCE hInstance = (HINSTANCE)0x400000);

protected:
	LPCTSTR m_lpTemplateName;
	virtual BOOL    OnInitDialog();
	virtual BOOL    OnEndDialog(BOOL ok);
	virtual HBRUSH  OnCtlColor(HDC hdc, HWND hwnd) { return 0; }
	virtual INT_PTR WindowProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	static INT_PTR CALLBACK WindowProcStatic(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};


#endif // !defined(_RL_WND_H____INCLUDED_)
