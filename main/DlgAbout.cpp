#include "stdafx.h"
#include "DlgAbout.h"
#include "resource.h"
#include "Common.h"
#include "Image.h"
#include "DlgMain.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

DlgAbout::DlgAbout()
{
	m_lpTemplateName = MAKEINTRESOURCE(IDD_ABOUTBOX);
	m_bkgndDC = NULL;
}

DlgAbout::~DlgAbout()
{
	if (m_bkgndDC!=NULL) ::DeleteDC(m_bkgndDC);
}

BOOL DlgAbout::OnInitDialog()
{
	CStringA str1 = CStringA("Ammyy Admin v") + settings.GetVersionSTR();
	LPCSTR  str2 = /*CStringA("Date:") + */CCommon::GetBuildDateTime();

	m_wnd2.AttachDlgItem(m_hWnd, IDC_BUILD_TIME);
	m_wnd2.SetTextA(str2);

	m_font1.CreatePointFont(150, "Arial");

	m_wnd1.AttachDlgItem(m_hWnd, IDC_AMMYY_VERSION);
	m_wnd1.SetFont((HFONT)m_font1, FALSE);
	m_wnd1.SetTextA(str1);

	m_wnd3.AttachDlgItem(m_hWnd, IDC_STATIC_COPYRIGHT);
	m_wnd4.AttachDlgItem(m_hWnd, IDOK);

	m_link.AttachDlgItem(m_hWnd, IDC_LINK);
	m_link.SubclassWindow();

	m_link_custom.AttachDlgItem(m_hWnd, IDC_LINK_CUSTOM);

	if (!settings.m_customization.m_url.IsEmpty()) {
		m_link_custom.SubclassWindow();
		m_link_custom.SetTextA(settings.m_customization.m_url);
	}

	// prepare background
	{
		m_size = this->GetClientSize();

		Image img(m_size.cx, m_size.cy);
		img.Clear(0xFF0000);

		//Gradients draw
		//int cy_main = h - 36;
		//img.DrawGradientV(188,188, 0, 1);	//top separator	
		//img.DrawGradientV(251,210, 1, cy_main);	//main
		img.DrawGradientV(250,200, 0, m_size.cy);	//bottom separator
		//img.DrawGradientV(247,187, cy_main+2, h-cy_main-2);	//bottom

		HBITMAP hBitmap = img.GetBitmap();
		m_bkgndDC = ::CreateCompatibleDC(NULL);
		::SelectObject(m_bkgndDC, hBitmap);
		::DeleteObject(hBitmap);

		HICON hIcon = (HICON)::LoadImage(TheApp.m_hInstance, MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 48, 48, LR_DEFAULTCOLOR);
		::DrawIconEx(m_bkgndDC, 11, 11, hIcon, 48, 48, 0, NULL, DI_NORMAL);
		::DestroyIcon(hIcon);
	}

	return TRUE;
}

void DlgAbout::OnEraseBackground(HDC hdc)
{
	::BitBlt(hdc, 0, 0, m_size.cx, m_size.cy, m_bkgndDC, 0, 0, SRCCOPY);
}

HBRUSH DlgAbout::OnCtlColor(HDC hdc, HWND hwnd) 
{
	if (hwnd==(HWND)m_link) {
		return m_link.OnCtlColor(hdc, hwnd);
	}
	if (hwnd==(HWND)m_link_custom) {
		return m_link_custom.OnCtlColor(hdc, hwnd);
	}
	if (hwnd==m_wnd1 || hwnd==m_wnd2 || hwnd==m_wnd3 || hwnd==m_wnd4)
	{
		::SetBkMode(hdc, TRANSPARENT);
		return DlgMain::m_pObject->m_hNullBrush;
	}
	
	return 0;
}

INT_PTR DlgAbout::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{	
	if (msg==WM_CTLCOLORSTATIC || msg==WM_CTLCOLOREDIT || msg == WM_CTLCOLORBTN) 
	{
		return (INT_PTR)this->OnCtlColor((HDC)wParam, (HWND)lParam);
	}
	else if (msg==WM_ERASEBKGND) {
		this->OnEraseBackground((HDC)wParam);
		return TRUE;
	}
	return RLDlgBase::WindowProc(hwnd, msg, wParam, lParam);
}
