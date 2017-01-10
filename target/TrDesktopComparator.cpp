#include "stdafx.h"
#include "TrDesktopComparator.h"
#include "../main/aaProtocol.h"


#if 0
// created for troubleshoot purposes;
// the code below is as simple and clear as possible
//
//
void TrDesktopComparator::GetChangedRect(const RECT &rect, RECT& newRect)
{
	const int screenCX = m_screenCX; // may be it we' work faster with local variable instead of class variable
	const int cx = (rect.right - rect.left);

	newRect = rect;

	long& x1 = newRect.left;
	long& x2 = newRect.right;
	long& y1 = newRect.top;
	long& y2 = newRect.bottom;

	// Exclude unchanged lines at the top
	{
		int y = y1;
		char *ptr = m_chngbuff + y * screenCX + x1;

		for (;y<y2; y++)
		{
			if (memchr(ptr, 1, cx) != 0)  break;
			ptr += screenCX;
		}

		y1 = y;
	}
		
	// Exclude unchanged lines at the bottom
	{
		int y = y2-1;
		char *ptr = m_chngbuff + y * screenCX + x1;

		for (;y>=y1; y--)
		{
			if (memchr(ptr, 1, cx) != 0)  break;
			ptr -= screenCX;
		}

		y2 = y+1;
	}
	

	// Exclude unchanged lines at the left
	{
		int x = x1;
		char *ptr = m_chngbuff + y1 * screenCX + x;

		for (;x<x2; x++)
		{
			char *ptr1 = ptr++;
			char *ptr1_end = ptr1 + (y2-y1)*screenCX;

			for (; ptr1<ptr1_end; ptr1 += screenCX)
			{
				if (*ptr1 != 0) goto exit_left;
			}
		}

	exit_left:
		x1 = x;
	}	
	
	// Exclude unchanged lines at the right
	{
		int x = x2-1;
		char *ptr = m_chngbuff + y1 * screenCX + x;
	
		for (; x>=x1; x--)
		{
			char *ptr1 = ptr--;
			char *ptr1_end = ptr1 + (y2-y1)*screenCX;

			for (; ptr1<ptr1_end; ptr1 += screenCX)
			{
				if (*ptr1 != 0) goto exit_right;
			}
		}

	exit_right:
		x2 = x+1;
	}
}

// simple
void TrDesktopComparator::DoCompare(TrRegion& region)
{
	TrRegion changed_rgn;
	this->PollFullScreen(changed_rgn);
			
	rectlist rects;
	changed_rgn.Rectangles(rects); // Get list of rectangles for checking

	for (rectlist::iterator i = rects.begin(); i != rects.end(); i++) {
		RECT newRect;
		GetChangedRect(*i, newRect);
		region.AddRect(newRect);
	}
}

#else

// complex
void TrDesktopComparator::DoCompare(TrRegion &region)
{
	TrRegion changed_rgn;
	this->PollFullScreen(changed_rgn);
			
	rectlist rects;
	changed_rgn.Rectangles(rects); // Get list of rectangles for checking

	for (rectlist::iterator i = rects.begin(); i != rects.end(); i++) {
		// Check changes in the rectangle
		GetChangedRegion_Normal(region, *i);
	}
}

#endif

// last parameter need only for compiler do right job!
template<typename T>
static void CompareBuffers(void* pSrc, void* pDst, void* pCmp, int pixels, T* _t=NULL)
{
	T* src = (T*)pSrc;
	T* dst = (T*)pDst;
	char* cmp = (char*)pCmp;
	char* cmp_end = cmp + pixels;

	// code without "if" a little faster
	while (cmp<cmp_end)
	{
		*cmp++ = *src!=*dst;
		//*dst = *src;
		dst++;
		src++;
	}

	/*
	while (cmp<cmp_end) 
	{
		T val = *src;

		if (*dst==val) {
			*cmd = 0;
		}
		else {
			*cmd = 1;
			*dst = val;
		}

		src++;
		dst++;
		cmd++;
	}
	*/
}


