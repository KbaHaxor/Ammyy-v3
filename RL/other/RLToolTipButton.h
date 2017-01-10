#if !defined(_RL_TOOLTIP_BUTTON___INCLUDED_)
#define _RL_TOOLTIP_BUTTON___INCLUDED_

#include "../RLWnd.h"
#include "RLToolTip.h"

/*
class CRLImageList
{
public:
	CRLImageList();
	~CRLImageList();
	
	void Create(int cx, int cy, UINT flags, int cInitial, int cGrow);
	inline operator HIMAGELIST() const { return m_handle; }
	
private:
	void Close();
	
	HIMAGELIST m_handle;
};
*/

class RLToolTipButton : public RLWndEx
{
public:
	RLToolTipButton();
	~RLToolTipButton();
	
	BOOL Create(int x, int y, int cx, int cy, HWND hwndParent, UINT nID);
	
	void SetToolTipTextW(LPCWSTR lpText);
	void SetButtonStyle(UINT nStyle, BOOL bRedraw);
	void DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct);
	void InitImage1(DWORD nResID);
	void InitImage2(HBITMAP hBitmap0, HBITMAP hBitmap1, bool bBitmap0, bool bBitmap1);
	
	LRESULT OnMouseMove(WPARAM WParam, LPARAM LParam);
	LRESULT OnMouseLeave(WPARAM WParam, LPARAM LParam);

	static void MakeBWBitmap(HBITMAP hBitmap);
	
private:
	virtual LRESULT WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	
private:
	//CRLImageList m_imageList;
	HBITMAP		m_hBitmaps[2];
	bool		m_bBitmapsRelease[2];
	RLToolTip   m_ToolTip;
	bool		 m_mouseOver;
};



#endif // !defined(_RL_TOOLTIP_BUTTON___INCLUDED_)
