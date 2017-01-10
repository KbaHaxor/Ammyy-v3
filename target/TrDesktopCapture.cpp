#include "stdafx.h"
#include "TrDesktopCapture.h"



TrDesktopCapture::TrDesktopCapture()
{
	m_hrootdc = NULL;
	m_hmemdc  = NULL;
	m_membitmap = NULL;

	for (int i=0; i<2; i++) {
		m_buffers[i].m_ptr = NULL;
		m_buffers[i].m_hBitmap = NULL;
	}
}

void TrDesktopCapture::CaptureFree()
{
	// Now free all the bitmap stuff
	if (m_hrootdc != NULL) {
		// Release our device context
		if(::ReleaseDC(NULL, m_hrootdc) == 0)
			throw RLException("failed to ReleaseDC(m_hrootdc)");
		m_hrootdc = NULL;
	}

	if (m_hmemdc != NULL) {
		// Release our device context
		if (!::DeleteDC(m_hmemdc))
			throw RLException("failed to DeleteDC(m_hmemdc)");
		m_hmemdc = NULL;
	}

	for (int i=0; i<2; i++) {
		if (m_buffers[i].m_hBitmap != NULL) {
			// Release the custom bitmap, if any
			if (!::DeleteObject(m_buffers[i].m_hBitmap)) throw RLException("failed to DeleteObject");
			m_buffers[i].m_hBitmap = NULL;
		}
		if (m_buffers[i].m_ptr != NULL) 
		{
			if (m_membitmap!=NULL) delete [] m_buffers[i].m_ptr; // Slow blits were enabled - free the slow blit buffer
			m_buffers[i].m_ptr = NULL;
		}
	}

	if (m_membitmap!=NULL) {
		::DeleteObject(m_membitmap);
		m_membitmap = NULL;
	}
}

void TrDesktopCapture::GetScreenRect(RECT& rect)
{
	rect.left	= ::GetSystemMetrics(SM_XVIRTUALSCREEN);
	rect.top	= ::GetSystemMetrics(SM_YVIRTUALSCREEN);
	rect.right	= rect.left + ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
	rect.bottom	= rect.top  + ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
}


void TrDesktopCapture::CaptureInitBitmap()
{	
	if (!m_hrootdc) m_hrootdc = ::GetDC(NULL);
	if (!m_hrootdc) 
		throw RLException("GetDC(NULL) ERROR=%d", GetLastError());

	// Create a compatible memory DC
	m_hmemdc = ::CreateCompatibleDC(m_hrootdc);
	if (!m_hmemdc)
		throw RLException("CreateCompatibleDC() ERROR=%d", ::GetLastError());

	// Check that the device capabilities are ok
	if ((::GetDeviceCaps(m_hrootdc, RASTERCAPS) & RC_BITBLT) == 0)
		throw RLException("ERROR: root device doesn't support BitBlt");
	
	if ((::GetDeviceCaps(m_hmemdc, RASTERCAPS) & RC_DI_BITMAP) == 0)	
		throw RLException("ERROR: memory device doesn't support GetDIBits");

	// Create the bitmap to be compatible with the ROOT DC!!!
	m_membitmap = ::CreateCompatibleBitmap(m_hrootdc, m_screenCX, m_screenCY);
	if (!m_membitmap)
		throw RLException("CreateCompatibleBitmap() ERROR=%d", ::GetLastError());

	_log2.Print(LL_INF, VTCLOG("created memory bitmap"));

	// Get the bitmap's format and colour details
	int result;
	m_bminfo.bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_bminfo.bmi.bmiHeader.biBitCount = 0;
	result = ::GetDIBits(m_hmemdc, m_membitmap, 0, 1, NULL, &m_bminfo.bmi, DIB_RGB_COLORS);
	if (result == 0)
		throw RLException("GetDIBits()#1 ERROR=%d", ::GetLastError());

	// TODO: bmiColors filled on second call
	result = ::GetDIBits(m_hmemdc, m_membitmap, 0, 1, NULL, &m_bminfo.bmi, DIB_RGB_COLORS);
	if (result == 0)
		throw RLException("GetDIBits()#2 ERROR=%d", ::GetLastError());

	int planes1 = ::GetDeviceCaps(m_hmemdc, PLANES);
	int planes2 = ::GetDeviceCaps(m_hrootdc, PLANES);
	
	_log2.Print(LL_INF, VTCLOG("DBG:memory context has %d planes!"),  planes1);
	_log2.Print(LL_INF, VTCLOG("DBG:display context has %d planes!"), planes2);
	if (planes1 != 1)
		throw RLException("ERROR: current display is PLANAR, not CHUNKY");

	// Henceforth we want to use a top-down scanning representation
	m_bminfo.bmi.bmiHeader.biHeight = - abs(m_bminfo.bmi.bmiHeader.biHeight);

	// Is the bitmap palette-based or truecolour?
	m_bminfo.truecolour = (::GetDeviceCaps(m_hmemdc, RASTERCAPS) & RC_PALETTE) == 0;
}

