#include "stdafx.h"
#include "Debug2.h"


CDebug2 Debug;

CDebug2::CDebug2() 
{
	//SetScreenRect(1280, 1024);
}

CDebug2::~CDebug2() 
{
	//if (buffer!=NULL) delete buffer;
}


void CDebug2::SetScreenRect(int cx, int cy)
{
	screenRect.left = screenRect.top = 0;
	screenRect.right = cx;
	screenRect.left  = cy;

	if (buffer!=NULL) delete buffer;

	buffer = new char[cx*cy*4];
}


void CDebug2::SaveBMPInternal(FILE* file, void *ptrBm, void *ptrPal, 
			int cx, int cy, int bmstride, int bmclrdepth)
{
	BITMAPINFOHEADER bih = {0};
	bih.biSize			= sizeof(bih);
	bih.biWidth			= cx;
	bih.biHeight		= cy;
	bih.biPlanes		= 1;
	bih.biCompression	= BI_RGB;

	DWORD bitFields[3] = {0, 0, 0};

	if (bmclrdepth == 1)
	{
		bih.biBitCount = 1;
		bih.biClrUsed = 2;
	}
	else if (bmclrdepth == 2)
	{
		bih.biBitCount = 2;
		bih.biClrUsed = 4;
	}
	else if (bmclrdepth == 4)
	{
		bih.biBitCount = 4;
		bih.biClrUsed = 0x10;
	}
	else if (bmclrdepth == 8)
	{
		bih.biBitCount = 8;
		bih.biClrUsed = 0x100;
	}
	else if (bmclrdepth == 16)
	{
		bih.biBitCount = 16;
		bih.biCompression = BI_BITFIELDS;
// TODO: use actual masks
		bitFields[0] = 0xF800;
		bitFields[1] = 0x07E0;
		bitFields[2] = 0x001F;
	}
	else if (bmclrdepth == 24)
	{
		bih.biBitCount = 24;
	}
	else if (bmclrdepth == 32)
	{
		bih.biBitCount = 32;
	}
	else
		throw RLException("SaveBMPInternal()#1");

	BITMAPFILEHEADER bfh = {0};
	bfh.bfType			= 0x4d42;	// 0x42 = "B" 0x4d = "M" 
	bfh.bfOffBits		= sizeof(BITMAPFILEHEADER) + bih.biSize;

	if (bih.biClrUsed)
	{
		bfh.bfOffBits += bih.biClrUsed * sizeof(RGBQUAD);
	}
	else if (bitFields[0] || bitFields[1] || bitFields[2])
	{
		bfh.bfOffBits += sizeof(bitFields);
	}

	unsigned lineSize = (((bih.biWidth * bih.biBitCount) + 15) / 8) & ~1;
	bfh.bfSize = bfh.bfOffBits + lineSize * bih.biHeight; 

	ULONG ulnWr = 0;
	if (fwrite(&bfh, 1, sizeof(bfh), file)!=sizeof(bfh))
		throw RLException("SaveBMPInternal()#2");

	if (fwrite(&bih, 1, sizeof(bih), file)!=sizeof(bih))
		throw RLException("SaveBMPInternal()#3");

	if (ptrPal)
	{
		int size = bih.biClrUsed * sizeof(RGBQUAD);
		if (fwrite(ptrPal, 1, size, file)!=(UINT)size)
			throw RLException("SaveBMPInternal()#4");
	}
	else if (bih.biCompression == BI_BITFIELDS)
	{
		if (fwrite(bitFields, 1, sizeof(bitFields), file)!=sizeof(bitFields))
			throw RLException("SaveBMPInternal()#5");
	}

	for (int i = 0; i < bih.biHeight; i++)
	{
		char *pDWr = (char*)ptrBm + (bih.biHeight - i - 1) * bmstride;

		if (fwrite(pDWr, 1, lineSize, file)!=lineSize)
			throw RLException("SaveBMPInternal()#6");
	}
}


