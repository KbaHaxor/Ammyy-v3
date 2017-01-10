#include "stdafx.h"
#include "RLSheet.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

RLPropertyPage::RLPropertyPage(LPCWSTR title, LPCWSTR pszTemplate, HINSTANCE hInstance)
{
    m_hpage = NULL;

    // initialize PROPSHEETPAGE struct
    memset(&m_psp, 0, sizeof(PROPSHEETPAGE));
    m_psp.dwSize = sizeof(PROPSHEETPAGE);
    m_psp.dwFlags = PSP_USECALLBACK;
    m_psp.hInstance = hInstance;
    m_psp.pszTemplate = pszTemplate;
    m_psp.pfnDlgProc = (DLGPROC)RLPropertyPage::WindowProcStatic;
    m_psp.pfnCallback = RLPropertyPage::PropPageCallback;
    m_psp.lParam = (LPARAM)this;

    if (title) {
		m_psp.pszTitle = title;
		m_psp.dwFlags |= PSP_USETITLE;
	}
}

RLPropertyPage::~RLPropertyPage()
{
}


BOOL RLPropertyPage::OnInitDialog()
{
	return TRUE;
}

void RLPropertyPage::OnOK()
{
}

BOOL RLPropertyPage::OnKillActive()
{ 
	return TRUE;
}

INT_PTR RLPropertyPage::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return 0;
}


INT_PTR RLPropertyPage::WindowProcStatic(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg==WM_INITDIALOG) {        
		PROPSHEETPAGE* pPage = (PROPSHEETPAGE*)lParam;
		RLPropertyPage* _this = (RLPropertyPage*)pPage->lParam;
		::SetWindowLong(hwnd, GWL_USERDATA, (long)_this);
		_this->m_hWnd = hwnd;
		return _this->OnInitDialog();
	}

	RLPropertyPage* _this = (RLPropertyPage*) ::GetWindowLong(hwnd, GWL_USERDATA);		

	if (uMsg==WM_NOTIFY) 
	{		
		ASSERT(_this!=NULL);
		ASSERT(lParam!=0);

		UINT code = ((NMHDR*)lParam)->code;
		
		if (code==PSN_APPLY) {
			_this->OnOK(); // PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE;
			return TRUE;
		}
		else if (code==PSN_KILLACTIVE) {
			BOOL b = !_this->OnKillActive();
			::SetWindowLong(hwnd, DWL_MSGRESULT, b);
			return TRUE;
		}
	}
	return (_this) ? _this->WindowProc(hwnd, uMsg, wParam, lParam) : 0;
}


UINT RLPropertyPage::PropPageCallback(HWND , UINT uMsg, LPPROPSHEETPAGEW ppsp)
{
	UINT uRet = 0;

    switch(uMsg)
    {
		case PSPCB_CREATE:
		{
			uRet = 1;
			break;
		}
	}

	return uRet;
}



RLPropertySheet::RLPropertySheet(LPCWSTR title, UINT uStartPage)
{
	//m_hWnd = NULL;
        
    memset(&m_psh, 0, sizeof(PROPSHEETHEADERW));
    m_psh.dwSize = sizeof(PROPSHEETHEADERW);
    m_psh.dwFlags = PSH_USECALLBACK | PSH_NOAPPLYNOW;
    m_psh.hInstance = (HINSTANCE)0x400000;
    m_psh.phpage = NULL;   // will be set later
    m_psh.nPages = 0;      // will be set later
    m_psh.pszCaption = title;
    m_psh.nStartPage = uStartPage;
    m_psh.pfnCallback = RLPropertySheet::PropSheetCallback;
}

RLPropertySheet::~RLPropertySheet()
{
}

INT_PTR RLPropertySheet::DoModal(HWND hWndParent)
{
    //ASSERT(m_hWnd == NULL);

    m_psh.hwndParent = hWndParent;
    m_psh.phpage = (HPROPSHEETPAGE*)m_arrPages.GetBuffer();
    m_psh.nPages = m_arrPages.GetLen()/sizeof(HPROPSHEETPAGE);

    INT_PTR nRet = ::PropertySheetW(&m_psh);

    return nRet;
}


void RLPropertySheet::AddPage(RLPropertyPage* pPage)
{
    HPROPSHEETPAGE hPage = ::CreatePropertySheetPageW(&pPage->m_psp);
    if( hPage== NULL) return;

    pPage->m_hpage = hPage;
    m_arrPages.AddRaw(&pPage->m_hpage, sizeof(HPROPSHEETPAGE));
}


int CALLBACK RLPropertySheet::PropSheetCallback(HWND hWnd, UINT uMsg, LPARAM lParam)
{   
	typedef struct {
		WORD dlgVer;
		WORD signature;
		DWORD helpID;
		DWORD exStyle;
		DWORD style;
	} DLGTEMPLATEEX;

    if (uMsg == PSCB_PRECREATE)  {
        if (lParam)
        {
            if (((DLGTEMPLATEEX*)lParam)->signature ==  0xFFFF){
				((DLGTEMPLATEEX*)lParam)->style &= ~DS_CONTEXTHELP;
            }
            else {
				((DLGTEMPLATE*  )lParam)->style &= ~DS_CONTEXTHELP;
			}
        }
	}
	else if (uMsg == PSCB_INITIALIZED)
    {
		//::SendMessage(hWnd, PSM_CHANGED, 0, 0);

        //ASSERT(hWnd != NULL);
        //RLPropertySheet * pT = RLPropertySheet::ExtractCreateWndData();
        //// subclass the sheet window
        //pT->SubclassWindow(hWnd);
        //// remove page handles array
        //pT->_CleanUpPages();
    }

    return 0;
}