void TrDesktopCapture::CaptureThunkBitmapInfo()
{
	// If we leave the pixel format intact, the blits can be optimised (DIBsection patch)
	m_formatmunged = false;

	// Attempt to force the actual format into one we can handle
	// We can handle 8-bit-palette and 16/32-bit-truecolour modes
	switch (m_bminfo.bmi.bmiHeader.biBitCount)
	{
		/*
	case 1:
	case 4:		
		_log2.Print(LL_INF, VTCLOG("DBG:used/bits/planes/comp/size = %d/%d/%d/%d/%d"),
					 (int)m_bminfo.bmi.bmiHeader.biClrUsed,
					 (int)m_bminfo.bmi.bmiHeader.biBitCount,
					 (int)m_bminfo.bmi.bmiHeader.biPlanes,
					 (int)m_bminfo.bmi.bmiHeader.biCompression,
					 (int)m_bminfo.bmi.bmiHeader.biSizeImage);
		
		// Correct the BITMAPINFO header to the format we actually want
		m_bminfo.bmi.bmiHeader.biClrUsed = 0;
		m_bminfo.bmi.bmiHeader.biPlanes = 1;
		m_bminfo.bmi.bmiHeader.biCompression = BI_RGB;
		m_bminfo.bmi.bmiHeader.biBitCount = 8;
		m_bminfo.bmi.bmiHeader.biSizeImage =
			abs((m_bminfo.bmi.bmiHeader.biWidth * m_bminfo.bmi.bmiHeader.biHeight * m_bminfo.bmi.bmiHeader.biBitCount)/ 8);
		m_bminfo.bmi.bmiHeader.biClrImportant = 0;
		m_bminfo.truecolour = false;

		m_formatmunged = true; // Display format is non-VNC compatible - use the slow blit method
		break;
		*/
	
	case 1:
	case 4:
	case 8:
	//case 24:
		// Update the bitmapinfo header
		m_bminfo.truecolour = true;
		m_bminfo.bmi.bmiHeader.biBitCount = 32;
		m_bminfo.bmi.bmiHeader.biPlanes = 1;
		m_bminfo.bmi.bmiHeader.biCompression = BI_RGB;
		m_bminfo.bmi.bmiHeader.biSizeImage = 
			abs((m_bminfo.bmi.bmiHeader.biWidth * m_bminfo.bmi.bmiHeader.biHeight * m_bminfo.bmi.bmiHeader.biBitCount)/ 8);

		m_formatmunged = true; // Display format is non-VNC compatible - use the slow blit method
		break;
	}
}


