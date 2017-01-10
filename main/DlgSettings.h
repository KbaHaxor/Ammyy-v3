#if !defined(AFX_DLGSETTINGS_H__5F544A95_C26A_44C2_951B_638BC7C470B8__INCLUDED_)
#define AFX_DLGSETTINGS_H__5F544A95_C26A_44C2_951B_638BC7C470B8__INCLUDED_

#include "../RL/RLWnd.h"
#include "../RL/RLSheet.h"


// common

class DlgSettingsPage1 : public RLPropertyPage
{
public:
	DlgSettingsPage1(LPCWSTR title, LPCWSTR pszTemplate):RLPropertyPage(title, pszTemplate) {}	

protected:
	virtual INT_PTR WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnInitDialog();
	virtual void OnOK();	

private:
	void OnKeyDown(LPMSG msg);
	static BOOL CALLBACK DSEnumCallback(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext);

	class CDeviceList {
	public:
		void Add(LPGUID lpGuid, LPCSTR description);
		void OnOK();
		RLStream m_guids;
		RLWndComboBox m_comboBox;
		int		m_index;
		LPGUID	lpGuidSettings;
	};

	CDeviceList m_sPlay;
	CDeviceList m_sRecd;

	RLWndButton   m_debugLog;
	RLWndButton   m_runAsSystem;
	RLWndButton   m_accessFiles;
};


// network page
//
class DlgSettingsPage2 : public RLPropertyPage
{
public:
	DlgSettingsPage2(LPCWSTR title, LPCWSTR pszTemplate):RLPropertyPage(title, pszTemplate) {}

protected:
	virtual BOOL    OnInitDialog();
	virtual BOOL	OnKillActive();
	virtual void    OnOK();	
	virtual INT_PTR WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	void OnUseWANChanged();
	void OnHttpProxy();
	void OnExtTCPports();
	void OnRouterTypeChanged();	
	void OnDirectTCPChanged();
	CStringA GetRouterText();


	RLWndComboBox m_routerType;
	RLWndEditBox  m_routerText;
	RLWndButton   m_useWAN;
	RLWndButton   m_wndAllowIncomingByIP;
	RLWndButton   m_wndAllowDirectTCP;
};


// client page

class DlgSettingsPage3 : public RLPropertyPage
{
public:
	DlgSettingsPage3(LPCWSTR title, LPCWSTR pszTemplate):RLPropertyPage(title, pszTemplate) {}

protected:
	virtual INT_PTR WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual BOOL    OnInitDialog();
	virtual void	OnOK();

private:
	void OnBtnPermissions();
};


// operator page

class DlgSettingsPage4 : public RLPropertyPage
{
public:
	DlgSettingsPage4(LPCWSTR title, LPCWSTR pszTemplate):RLPropertyPage(title, pszTemplate) {}

protected:
	virtual INT_PTR WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual BOOL    OnInitDialog();
	virtual void	OnOK();

private:
	RLWndComboBox m_encryption;
};

#endif // !defined(AFX_DLGSETTINGS_H__5F544A95_C26A_44C2_951B_638BC7C470B8__INCLUDED_)
