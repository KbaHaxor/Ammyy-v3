#if !defined(AFX_DEBUG_H__B3B62FAC_3B01_461C_8BF9_40796118E647__INCLUDED_)
#define AFX_DEBUG_H__B3B62FAC_3B01_461C_8BF9_40796118E647__INCLUDED_

#ifdef AMMYY_TARGET
#include "../vtc_target/vtcRegion.h"
#endif

class CDebug2 
{
public:
	CDebug2();
	virtual ~CDebug2();

	static void SaveBMP(LPCSTR fileName, int cx, int cy, void* pData, int bitsPerPixel);

	void SetScreenRect(int cx, int cy);

	static void CaptureScreen(HDC m_hBitmapDC, HBITMAP m_hBitmap, RECT& rect, const char* filename);

#ifdef AMMYY_TARGET
	static void AddRegionToLog(TrRegion& region, LPCSTR prefix);
#endif

private:
	static void SaveBMPInternal(FILE* file, void *ptrBm, void *ptrPal, 
			int cx, int cy, int bmstride, int bmclrdepth);

public:
	char* buffer;
	int  index;
	RECT screenRect;
};

extern CDebug2 Debug;

#endif // !defined(AFX_DEBUG_H__B3B62FAC_3B01_461C_8BF9_40796118E647__INCLUDED_)
