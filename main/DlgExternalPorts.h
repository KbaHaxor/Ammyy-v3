#if !defined(_DLG_DIRECTIN_H__3503FA97__INCLUDED_)
#define _DLG_DIRECTIN_H__3503FA97__INCLUDED_

#include "../RL/RLWnd.h"

class DlgExternalPorts : public RLDlgBase 
{
public:
	DlgExternalPorts();
	virtual ~DlgExternalPorts();

protected:
	virtual INT_PTR WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnInitDialog();
	virtual BOOL OnEndDialog(BOOL ok);

private:
	BOOL   OnListItemChanged(NMLISTVIEW* lParam);
	void   OnListChangedSelected(int index);
	UINT16 GetPort(int index);
	bool   GetState(int index);
	void   SetState(int index, bool state);
	void   SetState(int index, LPCSTR state);
	int    GetSelectedItem();
	void OnBtnCheck();
	void OnBtnAdd();
	void OnBtnRemove();
	void OnActivated();
	void FillListView();
	int  InsertRow(int index, UINT16 port, bool state);	

	RLWndList m_list;
	bool	   m_fullTest;

};

#endif // !defined(_DLG_DIRECTIN_H__3503FA97__INCLUDED_)
