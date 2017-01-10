// The TrRegion object turns a number of rectangular regions into a list of distinct, non-overlapping regions.

#if !defined(_TR_REGION_H__INCLUDED_)
#define _TR_REGION_H__INCLUDED_

#include <list>
typedef std::list<RECT> rectlist;

class TrRegion
{
public:
	TrRegion();
	~TrRegion();

	void AddRect(const RECT &rect);				// Add another rectangle to the regions	
	void SubtractRect(RECT &rect);				// Subtract a rectangle from the regions
	void Clear();								// Clear the current set of rectangles	
	void Combine  (TrRegion &rgn);				// Combine with another region
	void Intersect(TrRegion &rgn);				// Intersect with another region
	void Subtract (TrRegion &rgn);				// Subtract another region from this one

	// Rectangle retrieval routines - return false if no rects returned!
	// Note that these routines ADD rectangles to existing lists...
	bool Rectangles(rectlist &rects);					// Just return the rects
	//bool Rectangles(rectlist &rects, RECT &cliprect);	// Return all rects within the clip region	

	inline bool IsEmpty() { return region==NULL; }	// Is the region empty?

	inline void AddRect(int x, int y, int cx, int cy)
	{
		RECT r;
		r.left = x;
		r.top  = y;
		r.right  = x+cx;
		r.bottom = y+cy;
		this->AddRect(r);
	}

private:	
	HRGN region;							// Region used internally
};

#endif // _TR_REGION_H__INCLUDED_
