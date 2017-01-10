// This implementation uses the system region handling routines
// to speed things up and give the best results

#include "stdafx.h"
#include "trRegion.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


TrRegion::TrRegion()
{
	region = NULL;
}

TrRegion::~TrRegion()
{
	Clear();
}

void TrRegion::AddRect(const RECT &new_rect)
{
	// Create a new region containing the appropriate rectangle
	HRGN newregion = ::CreateRectRgnIndirect(&new_rect);

	if (region == NULL)
	{
		// Create the region and set it to contain this rectangle
		region = newregion;
	}
	else
	{
		// Merge it into the existing region
		if (CombineRgn(region, region, newregion, RGN_OR) == NULLREGION)
			Clear();

		// Now delete the temporary region
		::DeleteObject(newregion);
	}
}

/*
void TrRegion::AddRect(RECT R, int xoffset, int yoffset)
{
	R.left += xoffset;
	R.top += yoffset;
	R.right += xoffset;
	R.bottom += yoffset;
	AddRect(R);
}
*/

void TrRegion::SubtractRect(RECT &new_rect)
{
	if (region == NULL) return;

	// Create a new region containing the appropriate rectangle
	HRGN newregion = ::CreateRectRgnIndirect(&new_rect);

	// Remove it from the existing region
	if (::CombineRgn(region, region, newregion, RGN_DIFF) == NULLREGION)
		Clear();

	// Now delete the temporary region
	::DeleteObject(newregion);
}

void TrRegion::Clear()
{
	// Set the region to be empty
	if (region != NULL)
	{
		::DeleteObject(region);
		region = NULL;
	}
}

void TrRegion::Combine(TrRegion &rgn)
{
	if (rgn.region == NULL)
		return;

	if (region == NULL)
	{
		region = ::CreateRectRgn(0, 0, 0, 0);
		if (region == NULL)
			return;

		// Copy the specified region into this one...
		if (::CombineRgn(region, rgn.region, 0, RGN_COPY) == NULLREGION)
			Clear();
		return;
	}
	else {
		// Otherwise, combine the two
		if (::CombineRgn(region, region, rgn.region, RGN_OR) == NULLREGION)
			Clear();
	}
}

void TrRegion::Intersect(TrRegion &rgn)
{
	if (rgn.region == NULL)
		return;
	if (region == NULL)
		return;

	// Otherwise, intersect the two
	if (::CombineRgn(region, region, rgn.region, RGN_AND) == NULLREGION)
		Clear();
}

void TrRegion::Subtract(TrRegion &rgn)
{
	if (rgn.region == NULL)
		return;
	if (region == NULL)
		return;

	// Otherwise, intersect the two
	if (::CombineRgn(region, region, rgn.region, RGN_DIFF) == NULLREGION)
		Clear();
}



// Return all the rectangles
bool TrRegion::Rectangles(rectlist &rects)
{
	// If the region is empty then return empty rectangle list
	if (region == NULL)
		return false;

	// Get the size of buffer required
	int buffsize = ::GetRegionData(region, NULL, 0);
	RGNDATA* buff = (RGNDATA *) new BYTE [buffsize];
	if (buff == NULL)
		return false;

	// Now get the region data
	if (::GetRegionData(region, buffsize, buff))
	{
		for (DWORD x=0; x<(buff->rdh.nCount); x++)
		{
			// Obtain the rectangles from the list
			RECT *rect = (RECT *) (((BYTE *) buff) + sizeof(RGNDATAHEADER) + x * sizeof(RECT));
			rects.push_front(*rect);
		}
	}

	// Delete the temporary buffer
	delete[] buff;

	// Return whether there are any rects!
	return !rects.empty();
}

/*
// Return rectangles clipped to a certain area
bool TrRegion::Rectangles(rectlist &rects, RECT &cliprect)
{
	TrRegion cliprgn;

	// Create the clip-region
	cliprgn.AddRect(cliprect);

	// Calculate the intersection with this region
	cliprgn.Intersect(*this);

	return cliprgn.Rectangles(rects);
}
*/
