#ifndef _VR_OPTIONS_H__INCLUDED_
#define _VR_OPTIONS_H__INCLUDED_

#include "../RL/RLWnd.h"
#include "../main/aaProtocol.h"
#include "../main/DlgPermissionList.h"

class VrOptions : public RLDlgBase 
{
public:
	VrOptions();
	VrOptions& operator=(VrOptions& s);
	virtual ~VrOptions();

	void	SetScaling(int num, int den);
	
	// default connection options - can be set through Dialog
	bool    m_allowRemoteControl;
	bool    m_allowClipboardOut;
	bool    m_allowClipboardIn;
	bool	m_FullScreen;		 // FullScreen Mode or Window Mode
	bool	m_autoScrollWndMode; // auto-scroll for window mode
	bool	m_scaling;
	bool	m_FitWindow;
	int		m_scale_num, m_scale_den; // Numerator & denominator
	UINT8   m_cursorLocal;
	UINT8   m_cursorRemoteRequest;

	CStringA m_connectionInfo;
	Permission m_prm;

protected:
	BOOL    OnInitDialog();
	BOOL    OnEndDialog(BOOL ok);
	INT_PTR WindowProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	static void FillComboBox(RLWndComboBox& wnd, int key);
	static void Lim(HWND hwnd,int control,DWORD min, DWORD max);

	void OnOK1();
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	void OnBtnSave();
	void OnBtnLoad();
	void Load();
	void ApplyScale(UINT16 scale);
	void FillSavedItems();

	void EnableJpeg(bool enable);

	bool IsChecked(int nIDDlgItem);

	DlgPermissionList::Panel m_permPanel;

	RLWndComboBox	m_wndRemoteCursor;
	RLWndComboBox	m_wndLocalCursor;
};

#endif // _VR_OPTIONS_H__INCLUDED_