void TrDesktopCapture::CaptureSetPixFormat()
{
	// Examine the bitmapinfo structure to obtain the current pixel format
	m_frmL.type=0; // = m_bminfo.truecolour;

	// Set up the native buffer format
	m_frmL.bitsPerPixel = (UINT8) m_bminfo.bmi.bmiHeader.biBitCount;
	
	// Calculate the number of bytes per row
	m_bytesPerRow = m_screenCX * m_frmL.bitsPerPixel / 8;


	// Sort out the colour shifts, etc.
	DWORD maskR=0, maskB=0, maskG = 0;

	switch (m_bminfo.bmi.bmiHeader.biBitCount)
	{
	case 16:
		if (m_bminfo.bmi.bmiHeader.biCompression == BI_RGB)
		{
		// Standard 16-bit display, each word single pixel 5-5-5
			maskR = 0x7c00;
			maskG = 0x03e0;
			maskB = 0x001f;
		}
		else if (m_bminfo.bmi.bmiHeader.biCompression == BI_BITFIELDS)
		{
			maskR = *(DWORD *)&m_bminfo.bmi.bmiColors[0];
			maskG = *(DWORD *)&m_bminfo.bmi.bmiColors[1];
			maskB = *(DWORD *)&m_bminfo.bmi.bmiColors[2];
		}
		
		break;

	case 32:
		// Standard 24/32 bit displays
		if (m_bminfo.bmi.bmiHeader.biCompression == BI_RGB)
		{
			maskR = 0xff0000;
			maskG = 0x00ff00;
			maskB = 0x0000ff;
			// The real color depth is 24 bits in this case.
		}
		else if (m_bminfo.bmi.bmiHeader.biCompression == BI_BITFIELDS)
		{
			maskR = *(DWORD *)&m_bminfo.bmi.bmiColors[0];
			maskG = *(DWORD *)&m_bminfo.bmi.bmiColors[1];
			maskB = *(DWORD *)&m_bminfo.bmi.bmiColors[2];		
		}
		break;
	}

	// Other pixel formats are only valid if they're palette-based
	if (maskR==0 && maskG==0 && maskB==0)
		throw RLException("unsupported truecolour pixel format for SetPixFormat()");

	// Convert the data we just retrieved
	MaskToMaxAndShift(maskR,	m_frmL.rMax,	m_frmL.rShift);
	MaskToMaxAndShift(maskG,	m_frmL.gMax,	m_frmL.gShift);
	MaskToMaxAndShift(maskB,	m_frmL.bMax,	m_frmL.bShift);

	_log2.Print(LL_INF, VTCLOG("DBG:true-color desktop in SetPixFormat()"));
}

inline void TrDesktopCapture::MaskToMaxAndShift(DWORD mask, UINT16 &max, UINT8 &shift)
{
	for (shift = 0; (mask & 1) == 0; shift++)
		mask >>= 1;
	max = (UINT16) mask;
}

BOOL TrDesktopCapture::SetPalette()
{
	if (m_bminfo.truecolour)
	{
		// Not a palette based local screen - forget it!
		_log2.Print(LL_INF, VTCLOG("no palette data for truecolour display"));
		return TRUE;
	}

	// Lock the current display palette into the memory DC we're holding
	// *** CHECK THIS FOR LEAKS!

	UINT size = sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * 256);

	RLStream paletteWrapper(size);

	LOGPALETTE* palette = (LOGPALETTE *)paletteWrapper.GetBuffer();
	if (palette == NULL) {
		_log2.Print(LL_ERR, VTCLOG("error allocating palette"));
		return FALSE;
	}

	// Initialise the structure
	palette->palVersion = 0x300;
	palette->palNumEntries = 256;

	// Get the system colours
	if (::GetSystemPaletteEntries(m_hrootdc, 0, 256, palette->palPalEntry) == 0)
	{
		_log2.Print(LL_ERR, VTCLOG("GetSystemPaletteEntries() failed."));
		return FALSE;
	}

	// Create a palette from those
	HPALETTE pal = ::CreatePalette(palette);
	if (pal == NULL)
	{
		_log2.Print(LL_ERR, VTCLOG("CreatePalette() failed."));
		return FALSE;
	}

	// Select the palette into our memory DC
	HPALETTE oldpalette = ::SelectPalette(m_hmemdc, pal, FALSE);
	if (oldpalette == NULL)
	{
		_log2.Print(LL_ERR, VTCLOG("SelectPalette() failed."));
		::DeleteObject(pal);
		return FALSE;
	}

	// Worked, so realise the palette
	if (RealizePalette(m_hmemdc) == GDI_ERROR)
		_log2.Print(LL_WRN, VTCLOG("warning - failed to RealizePalette"));

	// It worked!
	::DeleteObject(oldpalette);
	_log2.Print(LL_INF, VTCLOG("initialised palette OK"));
	return TRUE;
}


