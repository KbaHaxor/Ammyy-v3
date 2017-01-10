#if !defined(AFX_RLPROPERTYSHEET_H__7107D9C8_B8CE_49CD_8F6F_286D8907B6DA__INCLUDED_)
#define AFX_RLPROPERTYSHEET_H__7107D9C8_B8CE_49CD_8F6F_286D8907B6DA__INCLUDED_

#include "RLStream.h"

class RLPropertyPage
{
public:
    RLPropertyPage(LPCWSTR title, LPCWSTR pszTemplate, HINSTANCE hInstance = (HINSTANCE)0x400000);
    ~RLPropertyPage(void);
    
protected:
	virtual BOOL    OnInitDialog();
	virtual BOOL	OnKillActive();
	virtual void    OnOK();
	virtual INT_PTR WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    static INT_PTR CALLBACK WindowProcStatic(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static UINT    CALLBACK PropPageCallback(HWND hWnd, UINT uMsg, LPPROPSHEETPAGEW ppsp);

protected:
	HWND m_hWnd;

private:
	HPROPSHEETPAGE m_hpage;
	PROPSHEETPAGEW m_psp;

	friend class RLPropertySheet;
};


class RLPropertySheet
{
public:
    RLPropertySheet(LPCWSTR title = NULL, UINT uStartPage = 0);
    ~RLPropertySheet(void);
    
    INT_PTR DoModal(HWND hWndParent = NULL);
    
    void AddPage(RLPropertyPage* pPage);

private:
    static int CALLBACK PropSheetCallback(HWND hWnd, UINT uMsg, LPARAM lParam);

private:
	PROPSHEETHEADERW m_psh;
	RLStream m_arrPages;
    //HWND m_hWnd;    
};

#endif // !defined(AFX_RLPROPERTYSHEET_H__7107D9C8_B8CE_49CD_8F6F_286D8907B6DA__INCLUDED_)
