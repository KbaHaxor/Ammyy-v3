#if !defined(_DLG_SPEED_TEST_H__3567FA97__INCLUDED_)
#define _DLG_SPEED_TEST_H__3567FA97__INCLUDED_

#include "../RL/RLWnd.h"
#include "../main/Transport.h"

class VrDlgSpeedTest : public RLDlgBase 
{
public:
	VrDlgSpeedTest();
	virtual ~VrDlgSpeedTest();

	int DoModal(HWND hWndParent=NULL)
	{
		int v = RLDlgBase::DoModal(hWndParent);
		m_hWnd = 0;
		return v;
	}
	
protected:
	virtual HBRUSH  OnCtlColor(HDC hdc, HWND hwnd);
	virtual INT_PTR WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnInitDialog();
	//virtual BOOL OnEndDialog(BOOL ok);

public:
	Transport* m_transport;
	
private:
	void OnBtnClear();
	void OnBtnStart();
	void DoSpeedTest(double test_time);
	void AddText(CStringA text);
	CStringA GetSpeedAsText(UINT64 bytes, double time);

	RLWnd m_wStatus, m_wText, m_wTestTime;

	CStringA m_text;
};

#endif // !defined(_DLG_SPEED_TEST_H__3567FA97__INCLUDED_)