void TrDesktopComparator::PollFullScreen(TrRegion& changed_rgn)
{
	_log2.Print(LL_INF, VTCLOG("PollFullScreen()#1 bits=%u"), (UINT)m_frmL.bitsPerPixel);

	const int BLOCK_SIZE1 = 16;

	// compare new and old buffers, full screen only
	{
		int pixels = m_screenCX * m_screenCY;
		switch (m_frmL.bitsPerPixel) {
			case 32: CompareBuffers<UINT32>(m_mainbuff, m_backbuff, m_chngbuff, pixels); break;
			case 24: CompareBuffers<UINT24>(m_mainbuff, m_backbuff, m_chngbuff, pixels); break;
			case 16: CompareBuffers<UINT16>(m_mainbuff, m_backbuff, m_chngbuff, pixels); break;
			case  8: CompareBuffers<UINT8 >(m_mainbuff, m_backbuff, m_chngbuff, pixels); break;
		}
	}

	_log2.Print(LL_INF, VTCLOG("PollFullScreen()#2"));

	{
		// Align BLOCK_SIZE1 x BLOCK_SIZE1  tiles to the left top corner of the shared area

		for (int y=0; y<m_screenCY; y += BLOCK_SIZE1)
		{
			const int tile_h = min(m_screenCY - y, BLOCK_SIZE1);
			const char *ptr  = m_chngbuff + (y * m_screenCX);

			for (int x = 0; x < m_screenCX; x += BLOCK_SIZE1)
			{
				const int tile_w = min(m_screenCX - x, BLOCK_SIZE1);

				const char *ptr2 = ptr;

				for (int y2=0; y2<tile_h; y2++) {
					if (memchr(ptr2, 1, tile_w) != NULL)
					{
						changed_rgn.AddRect(x, y, tile_w, tile_h);
						break;
					}
					ptr2 += m_screenCX;
				}
				ptr += tile_w;
			}
		}
	}

	_log2.Print(LL_INF, VTCLOG("PollFullScreen()#3"));
}


static const int BLOCK_SIZE2 = 32;



//	maxim: very complex algorithm and it has a bug, while drawing make garbage
void TrDesktopComparator::GetChangedRegion_Normal(TrRegion &rgn, const RECT &rect)
{
	int screenCX = this->m_screenCX; // may be it'll work faster with local variable instead of class variable
	const int cx = (rect.right - rect.left);

	//ASSERT(rect.left >= 0);
	//ASSERT(rect.top >= 0);
	
	char *o_ptr = m_chngbuff + (rect.top * screenCX + rect.left);

	RECT new_rect = rect;

	// Fast processing for small rectangles
	if (rect.right - rect.left <= BLOCK_SIZE2 &&
		rect.bottom - rect.top <= BLOCK_SIZE2)
	{
		for (int y = rect.top; y < rect.bottom; y++)
		{
			if (memchr(o_ptr, 1, cx) != 0) // if we found changes
			{
				new_rect.top = y;
				UpdateChangedSubRect(rgn, new_rect);
				break;
			}
			o_ptr += screenCX;
		}
		return;
	}

	// Process bigger rectangles
	bool bTop4Move = true;
	for (int y = rect.top; y < rect.bottom; y++)
	{
		if (memchr(o_ptr, 1, cx) != 0) // if we found changes
		{
			if (bTop4Move)
			{
				new_rect.top = y;
				bTop4Move = false;
			}
			// Skip a number of lines after a non-matched one
			int n = BLOCK_SIZE2 / 2 - 1;
			y += n;
			o_ptr += n * screenCX;
		}
		else
		{
			if (!bTop4Move)
			{
				new_rect.bottom = y;
				UpdateChangedRect(rgn, new_rect);
				bTop4Move = true;
			}
		}
		o_ptr += screenCX;		
	}
	if (!bTop4Move)
	{
		new_rect.bottom = rect.bottom;
		UpdateChangedRect(rgn, new_rect);
	}
}

