#if !defined(_TR_DESKTOP_CAPTURE_H__INCLUDED_)
#define _TR_DESKTOP_CAPTURE_H__INCLUDED_

#include "../main/aaDesktop.h"

class TrDesktopCapture
{
public:
	TrDesktopCapture();

	inline BYTE* GetBuffer(bool main) 
	{ 
		bool b = (main) ? m_frameFlipper : !m_frameFlipper;
		return (BYTE*)m_buffers[(b)?0:1].m_ptr;
	}

	BOOL SetPalette();

	static void GetScreenRect(RECT& rect);

public:
	void CaptureFullScreen();
	void CaptureInit();
	void CaptureInitPixelFormatOnly();
	void CaptureFree();
	void CaptureBitmap(HBITMAP hBitmap, int cx, int cy, RLStream& buffer);

private:
	void CaptureInitBitmap();
	void CaptureThunkBitmapInfo();
	void CaptureSetPixFormat();

	// Convert a bit mask eg. 00111000 to max=7, shift=3
	static void MaskToMaxAndShift(DWORD mask, UINT16 &max, UINT8 &shift);

public:
	int				m_screenX, m_screenY;
	int				m_screenCX, m_screenCY;
	aaPixelFormat	m_frmL;		// local pixel format

private:
	int				m_bytesPerRow;	// This is always handy to have	

	HDC				m_hmemdc;	// device contexts for memory
	HDC				m_hrootdc;	// device contexts for screen

	struct _BMInfo {
		BOOL			truecolour;
		BITMAPINFO		bmi;
		RGBQUAD			cmap[256]; // Colormap info - comes straight after BITMAPINFO - **HACK**
	} m_bminfo;

	struct SBuffer {
		VOID*	m_ptr;
		HBITMAP m_hBitmap;
	} m_buffers[2];

	HBITMAP		m_membitmap; // DDB bitmap
	bool		m_formatmunged;
	bool		m_frameFlipper;

	friend class TrEncoder;
};

#endif // _TR_DESKTOP_CAPTURE_H__INCLUDED_
