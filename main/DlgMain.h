#if !defined(AFX_INSTALLERDLG_H__99E8260B_D667_4EAC_9B4F_82A0FCFA2069__INCLUDED_)
#define AFX_INSTALLERDLG_H__99E8260B_D667_4EAC_9B4F_82A0FCFA2069__INCLUDED_

#include "../RL/other/TextProgressCtrl.h"
#include "InteropViewer.h"
#include "../RL/RLWnd.h"
#include "../RL/other/RLToolTipButton.h"
#include "ServerInteract.h"
#include "Light.h"
#include "DlgContactBook.h"


// Message used for system tray notifications

// need separate global object, cause TrDlgAccept can get m_brush after ~DlgMain()
class GUICommon
{
public:
	HBRUSH	 m_brush[4];

	GUICommon()
	{
		for (int i=0; i<COUNTOF(m_brush); i++) m_brush[i] = NULL;
	}

	~GUICommon()
	{
		for (int i=0; i<COUNTOF(m_brush); i++) {
			if (m_brush[i]!=NULL) ::DeleteObject(m_brush[i]);
		}
	}
};

extern GUICommon _GUICommon;


class DlgMain : public RLWndEx, public CServerInteract
{
// Construction
public:
	DlgMain();
	~DlgMain();

	static DlgMain* m_pObject;

	void OnBtnSettings(); // can be called directly
	void DoModal();

	static UINT ConvertID(LPCWSTR pID);
	static LPCWSTR m_pClassName;

// Implementation
private:
	void SetTitle();
	void OnDownloadFileStatus(LPCSTR msg, int percents);

	//virtual INT_PTR WindowProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	  virtual LRESULT WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	virtual BOOL OnInitDialog();
	void OnSysCommand(UINT nID, LPARAM lParam);
	void OnPaint();
	HCURSOR OnQueryDragIcon();
	void OnTimer(UINT nIDEvent);
	void OnBtnStart();
	void OnBtnStop();
public:
	void OnBtnConnect();
private:
	void OnBtnContactBook();
	void OnSelchangeType();
	void OnChangeLanguage(UINT nID);
	void OnHelpAbout();
	void OnHelpWebsite();
	void OnMenuExit();
	void OnClickYourID();
	void FillConnectionType(int count_old);
	
	virtual OnEndDialog(BOOL ok);

	void OnInitMenuPopup(HMENU hMenu, UINT nIndex, BOOL bSysMenu);

private:
	static void GetMenuText(HMENU hMenu, int menuCmd, WCHAR* buffer, int bufferLen);
	void CreateBackgroundImage();
	LRESULT OnMeasureItem(HWND hWnd, MEASUREITEMSTRUCT* mis);
	LRESULT OnDrawItem(DRAWITEMSTRUCT* dis);
	LRESULT OnCtlColor(HWND wnd, HDC hdc);
	void OnEraseBackground(HDC hdc);
	void SendInitCmd2();
	void OnClose();

	static void SetMenuText(HMENU hMenu, UINT uItem, UINT uTextIndex);

	int m_iLang;	// index of language

	RLWndComboBox		m_wndSpeed;
public:
	RLWnd				m_wndRemoteIdEdit;
private:
	RLWnd				m_wndBtnStart, m_wndBtnStop;
	RLWnd				m_wndStatus;
	RLWnd				m_wndBtnConnect;
	RLWndButton			m_wndViewOnly;
	RLWnd				m_wndYourIdEdit;
	RLWnd				m_wndYourIpEdit;
	RLToolTipButton		m_wndBtnContactBook;
	DlgContactBook		m_dlgContactBook;

public:
	HBRUSH	 m_hNullBrush;

private:
	NOTIFYICONDATA m_nid;

	HMENU	m_hMenu;
	HMENU	m_hMenuForID;

	HBRUSH	 m_bkgndBrush;
private:
	HFONT	 m_fonts[3];
	WndLight m_wndLights[12];
	bool	 m_onTimerCalled;

	int m_cx, m_cy;
	HDC	m_bkgndDC;

	struct Menu
	{
		HFONT hFont;
	} m_menu;

public:
	HICON	m_hIconSmall;
	HICON	m_hIconBig;
};

#endif // !defined(AFX_INSTALLERDLG_H__99E8260B_D667_4EAC_9B4F_82A0FCFA2069__INCLUDED_)