void TrDesktopComparator::UpdateChangedRect(TrRegion &rgn, const RECT &rect)
{
	// Pass small rectangles directly to UpdateChangedSubRect
	if (rect.right - rect.left <= BLOCK_SIZE2 &&
		rect.bottom - rect.top <= BLOCK_SIZE2)
	{
		UpdateChangedSubRect(rgn, rect);
		return;
	}

	RECT new_rect = rect;

	//ASSERT(rect.left >= 0);
	//ASSERT(rect.top >= 0);

	// Scan down the rectangle
	char *o_topleft_ptr = m_chngbuff + (rect.top * m_screenCX + rect.left);

	for (int y = rect.top; y < rect.bottom; y += BLOCK_SIZE2)
	{
		// Work out way down the bitmap
		char *o_row_ptr = o_topleft_ptr;

		const int blockbottom = min(y + BLOCK_SIZE2, rect.bottom);
		new_rect.bottom = blockbottom;

		bool bLeft4Move = true;

		for (int x = rect.left; x < rect.right; x += BLOCK_SIZE2)
		{
			// Work our way across the row
			char *o_block_ptr = o_row_ptr;

			const UINT blockright = min(x + BLOCK_SIZE2, rect.right);
			const UINT blockCX = (blockright-x);

			// Scan this block
			int ay;
			for (ay = y; ay < blockbottom; ay++)
			{
				if (memchr(o_block_ptr, 1, blockCX) != 0) // if we found changes
					break;
				o_block_ptr += m_screenCX;
			}
			if (ay < blockbottom)
			{
				// There were changes, so this block will need to be updated
				if (bLeft4Move)
				{
					new_rect.left = x;
					bLeft4Move = false;
					new_rect.top = ay;
				}
				else if (ay < new_rect.top)
				{
					new_rect.top = ay;
				}
			}
			else
			{
				// No changes in this block, process previous changed blocks if any
				if (!bLeft4Move)
				{
					new_rect.right = x;
					UpdateChangedSubRect(rgn, new_rect);
					bLeft4Move = true;
				}
			}

			o_row_ptr += blockCX;
		}

		if (!bLeft4Move)
		{
			new_rect.right = rect.right;
			UpdateChangedSubRect(rgn, new_rect);
		}

		o_topleft_ptr += m_screenCX * BLOCK_SIZE2;
	}
}

void TrDesktopComparator::UpdateChangedSubRect(TrRegion &rgn, const RECT &rect)
{	
	int cx = (rect.right - rect.left);
	int y, i;

	//ASSERT(rect.left >= 0);
	//ASSERT(rect.bottom >= 0);

	// Exclude unchanged lines at the bottom
	char *o_ptr = m_chngbuff + ((rect.bottom - 1) * m_screenCX + rect.left);
	RECT final_rect = rect;
	final_rect.bottom = rect.top + 1;
	for (y = rect.bottom - 1; y > rect.top; y--)
	{
		if (memchr(o_ptr, 1, cx) != 0) // if we found changes
		{
			final_rect.bottom = y + 1;
			break;
		}
		o_ptr -= m_screenCX;
	}

	// Exclude unchanged pixels at left and right sides
	//ASSERT(final_rect.left >= 0);
	//ASSERT(final_rect.top >= 0);

	o_ptr = m_chngbuff + (final_rect.top * m_screenCX + final_rect.left);
	int left_delta = cx - 1;
	int right_delta = 0;
	for (y = final_rect.top; y < final_rect.bottom; y++)
	{
		for (i = 0; i < cx - 1; i++)
		{
			if (o_ptr[i] != 0)
			{
				if (i < left_delta)
					left_delta = i;
				break;
			}
		}
		for (i = cx - 1; i > 0; i--)
		{
			if (o_ptr[i] != 0)
			{
				if (i > right_delta)
					right_delta = i;
				break;
			}
		}
		o_ptr += m_screenCX;
	}
	final_rect.right = final_rect.left + right_delta + 1;
	final_rect.left += left_delta;

	// Update the rectangle
	rgn.AddRect(final_rect);
}


/*
bool operator <(const RECT _X, const RECT _Y)
{
	return _X.top < _Y.top;
}

bool operator >(const RECT _X, const RECT _Y)
{
	return _X.top > _Y.top;
}

bool operator ==(const RECT &_X, const RECT &_Y)
{
	return (_X.left == _Y.left) &&
			(_X.right == _Y.right) &&
			(_X.top == _Y.top) &&
			(_X.bottom == _Y.bottom);
}

bool operator !=(const RECT &_X, const RECT &_Y)
{
	return (_X.left != _Y.left) ||
			(_X.right != _Y.right) ||
			(_X.top != _Y.top) ||
			(_X.bottom != _Y.bottom);
}
*/


