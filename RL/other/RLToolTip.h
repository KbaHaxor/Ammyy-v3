#if !defined(AFX_RLTOOLTIP_H__1C303488_3CEB_45C9_AC85_84DD276DBE1E__INCLUDED_)
#define AFX_RLTOOLTIP_H__1C303488_3CEB_45C9_AC85_84DD276DBE1E__INCLUDED_

class RLToolTip
{
public:
	RLToolTip();
	~RLToolTip();

	//void RelayEvent(LPMSG lpMsg);
	void Activate(BOOL bActivate);


	BOOL Create(HWND hwndParent);	
	BOOL AddToolW(HWND hwnd, LPCWSTR lpszText, LPCRECT lpRectTool, UINT nIDTool);
	void UpdateTipTextW(LPCWSTR lpszText, HWND hwnd, UINT nIDTool);
	//int  GetToolCount() const;

private:
	void FillInToolInfo(TOOLINFOW& ti, HWND hwnd, UINT nIDTool) const;

public:
	HWND m_hWnd;
};

#endif // !defined(AFX_RLTOOLTIP_H__1C303488_3CEB_45C9_AC85_84DD276DBE1E__INCLUDED_)
