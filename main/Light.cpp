#include "stdafx.h"
#include "../RL/RLWnd.h"
#include "Light.h"
#include "Common.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#pragma comment(lib, "comctl32.lib")

/*

//0x00FFFFFF - means background

const unsigned long btn_green[14][6] = { 
0x00FFFFFF,0x00FFFFFF,0x00A4A9A5,0x00878D8B,0x007A8180,0x007A8180,
0x00FFFFFF,0x00838988,0x004DA351,0x006DD161,0x009CE584,0x00ABE990,
0x00A2A7A3,0x004BA74C,0x006ED85E,0x00AEEA91,0x00AEEA91,0x00AEEA91,
0x00818886,0x0035C52D,0x00A7E989,0x00B2EC92,0x00B2EC92,0x00B2EC92,
0x007A8180,0x003DCE2C,0x00ACEB8B,0x00B6EE94,0x00B6EE94,0x00B6EE94,
0x007A8180,0x004AD331,0x0086E266,0x00BBF095,0x00BBF095,0x00BBF095,
0x007A8180,0x0058D936,0x005ADA38,0x0091E76C,0x00B6EF8E,0x00C0F297,
0x007A8180,0x0067DF3C,0x0067DF3C,0x0067DF3C,0x0067DF3C,0x0067DF3C,
0x007A8180,0x0075E441,0x0075E441,0x0075E441,0x0075E441,0x0075E441,
0x007A8180,0x0083EA46,0x0083EA46,0x0083EA46,0x0083EA46,0x0083EA46,
0x00818886,0x008FE94E,0x0090EF4B,0x0090EF4B,0x0090EF4B,0x0090EF4B,
0x00A1A6A2,0x008EC364,0x009CF44F,0x009CF44F,0x009CF44F,0x009CF44F,
0x00FFFFFF,0x00838988,0x0091BF69,0x00A3EE57,0x00A7F953,0x00A7F953,
0x00FFFFFF,0x00FFFFFF,0x00A2A7A3,0x00858C8A,0x007A8180,0x007A8180
};


const unsigned long btn_yellow[14][6] = { 
0x00FFFFFF,0x00FFFFFF,0x00A4A9A5,0x00878D8B,0x007A8180,0x007A8180,
0x00FFFFFF,0x00838988,0x00B5A14A,0x00E6CE59,0x00F2E27E,0x00F2E78B,
0x00A2A7A3,0x00BAA446,0x00EFD557,0x00F3E98D,0x00F3E98D,0x00F3E98D,
0x00818886,0x00E5C028,0x00F3E787,0x00F3EA90,0x00F3EA90,0x00F3EA90,
0x007A8180,0x00EEC92C,0x00F3E98B,0x00F4EC94,0x00F4EC94,0x00F4EC94,
0x007A8180,0x00F0CF37,0x00F2E06A,0x00F5EE97,0x00F5EE97,0x00F5EE97,
0x007A8180,0x00F2D542,0x00F2D644,0x00F4E473,0x00F5EE93,0x00F5F09B,
0x007A8180,0x00F4DC4D,0x00F4DC4D,0x00F4DC4D,0x00F4DC4D,0x00F4DC4D,
0x007A8180,0x00F5E258,0x00F5E258,0x00F5E258,0x00F5E258,0x00F5E258,
0x007A8180,0x00F7E863,0x00F7E863,0x00F7E863,0x00F7E863,0x00F7E863,
0x00818886,0x00F2E86F,0x00F9EE6E,0x00F9EE6E,0x00F9EE6E,0x00F9EE6E,
0x00A1A6A2,0x00C4C37B,0x00FBF378,0x00FBF378,0x00FBF378,0x00FBF378,
0x00FFFFFF,0x00838988,0x00BDBF80,0x00F0ED80,0x00FCF880,0x00FCF880,
0x00FFFFFF,0x00FFFFFF,0x00A2A7A3,0x00858C8A,0x007A8180,0x007A8180
};



const unsigned long btn_red[14][6] = { 
0x00FFFFFF,0x00FFFFFF,0x00A4A9A5,0x00878D8B,0x007A8180,0x007A8180,
0x00FFFFFF,0x00838988,0x00B34D41,0x00E74D2E,0x00F5663D,0x00F66F44,
0x00A2A7A3,0x00B84A3D,0x00F04A29,0x00F77146,0x00F77146,0x00F77146,
0x00818886,0x00E22A15,0x00F66D43,0x00F77348,0x00F77348,0x00F77348,
0x007A8180,0x00EB2C13,0x00F76F45,0x00F8764A,0x00F8764A,0x00F8764A,
0x007A8180,0x00ED331A,0x00F35835,0x00F9784C,0x00F9784C,0x00F9784C,
0x007A8180,0x00F03B20,0x00F03D21,0x00F55E3A,0x00F9754A,0x00FA7B4E,
0x007A8180,0x00F24327,0x00F24327,0x00F24327,0x00F24327,0x00F24327,
0x007A8180,0x00F44A2E,0x00F44A2E,0x00F44A2E,0x00F44A2E,0x00F44A2E,
0x007A8180,0x00F75234,0x00F75234,0x00F75234,0x00F75234,0x00F75234,
0x00818886,0x00F25B3F,0x00F9593B,0x00F9593B,0x00F9593B,0x00F9593B,
0x00A1A6A2,0x00C46E5B,0x00FB6040,0x00FB6040,0x00FB6040,0x00FB6040,
0x00FFFFFF,0x00838988,0x00BD7361,0x00F0694B,0x00FC6645,0x00FC6645,
0x00FFFFFF,0x00FFFFFF,0x00A2A7A3,0x00858C8A,0x007A8180,0x007A8180
};


*/