void CDebug2::SaveBMP(LPCSTR fileName, int cx, int cy, void* pData, int bitsPerPixel)
{
	int bytesPerPixel = bitsPerPixel / 8;
	int bmstride = cx * bytesPerPixel;

	FILE* file = fopen(fileName, "wb");
	ASSERT(file!=NULL);

	SaveBMPInternal(file, pData, NULL, cx, cy, bmstride, bitsPerPixel);

	fclose(file);
}


/*
// created for debug purposes
/*
bool	DebugBmDump(void *ptr, int cx, int cy, int bmstride, int bmclrdepth)
{
	if (bmclrdepth!=16 && bmclrdepth!=32)
	{
		// TODO: add 8 bpp
		return false;
	}

	SYSTEMTIME stm;
	GetLocalTime(&stm);
	TCHAR szFileName[MAX_PATH];
	sprintf(
		szFileName,
		"c:\\1\\%04u.%02u.%02u-%02u-%02u-%02u.%03u-0x%08x.bmp",
		stm.wYear, stm.wMonth, stm.wDay,
		stm.wHour, stm.wMinute, stm.wSecond, stm.wMilliseconds,
		ptr);

	FILE* file = fopen(szFileName, "wb");
	if (file==NULL) 
		return false;

	bool b= DebugSaveBitmapToBMPFile(file, ptr, NULL, cx, cy, bmstride, bmclrdepth);

	fclose(file);
	return b;
}
*/

// comment by maxim
// created for debug purposes
/*
void TrDesktop::DebugDumpSurfBuffers(const RECT &rcl, bool main, bool back)
{
	const int c2rect_re_vd_top = rcl.top - m_bmrect.top;
	const int c3rect_re_vd_left = rcl.left - m_bmrect.left;
	ASSERT(c2rect_re_vd_top >= 0);
	ASSERT(c3rect_re_vd_left >= 0);
	const UINT bytesPerPixel = m_frmL.bitsPerPixel / 8;
	const int offset = c2rect_re_vd_top * m_bytesPerRow + c3rect_re_vd_left * bytesPerPixel;

	char szTime[32];
	{
		SYSTEMTIME stm;
		GetLocalTime(&stm);
		sprintf(szTime, "%02hu.%02hu.%02hu-%02hu%02hu%02hu.%03hu", 
			stm.wYear%100, stm.wMonth, stm.wDay,
			stm.wHour, stm.wMinute, stm.wSecond, stm.wMilliseconds
		);
	}

	char szSuffix[32];
	{
		int dx = rcl.right - rcl.left;
		int dy = rcl.bottom - rcl.top;
		sprintf(szSuffix, "%.4d-%.4d - %dx%d.bmp", rcl.left, rcl.top, dx, dy);
	}



	char szFileName1[MAX_PATH];
	char szFileName2[MAX_PATH];
	
	sprintf(szFileName1, "c:\\1\\%s %s main.bmp", szTime, szSuffix);
	sprintf(szFileName2, "c:\\1\\%s %s back.bmp", szTime, szSuffix);

	int cx = rcl.right - rcl.left;
	int cy = rcl.bottom - rcl.top;
	int bmstride   = m_bytesPerRow;
	int bmclrdepth = m_frmL.bitsPerPixel;

	ASSERT(bmclrdepth==32);

	if (main) 
	{
		FILE* file1 = fopen(szFileName1, "wb");
		ASSERT(file1!=NULL);

		bool b= DebugSaveBitmapToBMPFile(file1, m_mainbuff+offset, NULL, cx, cy, bmstride, bmclrdepth);
		ASSERT(b==true);

		fclose(file1);
	}

	if (back)
	{
		FILE* file2 = fopen(szFileName2, "wb");
		ASSERT(file2!=NULL);

		bool b= DebugSaveBitmapToBMPFile(file2, m_backbuff+offset, NULL, cx, cy, bmstride, bmclrdepth);
		ASSERT(b==true);

		fclose(file2);
	}



	
	//bool b1 = DebugBmDump(
	//	m_mainbuff+offset,
	//	rcl.right - rcl.left,
	//	rcl.bottom - rcl.top,
	//	m_bytesPerRow,
	//	m_frmL.bitsPerPixel);

	//bool b2 = DebugBmDump(
	//	m_backbuff+offset,
	//	rcl.right - rcl.left,
	//	rcl.bottom - rcl.top,
	//	m_bytesPerRow,
	//	m_frmL.bitsPerPixel);
	
}
*/


