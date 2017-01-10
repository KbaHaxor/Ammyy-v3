#if !defined(_TR_DESKTOP_COPY_RECT_H__INCLUDED_)
#define _TR_DESKTOP_COPY_RECT_H__INCLUDED_

#include <map>

class TrDesktopCopyRect
{
public:
	class Item
	{
	public:
		int xs;   // source
		int ys;
		int cx;
		int cy;
		int xoff; // xd -xs;
		int yoff;

		// destanation
		inline int xd() const { return xs + xoff; }
		inline int yd() const { return ys + yoff; }

		/*
		inline void FillRect(RECT& r)
		{
			r.left   = xs;
			r.top    = ys;
			r.right  = xs + cx;
			r.bottom = ys + cy;
		}
		*/

		inline void FillFromRect(const RECT& r)
		{
			xs = r.left;
			ys = r.top;
			cx = r.right  - xs;
			cy = r.bottom - ys;
		}		
	};

	class Vector
	{
	public:
		void Reset() { m_stream.Reset(); }
		int  GetCount();
		void GetItem(int index, Item& item);
		void AddItem(const Item& item);
	private:
		RLStream m_stream;
	};


	TrDesktopCopyRect()  {};
	~TrDesktopCopyRect() {};

	void CheckVerySimple();
	void CheckSimple();
	void CheckComplex();
	void FindWindowsMovement(RECT& screen);
	void CopyRectInBackBuffer(const Item& item);
	
private:
	              void PolishRect(Item& item);
	__forceinline int  TestMovement(const Item& item);
public:
	static BOOL CALLBACK EnumWindowsFn(HWND hwnd, LPARAM arg);
	void AddRect(HWND hwnd, const RECT &rcDest, POINT ptSrc);		

	RECT	m_screen;	// screen rectangle

	struct WindowPos {
		int x;
		int y;
		bool b;
	};

	std::map<HWND, WindowPos> m_windows;

	__forceinline BYTE* GetMainBufferPtr(int x, int y) { return m_mainbuff + (y * m_screenCX + x) * m_bytes_per_pixel;}
	__forceinline BYTE* GetBackBufferPtr(int x, int y) { return m_backbuff + (y * m_screenCX + x) * m_bytes_per_pixel;}

public:
	BYTE	*m_mainbuff;
	BYTE	*m_backbuff;
	int		m_screenCX;
	int		m_screenCY;
	int		m_bytes_per_pixel; // bytes per pixel

	Vector	m_movementsIn;
	Vector	m_movementsOut;
};

#endif // _TR_DESKTOP_COPY_RECT_H__INCLUDED_