void TrDesktopCapture::CaptureInitPixelFormatOnly()
{
	m_screenX  = 0;
	m_screenY  = 0;
	m_screenCX = 1;
	m_screenCY = 1;

	CaptureInitBitmap();
	CaptureThunkBitmapInfo();
	CaptureSetPixFormat();
	CaptureFree();
}

void TrDesktopCapture::CaptureInit()
{
	m_frameFlipper = false; // doesn't matter what buffer is first

	{
		RECT rect;
		GetScreenRect(rect);

		m_screenX  = rect.left;
		m_screenY  = rect.top;
		m_screenCX = rect.right - rect.left;
		m_screenCY = rect.bottom - rect.top;

		_log2.Print(LL_INF, VTCLOG("source desktop metrics: (%d, %d), %d x %d"), m_screenX,  m_screenY, m_screenCX, m_screenCY);
	}

	CaptureInitBitmap();
	CaptureThunkBitmapInfo();
	CaptureSetPixFormat();

	// Create a new DIB section ***
	HBITMAP tempbitmap = NULL;
	if (!m_formatmunged && m_bminfo.truecolour) // Optimised blits don't work with palette-based displays, yet
	{
		tempbitmap = ::CreateDIBSection(m_hmemdc, &m_bminfo.bmi, DIB_RGB_COLORS, &m_buffers[0].m_ptr, NULL, 0);
		if (tempbitmap == NULL)
			_log2.Print(LL_WRN, VTCLOG("failed to build DIB section - reverting to slow blits"));
	}

	// NOTE allocation can not be supressed even with direct access mirror surface view

	if (tempbitmap == NULL)
	{
		::SelectObject(m_hmemdc, m_membitmap);

		// create our own buffer to copy blits through
		int screenBuffSize = m_frmL.bitsPerPixel/8 * m_screenCX * m_screenCY;
		for (int i=0; i<2; i++) {
			m_buffers[i].m_hBitmap = NULL;
			if ((m_buffers[i].m_ptr = new BYTE [screenBuffSize]) == NULL)
				throw RLException("unable to allocate main buffer[%d]", screenBuffSize);
		}
	}
	else {
		m_buffers[0].m_hBitmap = tempbitmap;

		// Delete the old memory bitmap
		if (m_membitmap != NULL) {
			::DeleteObject(m_membitmap);
			m_membitmap = NULL;
		}

		// Create second frame
		m_buffers[1].m_hBitmap = ::CreateDIBSection(m_hmemdc, &m_bminfo.bmi, DIB_RGB_COLORS, &m_buffers[1].m_ptr, NULL, 0);
		if (m_buffers[1].m_hBitmap == NULL)
			throw RLException("failed to build DIB section");

		_log2.Print(LL_INF, VTCLOG("enabled fast DIBsection blits OK"));
	}
	
	/* test for multimonitors with uncovered area
	int size = m_screenCX * m_screenCY * 4;
	BYTE* ptr0 = (BYTE*)m_buffers[0].m_ptr;
	BYTE* ptr1 = (BYTE*)m_buffers[1].m_ptr;
	for (int k=0; k<size; k++)
	{
		*ptr0++ = 200;
		*ptr1++ = 100;
	}
	*/
}


