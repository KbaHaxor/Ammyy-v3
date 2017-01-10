#include "stdafx.h"
#include "VrDlgPasswordInput.h"
#include "../main/resource.h"
#include "../main/Common.h"
#include "../main/Image.h"
#include "../main/DlgMain.h"


VrDlgPasswordInput::VrDlgPasswordInput()
{
	m_lpTemplateName = MAKEINTRESOURCE(IDD_PASSWORD_INPUT);
	m_bkgndDC = NULL;
}

VrDlgPasswordInput::~VrDlgPasswordInput()
{
	if (m_bkgndDC!=NULL) ::DeleteDC(m_bkgndDC);
}

BOOL VrDlgPasswordInput::OnInitDialog()
{
	m_wndComputerId.AttachDlgItem(m_hWnd, IDC_COMPUTER_ID);
	m_wndComputerId.SetTextA(m_computer);

	LPCSTR txt = (m_first) ?
		"Please enter password for accessing remote computer" :
		"The password you have entered is invalid, please try again";

	m_wStatic1.AttachDlgItem(m_hWnd, IDC_STATIC1);
	m_wStatic1.SetTextA(txt);

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

void VrDlgPasswordInput::OnEraseBackground(HDC hdc)
{
	::BitBlt(hdc, 0, 0, m_size.cx, m_size.cy, m_bkgndDC, 0, 0, SRCCOPY);
}

BOOL VrDlgPasswordInput::OnEndDialog(BOOL ok)
{
	if (ok) {
		RLWnd wndPassword(::GetDlgItem(m_hWnd, IDC_PASSWORD));
		m_password = wndPassword.GetTextA();
	}
	return TRUE;
}

HBRUSH VrDlgPasswordInput::OnCtlColor(HDC hdc, HWND hwnd) 
{
	::SetBkMode(hdc, TRANSPARENT);

	if (hwnd==(HWND)m_wndComputerId) 
	{
		return _GUICommon.m_brush[3];
	}
	if (hwnd==(HWND)m_wStatic1 && !m_first) {
		::SetTextColor(hdc ,RGB(195,32,32));
	}
	return DlgMain::m_pObject->m_hNullBrush;
}

INT_PTR VrDlgPasswordInput::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{	
	if (msg==WM_ERASEBKGND) {
		this->OnEraseBackground((HDC)wParam);
		return TRUE;
	}
	return RLDlgBase::WindowProc(hwnd, msg, wParam, lParam);
}