class ObjectSelector 
{
public:
	ObjectSelector(HDC hdc, HGDIOBJ hobj) { m_hdc = hdc; m_hOldObj = SelectObject(hdc, hobj); }
	~ObjectSelector() { m_hOldObj = SelectObject(m_hdc, m_hOldObj); }
private:
	HGDIOBJ m_hOldObj;
	HDC m_hdc;
};


void CDebug2::CaptureScreen(HDC m_hBitmapDC, HBITMAP m_hBitmap, RECT& rect, const char* filename)
{
	HDC winDC	= m_hBitmapDC;

	 // bitmap dimensions
	int bitmap_dx = rect.right - rect.left;
	int bitmap_dy = rect.bottom - rect.top;

	// create file
	FILE* file = fopen(filename, "wb");
	if(file==NULL) return;

	// save bitmap file headers
	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;

	fileHeader.bfType      = 0x4d42;
	fileHeader.bfSize      = 0;
	fileHeader.bfReserved1 = 0;
	fileHeader.bfReserved2 = 0;
	fileHeader.bfOffBits   = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	infoHeader.biSize          = sizeof(infoHeader);
	infoHeader.biWidth         = bitmap_dx;
	infoHeader.biHeight        = bitmap_dy;
	infoHeader.biPlanes        = 1;
	infoHeader.biBitCount      = 24;
	infoHeader.biCompression   = BI_RGB;
	infoHeader.biSizeImage     = 0;
	infoHeader.biXPelsPerMeter = 0;
	infoHeader.biYPelsPerMeter = 0;
	infoHeader.biClrUsed       = 0;
	infoHeader.biClrImportant  = 0;

	fwrite((char*)&fileHeader, 1, sizeof(fileHeader), file);
	fwrite((char*)&infoHeader, 1, sizeof(infoHeader), file);

	// dibsection information
	BITMAPINFO info;
	info.bmiHeader = infoHeader; 

	// create a dibsection and blit the window contents to the bitmap
	// HDC winDC = GetWindowDC(window);

 	// Select and realize hPalette
	ObjectSelector b(winDC, m_hBitmap);


	HDC memDC = CreateCompatibleDC(winDC);
	BYTE* memory = 0;
	HBITMAP bitmap = CreateDIBSection(winDC, &info, DIB_RGB_COLORS, (void**)&memory, 0, 0);
	SelectObject(memDC, bitmap);
	BitBlt(memDC, 0, 0, bitmap_dx, bitmap_dy, winDC, rect.left, rect.top, SRCCOPY);
	DeleteDC(memDC);

	// save dibsection data
	int bytes = (((24*bitmap_dx + 31) & (~31))/8)*bitmap_dy;
	fwrite(memory, 1, bytes, file);
	fclose(file);

	// HA HA, forgot paste in the DeleteObject lol, happy now ;)?
	DeleteObject(bitmap);
}

/*
#ifdef AMMYY_TARGET
void CDebug2::AddRegionToLog(TrRegion& region, LPCSTR prefix)
{
	rectlist rects;
	region.Rectangles(rects);

	int size = rects.size();

	_log.WriteError("%s %d rects",prefix, size);

	rectlist::iterator i = rects.begin();

	for (; i != rects.end(); i++)
	{
		RECT rect = *i;
		_log.WriteError("    %.4d-%.4d %.4dx%.4d", rect.left, rect.top, rect.right-rect.left, rect.bottom - rect.top);
	}			
}
#endif
*/