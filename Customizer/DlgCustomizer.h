#if !defined(_CUSTOMIZER_DLG_H__0CD85352__INCLUDED_)
#define _CUSTOMIZER_DLG_H__0CD85352__INCLUDED_

#include "../RL/RLWnd.h"

class DlgCustomizer : public RLDlgBase
{
public:
	DlgCustomizer();

protected:
	virtual INT_PTR WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	HICON m_hIcon;		
	virtual BOOL OnInitDialog();
	void OnSysCommand(UINT nID, LPARAM lParam);
	void OnPaint();
	HCURSOR OnQueryDragIcon();
	void OnBrowseSourceIcon();
	void OnTargetBrowse();
	void OnBtnUpdate();
	void OnBtnPermissions();

private:
	RLWnd  m_wndIcoPath;
	RLWnd  m_wndExePath;
	HWND   m_UpdateButton;
};

#endif // !defined(_CUSTOMIZER_DLG_H__0CD85352__INCLUDED_)
