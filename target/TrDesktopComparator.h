#if !defined(_TR_DESKTOP_COMPARATOR_H__INCLUDED_)
#define _TR_DESKTOP_COMPARATOR_H__INCLUDED_

#include "TrRegion.h"
#include "../main/aaDesktop.h"

class TrDesktopComparator
{
public:
	TrDesktopComparator()  {};
	~TrDesktopComparator() {};

	//void DoCompareSimple(TrRegion& region);
	void DoCompare(TrRegion& region);

	int		m_screenCX;
	int		m_screenCY;
	char	*m_chngbuff;	// buffer of changes, true - if pixel was changed
	BYTE	*m_mainbuff;
	BYTE	*m_backbuff;
	aaPixelFormat m_frmL;

private:
	void GetChangedRect(const RECT &rect, RECT& newRect);
	void GetChangedRegion_Normal(TrRegion &rgn, const RECT &rect);	
	void UpdateChangedRect(TrRegion &rgn, const RECT &rect);
	void UpdateChangedSubRect(TrRegion &rgn, const RECT &rect);

	void PollFullScreen(TrRegion& changed_rgn);
	static void CompareBuffers32_asm(void* pSrc, void* pDst, void* pCmp, int pixels);	

	class CRect
	{
	public:
		CRect(RECT rect): r(rect) {}

		inline int x1() { return r.left; }
		inline int y1() { return r.top; }
		inline int x2() { return r.right-1; }
		inline int y2() { return r.bottom-1; }
		inline int cx() { return r.right - r.left; }
		inline int cy() { return r.bottom - r.top; }

		RECT r;
	};

};

#endif // _TR_DESKTOP_COMPARATOR_H__INCLUDED_
