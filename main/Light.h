#if !defined(_WND_LIGHT_H__6C5EFC05_3FB3_46E9_8A27_4A3ABE417C86__INCLUDED_)
#define _WND_LIGHT_H__6C5EFC05_3FB3_46E9_8A27_4A3ABE417C86__INCLUDED_


class WndLight : public RLWndEx
{
public:
	WndLight();
	virtual ~WndLight();
	void SetState(int state);
	void Create2(int imgIndex, int x, int y, int cx, HWND hParentWnd, UINT nID);
	void Create3(int x, int y, int cx, int cy, HWND hParentWnd, UINT nID, HBITMAP hBitmap0, HBITMAP hBitmap1);

	void OnPaint();

private:	
	int		   m_state;
	HIMAGELIST m_imageList;

private:
	static HBITMAP CreateBitmapFromPixel(HDC hdc, UINT width, UINT height, UINT uBitsPerPixel, LPVOID pPixels);
	void ChangeBrightness24(byte* pData, int newMaxVal);
	virtual LRESULT WindowProc(HWND, UINT message, WPARAM wParam, LPARAM lParam);
};


#endif // !defined(_WND_LIGHT_H__6C5EFC05_3FB3_46E9_8A27_4A3ABE417C86__INCLUDED_)