void TrDesktopCapture::CaptureBitmap(HBITMAP hBitmap, int cx, int cy, RLStream& buffer)
{
	int size = cx * cy *( m_frmL.bitsPerPixel / 8);
	void* ptr = buffer.GetBuffer1(size);

	_BMInfo bminfo = m_bminfo;

	bminfo.bmi.bmiHeader.biSizeImage = size;
	bminfo.bmi.bmiHeader.biWidth  =  cx;
	bminfo.bmi.bmiHeader.biHeight = -cy;

	if (::GetDIBits(m_hmemdc, hBitmap, 0, cy, ptr, &bminfo.bmi, DIB_RGB_COLORS) == 0)
		throw RLException("ERROR CaptureBitmap() error=%d", ::GetLastError());
}



// Function to capture full screen immediately prior to sending an update, and save data to m_mainbuff
//
void TrDesktopCapture::CaptureFullScreen()
{
	m_frameFlipper = !m_frameFlipper;

	int iFrame = (m_frameFlipper) ? 0 : 1;


	// ASSUME rect related to virtual desktop

	_log2.Print(LL_INF, VTCLOG("CaptureScreen()#1"));

	// Finish drawing anything in this thread 
	// Wish we could do this for the whole system - maybe we should
	// do something with LockWindowUpdate here.
	::GdiFlush();

	if (m_membitmap == NULL) {
		// Select the memory bitmap into the memory DC
		HBITMAP oldbitmap = (HBITMAP)::SelectObject(m_hmemdc, m_buffers[iFrame].m_hBitmap);
		if (oldbitmap == NULL || oldbitmap==(HBITMAP)GDI_ERROR)
			throw RLException("ERROR#1 in CaptureFullScreen()");
	}

	// Capture screen into bitmap

	DWORD dwRop = (settings.m_captureHints) ? SRCCOPY+CAPTUREBLT : SRCCOPY;

	// source in m_hrootdc is relative to a virtual desktop,
	// whereas dst coordinates of m_hmemdc are relative to its top-left corner (0, 0)	
	if (::BitBlt(m_hmemdc, 0, 0, m_screenCX, m_screenCY, m_hrootdc, m_screenX, m_screenY, dwRop)==FALSE) {
		throw RLException("ERROR#2 in CaptureFullScreen() %u", ::GetLastError());
	}

	//double t1 = timer1.GetElapsedSeconds();
	//timer1.Start();
	

	// If fast (DIBsection) blits are disabled then use the old GetDIBits technique
	if (m_membitmap != NULL) 
	{
		// Calculate the scanline-ordered y position to copy from
		// NB: m_membitmap is bottom2top
		const int y_inv_re_vd = - m_screenY;
		ASSERT(y_inv_re_vd >= 0);

		// Set the number of bytes for GetDIBits to actually write
		// NOTE : GetDIBits pads the destination buffer if biSizeImage < no. of bytes required
		m_bminfo.bmi.bmiHeader.biSizeImage = m_screenCY * m_bytesPerRow;

		// Get the actual bits from the bitmap into the bit buffer
		// If fast (DIBsection) blits are disabled then use the old GetDIBits technique
		if (::GetDIBits(m_hmemdc, m_membitmap, y_inv_re_vd, m_screenCY, m_buffers[iFrame].m_ptr, &m_bminfo.bmi, DIB_RGB_COLORS) == 0)
			throw RLException("ERROR#1 GetDIBits() error=%d, y=%d, cy=%d", ::GetLastError(), y_inv_re_vd, m_screenCY);
	}

	//double t2 = timer1.GetElapsedSeconds();
	//_log.WriteError("CaptureFullScreen() %u %X %.6f %.6f", (int)blitok, m_buffers[iFrame].m_ptr, t1, t2);

	_log2.Print(LL_INF, VTCLOG("CaptureScreen()#5"));

	// save to bmp
	//static int index = 0;
	//CStringA path;
	//path.Format("c:\\1\\%.4d.bmp", ++index);
	//CDebug2::SaveBMP(path, m_screenCX, m_screenCY, m_buffers[iFrame].m_ptr, m_bminfo.bmi.bmiHeader.biBitCount);
}