WndLight::WndLight()
{
	m_state = -1;
}

WndLight::~WndLight()
{
	if (m_imageList!=NULL) {
		::ImageList_Destroy(m_imageList);
	}
}


void WndLight::ChangeBrightness24(byte* pData, int newMaxVal)
{
	pData[0] = (byte)((pData[0]*newMaxVal)/255);	// blue
	pData[1] = (byte)((pData[1]*newMaxVal)/255);	// green
	pData[2] = (byte)((pData[2]*newMaxVal)/255);	// red
}


void WndLight::OnPaint() 
{
	if (m_state < 0)
		return;

	PAINTSTRUCT ps;

	HDC hdc = ::BeginPaint(m_hWnd, &ps);
	
	/*
	{ // alternative way of drawing
	HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, g_hbmBall);

	BITMAP bm;
	GetObject(g_hbmBall, sizeof(bm), &bm);

	BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);

	SelectObject(hdcMem, hbmOld);
    DeleteDC(hdcMem);
	}
	*/
	
	::ImageList_Draw(m_imageList, m_state, hdc, 0, 0, ILD_NORMAL);

	::EndPaint(m_hWnd, &ps);
}


HBITMAP WndLight::CreateBitmapFromPixel(HDC hdc, UINT width, UINT height, UINT uBitsPerPixel, LPVOID pPixels)
{
    if ( !width || !height || !uBitsPerPixel )
		return NULL;

	BITMAPINFO bmpInfo = { 0 };
    bmpInfo.bmiHeader.biBitCount = uBitsPerPixel;
    bmpInfo.bmiHeader.biHeight = height;
    bmpInfo.bmiHeader.biWidth = width;
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	
	LPVOID pBitmapBitmapBits=NULL; 
    HBITMAP hBitmap = CreateDIBSection( hdc, &bmpInfo, DIB_RGB_COLORS, &pBitmapBitmapBits , NULL, 0);

	//HBITMAP hBitmap =::CreateBitmap(width, height, 1, 32, NULL); // if video card 16-bit doesn't work, I don't know why

    if (!hBitmap)
        return NULL;

	LONG lBmpSize = width * height * (uBitsPerPixel/8) ;
    VERIFY(::SetBitmapBits(hBitmap, lBmpSize, pPixels)==lBmpSize);

    return hBitmap;
}


