#include "stdafx.h"
#include <Htmlhelp.h>
#include "vrMain.h"
#include "vrHelp.h"
#include "../main/resource.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#pragma comment(lib, "HtmlHelp.lib")

VrHelp::VrHelp()
{
	m_dwCookie = NULL;
	::HtmlHelp(NULL, NULL, HH_INITIALIZE, (DWORD)&m_dwCookie);
}


void VrHelp::Popup(LPARAM lParam) 
{
	LPHELPINFO hlp = (LPHELPINFO) lParam;
	HH_POPUP popup;

	if (hlp->iCtrlId != 0) {
		
		popup.cbStruct = sizeof(popup);
		popup.hinst = vrMain.m_hInstance;
		popup.idString = (UINT)hlp->iCtrlId;
		SetRect(&popup.rcMargins, -1, -1, -1, -1);
		popup.pszFont = "MS Sans Serif, 8, , ";
		popup.clrForeground = -1;
		popup.clrBackground = -1;
		popup.pt.x = -1;
		popup.pt.y = -1;

		switch  (hlp->iCtrlId) {
		case IDC_STATIC_LEVEL:
		case IDC_STATIC_TEXT_LEVEL:
		//case IDC_STATIC_FAST:
		//case IDC_STATIC_BEST:
		//	popup.idString = IDC_COMPRESSLEVEL;
		//	break;
		case IDC_STATIC_TEXT_QUALITY:
		//case IDC_STATIC_JPEG1:
		//case IDC_STATIC_JPEG2:
		//	popup.idString = IDC_QUALITYLEVEL;
		//	break;
		//case IDC_STATIC_ENCODING:
		//	popup.idString = IDC_ENCODING;
		//	break;
		case IDC_STATIC_SCALE:
		case IDC_STATIC_P:
			popup.idString = IDC_SCALE_EDIT;
			break;
		}

		::HtmlHelp((HWND)hlp->hItemHandle, NULL, HH_DISPLAY_TEXT_POPUP, (DWORD)&popup);
	}
}

BOOL VrHelp::TranslateMsg(MSG *pmsg)
{
	return (::HtmlHelp(NULL, NULL, HH_PRETRANSLATEMESSAGE, (DWORD)pmsg) != 0);
}

VrHelp::~VrHelp()
{
	::HtmlHelp(NULL, NULL, HH_UNINITIALIZE, (DWORD)m_dwCookie);
}

