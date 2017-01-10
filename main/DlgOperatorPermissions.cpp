#include "stdafx.h"
#include "DlgOperatorPermissions.h"
#include "resource.h"
#include "Common.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


DlgOperatorPermissions::DlgOperatorPermissions(bool edit)
{
	m_lpTemplateName = MAKEINTRESOURCE(IDD_OPERATOR_PERMISSION);
	m_blockPassword = edit;
}

DlgOperatorPermissions::~DlgOperatorPermissions() {}

BOOL DlgOperatorPermissions::OnInitDialog()
{
	this->SetTextW(L"Ammyy - " + rlLanguages.GetValue(D_ACCESS_PERMISSIONS));

	m_wPassword1.AttachDlgItem(m_hWnd, IDC_PASSWORD1);
	m_wPassword2.AttachDlgItem(m_hWnd, IDC_PASSWORD2);

	m_wComputerId.AttachDlgItem(m_hWnd, IDC_COMPUTER_ID);

	char buffer[16];

	if (m_permission.m_id>0)
		sprintf(buffer, "%u", m_permission.m_id);
	else
		strcpy(buffer, "ANY");

	m_wComputerId.SetTextA(buffer);

	if (m_blockPassword) {
		if (m_permission.m_password.IsEmpty())
			m_blockPassword = false;
	}

	InitPasswords();

	return TRUE;
}


BOOL DlgOperatorPermissions::OnEndDialog(BOOL ok)
{
	if (!ok) return TRUE;

	CStringA txt = m_wComputerId.GetTextA();

	if (txt=="ANY") {
		m_permission.m_id = 0;
	}
	else {
		txt.Remove(' ');
		INT id = atol(txt);
		if (id<=0) {
			::MessageBox(m_hWnd, "Incorrect computer ID", "Ammyy Admin", MB_ICONWARNING);
			return FALSE;
		}
		m_permission.m_id = id;
	}

	if (!m_blockPassword) 
	{
		CStringA password = m_wPassword1.GetTextA();

		if (password!=m_wPassword2.GetTextA()) {
			LPCSTR txt = "The password was not correctly confirmed. Please ensure that the password and confirmation match exactly.";
			::MessageBox(m_hWnd, txt, "Ammyy Admin", MB_ICONSTOP);
			return FALSE;
		}

		if (m_permission.m_id==0 && password.IsEmpty()) {
			LPCSTR txt = "We don't recommend for security reason to create permission for ANY operator without password\n\nDo you want to continue?";
			if (::MessageBoxA(m_hWnd, txt, "Ammyy Admin", MB_ICONWARNING|MB_YESNO|MB_DEFBUTTON2)==IDNO) return FALSE;
		}
		m_permission.m_password.Calculate(password);
	}

	return TRUE;
}

void DlgOperatorPermissions::InitPasswords()
{
	LPCSTR text   = (m_blockPassword) ? "**********" : "";
	BOOL   enable = (m_blockPassword) ? FALSE : TRUE;
	::EnableWindow(m_wPassword1, enable);
	::EnableWindow(m_wPassword2, enable);
	m_wPassword1.SetTextA(text);
	m_wPassword2.SetTextA(text);
}

void DlgOperatorPermissions::OnPasswordReset()
{
	if (m_blockPassword) {
		m_blockPassword = false;
		InitPasswords();
	}
}


INT_PTR DlgOperatorPermissions::WindowProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) 
	{
	case WM_LBUTTONDBLCLK:
		{
			POINT p;
			p.x = LOWORD(lParam);
			p.y = HIWORD(lParam);

			::ClientToScreen(m_hWnd, &p);			

			if (m_wPassword1.IsInsideWindow(p) || m_wPassword2.IsInsideWindow(p)) {
				OnPasswordReset();
			}
		}
	}
	return 0;
}

