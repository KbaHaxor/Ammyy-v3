#if !defined(_ADDCOMPUTER_H__62753545_E82F_4364_AE09_C310F99F089E__INCLUDED_)
#define _ADDCOMPUTER_H__62753545_E82F_4364_AE09_C310F99F089E__INCLUDED_

#include "../RL/RLWnd.h"
#include "Settings.h"

class DlgOperatorPermissions : public RLDlgBase  
{
public:
	DlgOperatorPermissions(bool edit);
	virtual ~DlgOperatorPermissions();

	Permission m_permission;

protected:
	BOOL OnInitDialog();
	BOOL OnEndDialog(BOOL ok);
	INT_PTR WindowProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	void InitPasswords();
	void OnPasswordReset();

	RLWndComboBox	m_wComputerId;
	RLWnd			m_wPassword1, m_wPassword2;
	
	bool			m_blockPassword;
};

#endif // !defined(_ADDCOMPUTER_H__62753545_E82F_4364_AE09_C310F99F089E__INCLUDED_)
