#if !defined(AFX_DLGACCESSLIST_H__00904B5E_FA86_45A5_8310_D63A3A082E54__INCLUDED_)
#define AFX_DLGACCESSLIST_H__00904B5E_FA86_45A5_8310_D63A3A082E54__INCLUDED_

#include "../RL/RLWnd.h"
#include "Settings.h"

class DlgPermissionList : public RLDlgBase 
{
public:
	class Panel {
	public:
		void Create(HWND m_hWndDialog, int x, int y);
		RLWndButton m_wnd[7]; // check-boxes
	private:
		void Create1(RLWnd& wnd, HWND m_hWndDialog, LPCSTR wName, DWORD style, int x, int y, int cx, int cy, HFONT hFont);
	};
public:
	DlgPermissionList();
	virtual ~DlgPermissionList();

protected:
	virtual INT_PTR WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnInitDialog();
	virtual BOOL OnEndDialog(BOOL ok);

private:
	int  InsertComputerRow(int item, UINT32 computerID);
	void SetPermission(int iComputer, int iPermission, bool value);
	void FillListView(int index);
	void FillRow(int index);
	int GetSelectedItem();
	int FindKey(const Permission& v, int* where);
	void OnPermissionCheckBoxClicked(int index);
	void OnBtnAdd();
	void OnBtnEdit();
	void OnBtnRemove();
	BOOL OnItemChanged1(NMLISTVIEW* pNMListView);
	void OnChangedSelectedComputer(int index);

	std::vector<Permission> m_items;
	RLWndButton m_wProtectSettings;
	RLWndList	m_pcList;
	int m_selectedComputerIndex;
	Panel m_panel;
};

#endif // !defined(AFX_DLGACCESSLIST_H__00904B5E_FA86_45A5_8310_D63A3A082E54__INCLUDED_)
