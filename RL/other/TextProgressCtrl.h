#if !defined(AFX_TEXTPROGRESSCTRL_H__4C78DBBE_EFB6_11D1_AB14_203E25000000__INCLUDED_)
#define AFX_TEXTPROGRESSCTRL_H__4C78DBBE_EFB6_11D1_AB14_203E25000000__INCLUDED_

#include "../RL/RLWnd.h"

class CTextProgressCtrl : public RLWndEx
{
public:
	CTextProgressCtrl();
	virtual ~CTextProgressCtrl();	

	void SetPos(int pos);
	void SetWindowText(LPCSTR text);
	void SetRange32(int nMin, int nMax);
	
    COLORREF SetBarColour(COLORREF crBarClr = CLR_DEFAULT);
    COLORREF GetBarColour() const;
    COLORREF SetBgColour(COLORREF crBgClr = CLR_DEFAULT);
    COLORREF GetBgColour() const;

private:
	LRESULT WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT OnSetBarColor(WPARAM, LPARAM Colour);
    LRESULT OnSetBkColor(WPARAM, LPARAM Colour);
	LRESULT OnSetText(UINT, LPCTSTR szText);
	void	OnPaint();

public:
	DWORD	m_dwTextStyle;
	bool	m_useParentFont;

private:
	COLORREF m_crTextClr;
    int      m_nPos, 
             m_nMax, 
             m_nMin;
    CStringA m_strText;
    int      m_nBarWidth;
    COLORREF m_crBarClr;
    COLORREF m_crBgClr;
	HFONT	 m_fontParent;

	bool	m_vertical;
};

#endif // !defined(AFX_TEXTPROGRESSCTRL_H__4C78DBBE_EFB6_11D1_AB14_203E25000000__INCLUDED_)
