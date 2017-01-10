#include "stdafx.h"
#include "DlgAddComputer.h"
#include "resource.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


DlgAddComputer2::DlgAddComputer2(bool edit)
{
	m_lpTemplateName = MAKEINTRESOURCE(IDD_ADD_COMPUTER);
	m_edit = edit;
}

DlgAddComputer2::~DlgAddComputer2()
{
}

BOOL DlgAddComputer2::OnInitDialog()
{
	m_wndFolder      .AttachDlgItem(m_hWnd, IDC_FOLDER);
	m_wndComputerID  .AttachDlgItem(m_hWnd, IDC_COMPUTER_ID);
	m_wndComputerName.AttachDlgItem(m_hWnd, IDC_COMPUTER_ALIAS);
	m_wndLabelID     .AttachDlgItem(m_hWnd, IDC_LABEL_ID);
	m_wndDescription .AttachDlgItem(m_hWnd, IDC_DESCRIPTION);

	this->SetTextW(L"Ammyy - "+rlLanguages.GetValue(D_CONTACT_BOOK));

	bool folder = (m_edit) ? (m_computerID.GetLength()==0) : false;

	m_wndFolder.SetCheckBool(folder);
	m_wndComputerID.SetTextW(m_computerID);
	m_wndComputerName.SetTextW(m_alias);
	m_wndDescription .SetTextW(m_description);

	if (m_edit) ::EnableWindow(m_wndFolder, FALSE);

	OnFolderCheckBoxClicked();

	return TRUE;
}



BOOL DlgAddComputer2::OnEndDialog(BOOL ok)
{
	if (!ok) return TRUE;

	bool folder = m_wndFolder.GetCheckBool();

	CStringW computerID;

	if (!folder) {
		computerID = m_wndComputerID.GetTextA();
		computerID.TrimRight();
		computerID.TrimLeft();

		if (computerID.GetLength()==0) {
			::MessageBox(m_hWnd, "ID is empty", "Ammyy Admin", MB_ICONERROR);
			return FALSE;
		}
	}

	m_computerID  = computerID;
	m_alias       = m_wndComputerName.GetTextW();
	m_description = m_wndDescription .GetTextW();

	return TRUE;
}

void DlgAddComputer2::OnFolderCheckBoxClicked()
{
	bool folder = m_wndFolder.GetCheckBool();

	m_wndComputerID.ShowWindow1(!folder);
	m_wndLabelID   .ShowWindow1(!folder);
}

INT_PTR DlgAddComputer2::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{	
	switch (uMsg) 
	{
	case WM_COMMAND:
		{
			WORD wItem = LOWORD(wParam);
									
			if (BN_CLICKED  == HIWORD(wParam)) {
				if ((HWND)lParam==(HWND)m_wndFolder) { 
					OnFolderCheckBoxClicked();
					return TRUE;
				}				
			}
		}
		break;
	}
	return 0;
}


