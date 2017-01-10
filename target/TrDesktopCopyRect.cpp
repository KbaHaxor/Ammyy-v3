#include "stdafx.h"
#include "TrDesktopCopyRect.h"
#include "../main/aaProtocol.h"
#include "TrRegion.h"


// Routine to find out which windows have moved
void TrDesktopCopyRect::FindWindowsMovement(RECT& screen)
{
	m_movementsIn.Reset();
	m_screen  = screen;

	for (std::map<HWND,WindowPos>::iterator it=m_windows.begin(); it != m_windows.end();) {
		if (!it->second.b) {
			//_log.WriteError("Removed WINDOW %X", (int)it->first);
			std::map<HWND,WindowPos>::iterator it_delete = it++;
			m_windows.erase(it_delete);
		}
		else {
			it->second.b = false; // clearing, if won't be set, will remove it next time
			it++;
		}
	}

	// Enumerate all the desktop windows for movement
	::EnumWindows((WNDENUMPROC)EnumWindowsFn, (LPARAM)this);
}


// Callback routine used internally to catch window movement...
BOOL TrDesktopCopyRect::EnumWindowsFn(HWND hwnd, LPARAM arg)
{
	//excluding the popup windows - commented by maxim
	//if ((GetWindowLong( hwnd, GWL_STYLE) & WS_POPUP) !=0) return TRUE;

	if (::IsWindowVisible(hwnd))
	{
		RECT dest;		

		// Get the window rectangle
		if (::GetWindowRect(hwnd, &dest)) 
		{
			TrDesktopCopyRect* _this = (TrDesktopCopyRect*)arg;

			std::map<HWND,WindowPos>::iterator it = _this->m_windows.find(hwnd);

			if (it!=_this->m_windows.end()) 
			{
				POINT source;
				source.x = it->second.x;
				source.y = it->second.y;

				// Got the destination position.  Now send to clients!
				if ((source.x != dest.left) || (source.y != dest.top))
				{
					// Update the property entry
					it->second.x = dest.left;
					it->second.y = dest.top;

					// Store of the copyrect
					((TrDesktopCopyRect*)arg)->AddRect(hwnd, dest, source);
				}

				it->second.b = true; // mark that we found window
			}
			else {
				WindowPos windowPos;
				windowPos.x = dest.left;
				windowPos.y = dest.top;
				windowPos.b = true;

				_this->m_windows.insert(std::pair<HWND,WindowPos>(hwnd,windowPos));
			}
		}
	}
	
	return TRUE;
}

void TrDesktopCopyRect::AddRect(HWND hwnd, const RECT &rcDest, POINT ptSrc)
{
	if (ptSrc.x==-32000 && ptSrc.y==-32000) return;				//maxim, it's occur when restoring window
	if (rcDest.left==-32000 && rcDest.top==-32000) return;		//maxim, it's occur when minimizing window
	if (ptSrc.x==rcDest.left && ptSrc.y==rcDest.top) return;	//maxim: didn't move

	// motion vector
	int xoff = rcDest.left - ptSrc.x;
	int yoff = rcDest.top  - ptSrc.y;

	// Clip the destination to the screen
	RECT rcDr2;
	if (!IntersectRect(&rcDr2, &rcDest, &m_screen)) return;

	//_log.WriteInfo("-TrDesktop::CopyRect %.8X %d-%d %d-%d %dx%d",
	//	(UINT)hwnd,
	//	ptSrc.x, ptSrc.y,
	//	rcDest.left, rcDest.top,
	//	rcDest.right - rcDest.left, rcDest.bottom - rcDest.top);

	// Adjust the source correspondingly
	RECT rcSource;
	rcSource.left   = rcDr2.left   - xoff;
	rcSource.right  = rcDr2.right  - xoff;
	rcSource.top    = rcDr2.top    - yoff;
	rcSource.bottom = rcDr2.bottom - yoff;

	// Clip the source to the screen
	RECT rcSr2;
	if (!IntersectRect(&rcSr2, &rcSource, &m_screen)) return;

	// no need to change rcDr2, cause we use cx,cy,xoff,yoff next

	const int cx = rcSr2.right  - rcSr2.left;
	const int cy = rcSr2.bottom - rcSr2.top;

	if (cx >= 16 && cy >= 16)
	{
		Item item;
		item.xs = rcSr2.left - m_screen.left; // adjust to virtual screen with left,top - (0,0)
		item.ys = rcSr2.top  - m_screen.top;
		item.cx = cx;
		item.cy = cy;
		item.xoff = xoff;
		item.yoff = yoff;

		m_movementsIn.AddItem(item);
	}	
}


int TrDesktopCopyRect::Vector::GetCount()
{
	return m_stream.GetLen()/(sizeof(LONG)*6);
}

void TrDesktopCopyRect::Vector::GetItem(int index, Item& item)
{
	m_stream.SetReadPos(index*sizeof(LONG)*6);
	m_stream.GetRaw(&item, sizeof(item));
}

