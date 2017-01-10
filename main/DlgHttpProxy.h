#if !defined(AFX_DLGHTTPPROXY_H__283C8A1A_F5DA_446E_B6EE_DB08AC65733F__INCLUDED_)
#define AFX_DLGHTTPPROXY_H__283C8A1A_F5DA_446E_B6EE_DB08AC65733F__INCLUDED_

#include "../RL/RLWnd.h"

class DlgHttpProxy : public RLDlgBase 
{
public:
	DlgHttpProxy();
	virtual ~DlgHttpProxy();

	// Get current IE proxy server, return port or 0 if proxy if not set
	static UINT16 GetIEProxy(CStringA& host);
	
protected:
	virtual INT_PTR WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnInitDialog();
	virtual BOOL OnEndDialog(BOOL ok);

private:
	bool m_use;
	void OnProxyTypeChanged();
	void OnBtnSetIEProxy();
	void SetControlsEnabled();

	RLWnd m_wndAddress, m_wndPort, m_wndUsername, m_wndPassword;
	RLWnd m_wndIEAddress, m_wndIEPort;

	RLWndButton m_wndUseProxy;
};

#endif // !defined(AFX_DLGHTTPPROXY_H__283C8A1A_F5DA_446E_B6EE_DB08AC65733F__INCLUDED_)
