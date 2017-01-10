#if !defined(_TR_ENCODER_HEXTILE_H__INCLUDED)
#define _TR_ENCODER_HEXTILE_H__INCLUDED
#pragma once

#include "TrEncoder.h"

class TrEncoderHexT : public TrEncoder
{
public:
	TrEncoderHexT();
	~TrEncoderHexT();

	virtual const char* GetEncodingName() { return "Hextile"; }

	virtual UINT RequiredBuffSize(UINT width, UINT height);

protected:
	virtual void EncodeRect(const RECT &rect);

	__forceinline static void PUT_PIXEL(UINT8* pix, UINT8* dest, INT32& destoffset)
	{	
		dest[destoffset++] = *pix;
	}

	__forceinline static void PUT_PIXEL(UINT16* pix, UINT8* dest, INT32& destoffset)
	{
		*((UINT16*)(dest+destoffset)) = *pix;
		destoffset += 2;
	}

	__forceinline static void PUT_PIXEL(UINT24* pix, UINT8* dest, INT32& destoffset)
	{
		//*((UINT32*)(dest+destoffset)) = *((UINT32*)pix);

		// this work a little faster that as UINT32, I don't know why..
		*((UINT16*)(dest+destoffset  )) = ((UINT16*)pix)[0];
		*((UINT8 *)(dest+destoffset+2)) = ((UINT8 *)pix)[2];
		destoffset += 3;
	}

	__forceinline static void PUT_PIXEL(UINT32* pix, UINT8* dest, INT32& destoffset)
	{
		*((UINT32*)(dest+destoffset)) = *pix;
		destoffset += 4;
	}

	template <typename PIXEL>
	void EncodeHextiles(const RECT &rect, PIXEL *none)
	{																			
		const int rx = rect.left;
		const int ry = rect.top;
		const int rw = rect.right - rect.left;
		const int rh = rect.bottom - rect.top;

		aaFramebufferUpdateRectHeader surh;
		surh.r.x = rect.left;
		surh.r.y = rect.top;
		surh.r.w = rw;
		surh.r.h = rh;
		surh.encoder = aaEncoderAAFC;

		this->Out(&surh, sz_aaFramebufferUpdateRectHeader);

		BYTE* dest = m_dest.m_buff;
		PIXEL bg = 0;
		PIXEL fg = 0;
		PIXEL clientPixelData[16*16];

		for (int y = ry; y < ry+rh; y += 16)
		{
		for (int x = rx; x < rx+rw; x += 16)
		{
			int w = min(rx+rw-x, 16);
			int h = min(ry+rh-y, 16);
		    																	
			RECT hexrect;
			hexrect.left   = x;
			hexrect.top    = y;
			hexrect.right  = x+w;
			hexrect.bottom = y+h;
			Translate((BYTE *) &clientPixelData, hexrect);

			int destoffset = 1;																				
			int rectoffset = 0;
			dest[rectoffset] = 0;

			PIXEL oldFg = fg;
			PIXEL oldBg = bg;
			bool mono, solid;
			PIXEL newBg, newFg;
																				
			TestColours(clientPixelData, w * h, &mono, &solid, &newBg, &newFg);	
																				
			if (newBg != bg)
			{
				bg = newBg;
				dest[rectoffset] |= aaHextileBackgroundSpecified;
				PUT_PIXEL(&bg, dest, destoffset);
			}																	
																				
			if (!solid) {														
				dest[rectoffset] |= aaHextileAnySubrects;						
																				
				if (mono)														
				{																
					if (newFg != fg)
					{
						fg = newFg;
						dest[rectoffset] |= aaHextileForegroundSpecified;
						PUT_PIXEL(&fg, dest, destoffset);
					}															
				}																
				else															
				{																
					dest[rectoffset] |= aaHextileSubrectsColoured;
				}																
																				
				int encodedbytes = SubrectEncode(clientPixelData, dest+destoffset, w, h, bg, mono);				
				if (encodedbytes == 0)												
				{	
					/* encoding was too large, use raw */
					fg = oldFg;
					bg = oldBg;
					destoffset = rectoffset;
					dest[destoffset++] = aaHextileRaw;

					// clientPixelData was changed, so we need translate again
					Translate((BYTE *) (dest + destoffset), hexrect);
																					
					encodedbytes = w * h * sizeof(PIXEL);
				}																	
				destoffset += encodedbytes;
			}																		
			this->Out(dest, destoffset);
		}																			
		}																			
	}																				


	template <typename PIXEL>
	static UINT SubrectEncode(PIXEL *src, BYTE *dest, int w, int h, PIXEL bg,	bool mono)
	{
		int hx=0,vx=0;
		int numsubs = 0;

		int destoffset = 1;

		for (int y=0; y<h; y++)
		{
			PIXEL* line = src+(y*w);
			for (int x=0; x<w; x++)
			{
				if (line[x] == bg) continue;

				PIXEL cl = line[x];
				int hy = y-1;
				bool hyflag = true;
				for (int j=y; j<h; j++)
				{
					PIXEL* seg = src+(j*w);
					if (seg[x] != cl) {break;}
					int i = x;
					while ((seg[i] == cl) && (i < w)) i++;
					i--;
					if (j == y) vx = hx = i;
					if (i < vx) vx = i;
					if (hyflag && (i >= hx))
						hy++;
					else
						hyflag = false;
				}									
				int vy = j-1;

				/* We now have two possible subrects: (x,y,hx,hy) and
				 * (x,y,vx,vy).  We'll choose the bigger of the two.
				 */	
				int hw = hx-x+1;
				int hh = hy-y+1;
				int vw = vx-x+1;
				int vh = vy-y+1;

				int thew,theh;

				if ((hw*hh) > (vw*vh))
				{
					thew = hw;
					theh = hh;
				}
				else
				{
					thew = vw;
					theh = vh;
				}

				numsubs++;

				if (!mono) PUT_PIXEL(&cl, dest, destoffset);

				dest[destoffset++] = aaHextilePackXY(x,y);
				dest[destoffset++] = aaHextilePackWH(thew,theh);

				if (destoffset > (w * h * sizeof(PIXEL))) return 0;
			
				/*
				 * Now mark the subrect as done.
				 */
				for (int y2=y; y2<(y+theh); y2++) {
				for (int x2=x; x2<(x+thew); x2++) {
					src[y2*w+x2] = bg;
				}
				}
			}
		}

		dest[0] = numsubs;							

		return destoffset;
	}


	template <typename PIXEL>
	static void TestColours(PIXEL *data, int size, bool *mono,	bool *solid, PIXEL *bg, PIXEL *fg)
	{
		PIXEL colour1, colour2;
		int n1 = 0, n2 = 0;
		*mono = true;
		*solid = true;

		for (; size>0; size--, data++)
		{
			if (n1 == 0)
				colour1 = *data;

			if (*data == colour1)
			{
				n1++;
				continue;
			}

			if (n2 == 0)
			{
				*solid = false;
				colour2 = *data;
			}

			if (*data == colour2)
			{
				n2++;
				continue;
			}

			*mono = false;
			break;
		}

		if (n1 > n2)
		{
			*bg = colour1;
			*fg = colour2;
		}
		else
		{
			*bg = colour2;
			*fg = colour1;
		}
	}

private:
	friend class TrEncoderZlibHex;
};

#endif // _TR_ENCODER_HEXTILE_H__INCLUDED