void TrDesktopCopyRect::Vector::AddItem(const Item& item)
{
	m_stream.AddRaw(&item, sizeof(item));
}


void TrDesktopCopyRect::CopyRectInBackBuffer(const Item& item)
{
	int xs = item.xs;
	int ys = item.ys;
	int xd = item.xd();
	int yd = item.yd();
	int cy = item.cy;

	ASSERT(item.xs >= 0);
	ASSERT(item.ys >= 0);
	ASSERT(item.cx >  0);
	ASSERT(item.cy >  0);

	int m_bytesPerRow = m_screenCX * m_bytes_per_pixel;

	BYTE *dstptr = GetBackBufferPtr(xd, yd);
	BYTE *srcptr = GetBackBufferPtr(xs, ys);

	const UINT bytesPerLine = item.cx * m_bytes_per_pixel;

	if (yd < ys)
	{
		for (int y=cy; y>0; y--)
		{
			memcpy(dstptr, srcptr, bytesPerLine);
			srcptr += m_bytesPerRow;
			dstptr += m_bytesPerRow;
		}
	}
	else if (yd > ys)
	{
		srcptr += (m_bytesPerRow * (item.cy - 1));
		dstptr += (m_bytesPerRow * (item.cy - 1));
		for (int y=cy; y>0; y--)
		{
			memcpy(dstptr, srcptr, bytesPerLine);
			srcptr -= m_bytesPerRow;
			dstptr -= m_bytesPerRow;
		}
	}
	else // (yd == ys) worst case, but we can use "memmove" for overlapping memory
	{
		//char* buffer = new char[bytesPerLine];

		for (int y=cy; y>0; y--)
		{
			//memcpy(buffer, srcptr, bytesPerLine);
			//memcpy(dstptr, buffer, bytesPerLine);
			memmove(dstptr, srcptr, bytesPerLine);
			srcptr += m_bytesPerRow;
			dstptr += m_bytesPerRow;
		}

		//delete[] buffer;
	}
}


template<typename T>
class TrDesktopCopyRect2 : public TrDesktopCopyRect
{
public:	
	int TestMovement(int xs, int ys, int cx, int cy, int xd, int yd)
	{
		int v1 = 0; // count the same pixels if no movement
		int v2 = 0; // count the same pixels if was movement

		T *ptr0 = (T*)this->GetMainBufferPtr(xd, yd);
		T *ptr1 = (T*)this->GetBackBufferPtr(xd, yd);
		T *ptr2 = (T*)this->GetBackBufferPtr(xs, ys);

		for (; cy>0; cy--) {
			T* ptr0_c = ptr0;
			T* ptr1_c = ptr1;
			T* ptr2_c = ptr2;

			for (int i=cx; i>0; i--) {
				if (*ptr0_c == *ptr1_c) v1++;
				if (*ptr0_c == *ptr2_c) v2++;

				ptr0_c++;
				ptr1_c++;
				ptr2_c++;
			}

			ptr0 += this->m_screenCX;
			ptr1 += this->m_screenCX;
			ptr2 += this->m_screenCX;
		}

		//_log.WriteError("TestMovement  %u %u %ux%u %d %d === %u - %u", xs, ys, _cx, _cy, xd-xs, yd-ys, v2, v1);

		return (v2-v1);
	}

	
	__forceinline void PolishRect(Item& item)
	{
		int& x1 = item.xs;
		int  x2 = item.xs + item.cx;
		int& y1 = item.ys;
		int  y2 = item.ys + item.cy;
		const int xoff = item.xoff;
		const int yoff = item.yoff;
		const int cx = item.cx;

		// Exclude unchanged lines at the top
		{
			int y = y1;

			for (;y<y2; y++) {
				if (TestMovement(x1, y, cx, 1, x1+xoff, y+yoff)>0) break; // need movement for this line
			}

			y1 = y;
		}
			
		// Exclude unchanged lines at the bottom
		{
			int y = y2-1;

			for (;y>=y1; y--) {
				if (TestMovement(x1, y, cx, 1, x1+xoff, y+yoff)>0) break; // need movement for this line
			}

			y2 = y+1;
		}
		
		int cy  = y2 - y1;
		item.cy = cy;
		if (cy<=0) return;

		// Exclude unchanged lines at the left
		{
			int x = x1;

			for (;x<x2; x++) {
				if (TestMovement(x, y1, 1, cy, x+xoff, y1+yoff)>0) break; // need movement for this line
			}

			x1 = x;
		}	
		
		// Exclude unchanged lines at the right
		{
			int x = x2-1;
		
			for (; x>=x1; x--) {
				if (TestMovement(x, y1, 1, cy, x+xoff, y1+yoff)>0) break; // need movement for this line
			}

			x2 = x+1;
		}

		item.cx = x2 - x1;
	}
};


