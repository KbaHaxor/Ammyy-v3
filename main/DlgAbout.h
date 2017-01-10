#if !defined(AFX_DLGABOUT_H__0B8E0B3E_F4A2_4AE2_A703_4869F026231A__INCLUDED_)
#define AFX_DLGABOUT_H__0B8E0B3E_F4A2_4AE2_A703_4869F026231A__INCLUDED_

#include "../RL/RLWnd.h"
#include "../RL/FontWTL.h"
#include "StaticLink.h"

class DlgAbout : public RLDlgBase
{
public:
	DlgAbout();
	virtual ~DlgAbout();
	BOOL OnInitDialog();

protected:
	virtual INT_PTR WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	HBRUSH OnCtlColor(HDC hdc, HWND hwnd);

private:
	void OnEraseBackground(HDC hdc);

	CFontWTL m_font1;

	CStaticLink m_link;
	CStaticLink m_link_custom;

	RLWnd m_wnd1, m_wnd2, m_wnd3, m_wnd4;

	SIZE m_size;
	HDC	m_bkgndDC;
};

#endif // !defined(AFX_DLGABOUT_H__0B8E0B3E_F4A2_4AE2_A703_4869F026231A__INCLUDED_)
