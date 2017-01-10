#if !defined(AFX_DLGMAIN_H__CF12CA87_B97F_4D06_B1FA_4C2A40F353A0__INCLUDED_)
#define AFX_DLGMAIN_H__CF12CA87_B97F_4D06_B1FA_4C2A40F353A0__INCLUDED_

#include "../RL/RLWnd.h"

class DlgMain  : public RLDlgBase
{
public:
	DlgMain();
	virtual ~DlgMain();

	bool m_autoStart;

protected:
	virtual INT_PTR WindowProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnInitDialog();
	virtual BOOL OnEndDialog(BOOL ok);

private:	
	void OnBtnStart();
	void OnBtnStop();
	void OnInitMenuPopup(HMENU hMenu, UINT nIndex, BOOL bSysMenu);
};


#endif // !defined(AFX_DLGMAIN_H__CF12CA87_B97F_4D06_B1FA_4C2A40F353A0__INCLUDED_)