int TrDesktopCopyRect::TestMovement(const Item& item)
{
	int xs = item.xs;
	int ys = item.ys;
	int cx = item.cx;
	int cy = item.cy;
	int xd = item.xd();
	int yd = item.yd();

	switch(m_bytes_per_pixel) {
		case 1: return ((TrDesktopCopyRect2<UINT8> *)this)->TestMovement(xs, ys, cx, cy, xd, yd);
		case 2: return ((TrDesktopCopyRect2<UINT16>*)this)->TestMovement(xs, ys, cx, cy, xd, yd);
		case 3: return ((TrDesktopCopyRect2<UINT24>*)this)->TestMovement(xs, ys, cx, cy, xd, yd);
		case 4: return ((TrDesktopCopyRect2<UINT32>*)this)->TestMovement(xs, ys, cx, cy, xd, yd);
	}

	throw RLException("TestRect");
}

void TrDesktopCopyRect::PolishRect(Item& item)
{
	//_log.WriteError("Polish#1 %u %u %ux%u", item.xs, item.ys, item.cx, item.cy);

	switch(m_bytes_per_pixel) {
		case 1: ((TrDesktopCopyRect2<UINT8> *)this)->PolishRect(item); break;
		case 2: ((TrDesktopCopyRect2<UINT16>*)this)->PolishRect(item); break;
		case 3: ((TrDesktopCopyRect2<UINT24>*)this)->PolishRect(item); break;
		case 4: ((TrDesktopCopyRect2<UINT32>*)this)->PolishRect(item); break;
		default:
			throw RLException("PolishRect");
	}

	//_log.WriteError("Polish#2 %u %u %ux%u", item.xs, item.ys, item.cx, item.cy);
}


// for v2.13
/*
void TrDesktopCopyRect::CheckVerySimple()
{
	m_movementsOut.m_stream.Reset();

	int countMovements = m_movementsIn.GetCount();
	for (int i=0; i<countMovements; i++) {
		Item item;
		m_movementsIn.GetItem(i, item);
		m_movementsOut.AddItem(dst, item);
		this->CopyRectInBackBuffer(item);
	}
}
*/


// maximp: I don't know why, but it works better than CheckComplex on tests
//
void TrDesktopCopyRect::CheckSimple()
{
	m_movementsOut.Reset();

	int countMovements = m_movementsIn.GetCount();
	for (int i=0; i<countMovements; i++) {
		Item item;
		m_movementsIn.GetItem(i, item);

		PolishRect(item);

		if (item.cx==0 || item.cy==0) continue;

		int v = TestMovement(item);

		if (v>0) 
		{
			m_movementsOut.AddItem(item);
			this->CopyRectInBackBuffer(item);
		}
	}
}

/*
void TrDesktopCopyRect::CheckComplex()
{
	const int BLOCK_SIZE1 = 32;

	m_movementsOut.Reset();

	int countMovements = m_movementsIn.GetCount();
	for (int i=0; i<countMovements; i++) {
		Item item;
		m_movementsIn.GetItem(i, item);

		Item item2 = item;

		//_log.WriteError("Complex#1 %u-%u %u-%u %u-%u", xs, ys, xd, yd, cx1, cy1);

		TrRegion rgn;

		bool previous_added = true;

		int cy = item.cy;

		while (cy>0) {
			item2.cy = min(cy, BLOCK_SIZE1);
			int cx   = item.cx;
			item2.xs = item.xs;
			while (cx>0) {
				item2.cx = min(cx, BLOCK_SIZE1);
		
				int v = TestMovement(item2);
				if (v==0 && previous_added) v=1;
				previous_added = (v>0);

				if (previous_added) {
					rgn.AddRect(item2.xs, item2.ys, item2.cx, item2.cy);
					//_log.WriteError("Complex#1+ %u-%u %ux%u  %d %d", xs, ys, cx, cy, v, cy1);
				}
				else {
					int h = 9;
					//_log.WriteError("Complex#1- %u-%u %ux%u  %d %d", xs, ys, cx, cy, v, cy1);
				}

				      cx -= item2.cx; 
				item2.xs += item2.cx;
			}
			      cy -= item2.cy; 
			item2.ys += item2.cy;
		}

		rectlist rects;
		rgn.Rectangles(rects); // Get list of rectangles for checking

		for (rectlist::iterator i = rects.begin(); i != rects.end(); i++) {
			RECT r = *i;

			//_log.WriteError("Complex#2 %u %u %u %u", r.left, r.top, r.right-r.left, r.bottom - r.top);			

			item2.FillFromRect(r);
			
			PolishRect(item2);

			//_log.WriteError("Complex#3 %u %u %u %u", r.left, r.top, r.right-r.left, r.bottom - r.top);

			if (item2.cx*item2.cy>0) {
				m_movementsOut.AddItem(item2);
				this->CopyRectInBackBuffer(item2);
			}
		}
	}
}
*/