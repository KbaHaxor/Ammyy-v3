#if !defined(AFX_VRDLGPASSWORDINPUT_H__579D7F4F_9003_4FA4_B30C_B7FE7139E0C9__INCLUDED_)
#define AFX_VRDLGPASSWORDINPUT_H__579D7F4F_9003_4FA4_B30C_B7FE7139E0C9__INCLUDED_

#include "../RL/RLWnd.h"

class VrDlgPasswordInput : public RLDlgBase 
{
public:
	VrDlgPasswordInput();
	virtual ~VrDlgPasswordInput();

	bool	 m_first;
	CStringA m_computer;
	CStringA m_password;

protected:
	virtual BOOL OnInitDialog();
	virtual BOOL OnEndDialog(BOOL ok);
	virtual HBRUSH OnCtlColor(HDC hdc, HWND hwnd);	
	virtual INT_PTR WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	void OnEraseBackground(HDC hdc);

	SIZE  m_size;
	HDC	  m_bkgndDC;
	RLWnd m_wStatic1, m_wndComputerId;
};

#endif // !defined(AFX_VRDLGPASSWORDINPUT_H__579D7F4F_9003_4FA4_B30C_B7FE7139E0C9__INCLUDED_)