/*
void WndLight::Create2(int imgIndex, int x, int y, int cx, HWND hParentWnd, UINT nID)
{
	COLORREF backgroundColor = ::GetSysColor(COLOR_BTNFACE);

	const unsigned long* imgData = (imgIndex==1) ? &btn_green[0][0] : (imgIndex==2) ? &btn_yellow[0][0] : &btn_red[0][0];

	const int cy = 14;

	int nPixel = cx*cy;

	DWORD* imgNormal = new DWORD[nPixel];
	DWORD* imgDark   = new DWORD[nPixel];

	int i=0;
	for (int y1=0; y1<14; y1++) {
	for (int x1=0; x1<cx; x1++) {
		int x2 = 5;
		if (x1<5) x2 = x1;
		if (cx-x1<5) x2 = cx-x1-1;
		DWORD v = imgData[y1*6+x2];

		if (v==0x00FFFFFF) {
			imgNormal[i] = backgroundColor;
			imgDark[i]   = backgroundColor;
		}
		else {
			imgNormal[i] = v;
			ChangeBrightness24((byte*)&v, 120);
			imgDark[i]   = v;
		}

		i++;
	}
	}
	
	HDC hdcMem = ::CreateCompatibleDC(NULL);

	HBITMAP hBitmapNormal = CreateBitmapFromPixel(hdcMem, cx, cy, 32, imgNormal);
	HBITMAP hBitmapDark   = CreateBitmapFromPixel(hdcMem, cx, cy, 32, imgDark);
	ASSERT( hBitmapNormal!=NULL );
	ASSERT( hBitmapDark  !=NULL );

	::DeleteDC(hdcMem);

	delete[] imgNormal;
	delete[] imgDark;
	

	m_imageList = ::ImageList_Create(cx, cy, ILC_COLORDDB, 2, 0);	//|ILC_MASK
	::ImageList_Add(m_imageList, hBitmapDark,   NULL);
	::ImageList_Add(m_imageList, hBitmapNormal, NULL);
	
	::DeleteObject(hBitmapNormal);
	::DeleteObject(hBitmapDark);

	m_state = 0;

	m_hWnd  = ::CreateWindowW(WC_STATICW, L"", WS_CHILD | WS_VISIBLE, x, y, cx, cy, hParentWnd, (HMENU)nID, NULL, NULL); 

	if (m_hWnd==NULL) 
		throw RLException("ERROR in CreateWindowW()");

	this->SubclassWindow(true);
}
*/

void WndLight::Create3(int x, int y, int cx, int cy, HWND hParentWnd, UINT nID, HBITMAP hBitmap0, HBITMAP hBitmap1)
{	
	m_imageList = ::ImageList_Create(cx, cy, ILC_COLORDDB, 2, 0);
	::ImageList_Add(m_imageList, hBitmap0, NULL);
	::ImageList_Add(m_imageList, hBitmap1, NULL);
	
	m_state = 0;

	m_hWnd  = ::CreateWindowW(WC_STATICW, L"", WS_CHILD | WS_VISIBLE, x, y, cx, cy, hParentWnd, (HMENU)nID, NULL, NULL);

	if (m_hWnd==NULL) 
		throw RLException("ERROR in CreateWindowW()");

	this->SubclassWindow(true);
}


void WndLight::SetState(int state)
{
	if (m_state == state)
		return;

	m_state = state;

	if (m_hWnd!=0) 
		Invalidate(FALSE);
}

LRESULT WndLight::WindowProc(HWND, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg==WM_PAINT) {
		OnPaint();
		return TRUE;
	}

	return RLWndEx::WindowProc(m_hWnd, msg, wParam, lParam);
}
