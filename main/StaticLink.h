#if !defined(AFX_STATICLINK_H__DE16D4B5_8681_401A_9311_EFDC73E24509__INCLUDED_)
#define AFX_STATICLINK_H__DE16D4B5_8681_401A_9311_EFDC73E24509__INCLUDED_

#include "../RL/RLWnd.h"
#include "../RL/FontWTL.h"


class CStaticLink: public RLWndEx
{
public:
	CStaticLink();
	virtual ~CStaticLink();

	HBRUSH OnCtlColor(HDC hdc, HWND hwnd);

private:
	virtual LRESULT WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	void OnLButton();

private:
	COLORREF m_color;
	CFontWTL m_font;
	HCURSOR	 m_cursor;
};

#endif // !defined(AFX_STATICLINK_H__DE16D4B5_8681_401A_9311_EFDC73E24509__INCLUDED_)
