#if (!defined(_ACCEPT_DIALOG_H__INCLUDED))
#define _ACCEPT_DIALOG_H__INCLUDED

#include "../RL/RLWnd.h"
#include "../main/Transport.h"

class TrDlgAccept: public RLDlgBase
{
public:
	TrDlgAccept();
	virtual ~TrDlgAccept();

	bool	m_remember;

	Permission	m_prm;
	Transport*  m_transport;

protected:
	virtual BOOL OnInitDialog();
	virtual BOOL OnEndDialog(BOOL ok);
	virtual HBRUSH OnCtlColor(HDC hdc, HWND hwnd);
	virtual INT_PTR WindowProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	void   OnExit(bool accept);
	void   SaveValues();

	RLWndButton m_wndBtn[5], m_wndRemember;
};

#endif
