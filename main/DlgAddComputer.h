#if !defined(_ADDCOMPUTER_2_H__62753545_E82F_4364_AE09_C310F99F089E__INCLUDED_)
#define _ADDCOMPUTER_2_H__62753545_E82F_4364_AE09_C310F99F089E__INCLUDED_

#include "../RL/RLWnd.h"

class DlgAddComputer2 : public RLDlgBase  
{
public:
	DlgAddComputer2(bool edit);
	virtual ~DlgAddComputer2();

	CStringW m_computerID;
	CStringW m_alias;
	CStringW m_description;

protected:
	virtual BOOL OnEndDialog(BOOL ok);
	virtual BOOL OnInitDialog();
	virtual INT_PTR WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	void OnFolderCheckBoxClicked();

	RLWndEditBox m_wndComputerID;
	RLWndEditBox m_wndComputerName;
	RLWndEditBox m_wndDescription;
	RLWndButton	 m_wndFolder;
	RLWnd		 m_wndLabelID;
	bool		 m_edit;		// true for edit, false for new
};

#endif // !defined(_ADDCOMPUTER_2_H__62753545_E82F_4364_AE09_C310F99F089E__INCLUDED_)
