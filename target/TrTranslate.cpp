#include "stdafx.h"
#include "TrTranslate.h"


//predefined format
aaPixelFormat TrTranslator::f_x8r8g8b8(32, 0, 0xFF, 0xFF, 0xFF, 16, 8, 0);
aaPixelFormat TrTranslator::f_r8g8b8  (24, 0, 0xFF, 0xFF, 0xFF, 16, 8, 0);
aaPixelFormat TrTranslator::f_x1r5g5b5(16, 0, 0x1F, 0x1F, 0x1F, 10, 5, 0);
aaPixelFormat TrTranslator::f_r5g6b5  (16, 0, 0x1F, 0x3F, 0x1F, 11, 5, 0);
aaPixelFormat TrTranslator::f_r3g3b2  (8,  0, 0x7,  0x7,  0x3,   5, 2, 0);
aaPixelFormat TrTranslator::f_y8      (8,  aaGrayScale, 0, 0, 0, 0, 0, 0);


class TrTranslatorNone : public TrTranslator
{
public:
	TrTranslatorNone(UINT pixelSize)
	{
		m_needToDelete = true;
		m_pixelSize = pixelSize;
	}

	virtual void Translate(void* _dst, const void* _src, int nPixels)
	{
		memcpy(_dst, _src, nPixels * m_pixelSize);
	}

protected:
	int m_pixelSize; // in bytes
};


//-----------------------------------------------------------------
// general converter supprorts masks of channels that cross bytes boundary
//-----------------------------------------------------------------
// -8Kb of exe size
class TrTranslatorGeneralBase : public TrTranslator
{
public:
	virtual ~TrTranslatorGeneralBase(){}

	TrTranslatorGeneralBase(const aaPixelFormat& _fSrc, const aaPixelFormat& _fDst)
		: fSrc(_fSrc), fDst(_fDst)
	{
		m_needToDelete = true;

		rDiff = GetNumBits(fSrc.rMax) - GetNumBits(fDst.rMax);
		gDiff = GetNumBits(fSrc.gMax) - GetNumBits(fDst.gMax);
		bDiff = GetNumBits(fSrc.bMax) - GetNumBits(fDst.bMax);

		// build table for fast convertion
		for (UINT i=0; i<256; i++)
		{
			UINT c;
			c = (rDiff) ? i*fDst.rMax/fSrc.rMax : rDiff;   m_table[0][i] = c << fDst.rShift;
			c = (gDiff) ? i*fDst.gMax/fSrc.gMax : gDiff;   m_table[1][i] = c << fDst.gShift;
			c = (bDiff) ? i*fDst.bMax/fSrc.bMax : bDiff;   m_table[2][i] = c << fDst.bShift;
		}

		rMask = (UINT32)fSrc.rMax << fSrc.rShift;
		gMask = (UINT32)fSrc.gMax << fSrc.gShift;
		bMask = (UINT32)fSrc.bMax << fSrc.bShift;
		rShift = rDiff+fSrc.rShift;
		gShift = gDiff+fSrc.gShift;
		bShift = bDiff+fSrc.bShift;
	}
protected:
	aaPixelFormat fSrc, fDst;
	UINT m_table[3][256];		// if type "TD" should be 257 for UINT24
	char rDiff, gDiff, bDiff;
	UINT32 rMask, gMask, bMask;
	char rShift, gShift, bShift;
};




//-----------------------------------------------------------------
// general converter supprorts masks of channels that cross bytes boundary
//-----------------------------------------------------------------
template<int table, typename TS, typename TD>
class TrTranslatorGeneral : public TrTranslatorGeneralBase
{
public:
	TrTranslatorGeneral(const aaPixelFormat& _fSrc, const aaPixelFormat& _fDst)
		: TrTranslatorGeneralBase(_fSrc, _fDst)
	{
	}

	virtual void Translate(void* _dst, const void* _src, int nPixels)
	{
		//do not read and write last pixel UINT24 as UINT32 due to access violation
		if(sizeof(TS)==3 || sizeof(TD)==3) nPixels--;

		const TS* src = (const TS*)_src;
		const TS* end = src + nPixels;
		      TD* dst =       (TD*)_dst;

		while(src<end)
		{
			UINT s = (sizeof(TS)==3) ? *((UINT32*)src) : *src;
			UINT res;
			if (table==0)
			{
				res = (((s & rMask) >> rShift) << fDst.rShift)
				    | (((s & gMask) >> gShift) << fDst.gShift)
				    | (((s & bMask) >> bShift) << fDst.bShift);
			}
			else
			{
				UINT r = (s >> fSrc.rShift) & fSrc.rMax;
				UINT g = (s >> fSrc.gShift) & fSrc.gMax;
				UINT b = (s >> fSrc.bShift) & fSrc.bMax;
				res = m_table[0][r] | m_table[1][g] | m_table[2][b];
			}

			if(sizeof(TD)==3) *(UINT32*)dst = res;
			else *dst = res;

			src++;
			dst++;
		}

		//always convert last pixel by table (simple)
		if(sizeof(TS)==3 || sizeof(TD)==3)	//compile-time
		{
			UINT32 c = *src;
			UINT r = (c >> fSrc.rShift) & fSrc.rMax;
			UINT g = (c >> fSrc.gShift) & fSrc.gMax;
			UINT b = (c >> fSrc.bShift) & fSrc.bMax;
			*dst = m_table[0][r] | m_table[1][g] | m_table[2][b];
		}
	}
};


//-----------------------------------------------------------------
// general converter for RGB -> Y conversion
//-----------------------------------------------------------------
class TrTranslator2GrayscaleBase : public TrTranslator
{
public:

	TrTranslator2GrayscaleBase(const aaPixelFormat& _fSrc)
		: fSrc(_fSrc)
	{
		m_needToDelete = true;

		for(UINT i=0; i<256; i++)
		{
			m_table[0][i] = (UINT8)(i * 0.299f * 255.0f / fSrc.rMax);
			m_table[1][i] = (UINT8)(i * 0.587f * 255.0f / fSrc.gMax);
			m_table[2][i] = (UINT8)(i * 0.114f * 255.0f / fSrc.bMax);
		}
	}

protected:
	aaPixelFormat fSrc;
	UINT8 m_table[3][256];
};


//-----------------------------------------------------------------
// general converter for RGB -> Y conversion
//-----------------------------------------------------------------
template<typename TS>
class TrTranslator2Grayscale : public TrTranslator2GrayscaleBase
{
public:

	TrTranslator2Grayscale(const aaPixelFormat& _fSrc)
		: TrTranslator2GrayscaleBase(_fSrc)
	{
	}

	virtual void Translate(void* _dst, const void* _src, int nPixels)
	{
		if (sizeof(TS)==3) nPixels--; //do not read last pixel UINT24 as UINT32 due to access violation

		const TS* src = (const TS*)_src;
		const TS* end = src + nPixels;
		   UINT8* dst = (UINT8*)_dst;

		//movzx-and -> and
		UINT rMax = fSrc.rMax;
		UINT gMax = fSrc.gMax;
		UINT bMax = fSrc.bMax;
		while(src<end)
		{
			UINT s = (sizeof(TS)==3) ? *((UINT32*)src) : *src;
			UINT r = (s >> fSrc.rShift) & rMax;
			UINT g = (s >> fSrc.gShift) & gMax;
			UINT b = (s >> fSrc.bShift) & bMax;
			*dst  = m_table[0][r] + m_table[1][g] + m_table[2][b];

			src++;
			dst++;
		}

		//always convert last pixel by table (simple)
		if(sizeof(TS)==3)	//compile-time
		{
			UINT32 s = *src;
			UINT r = (s >> fSrc.rShift) & rMax;
			UINT g = (s >> fSrc.gShift) & gMax;
			UINT b = (s >> fSrc.bShift) & bMax;
			*dst  = m_table[0][r] + m_table[1][g] + m_table[2][b];
		}
	}
};



//-----------------------------------------------------------------
// general converter for Y -> RGB
//-----------------------------------------------------------------
template<int table, typename TD>
class TrTranslatorGrayscale : public TrTranslator
{
public:

	TrTranslatorGrayscale(const aaPixelFormat& _fDst)
		: fDst(_fDst)
	{
		m_needToDelete = true;
		rDiff = 8 - GetNumBits(fDst.rMax);
		gDiff = 8 - GetNumBits(fDst.gMax);
		bDiff = 8 - GetNumBits(fDst.bMax);
	}

	virtual void Translate(void* _dst, const void* _src, int nPixels)
	{
		if(sizeof(TD)==3) nPixels--; //do not write last pixel UINT24 as UINT32 due to access violation

		const UINT8* src = (const UINT8*)_src;
		const UINT8* end = src + nPixels;
		         TD* dst = (TD*)_dst;

		while(src<end)
		{
			UINT s = *src;
			UINT res = ((s >> rDiff) << fDst.rShift)
			         | ((s >> gDiff) << fDst.gShift)
			         | ((s >> bDiff) << fDst.bShift);

			if(sizeof(TD)==3) *(UINT32*)dst = res;
			else *dst = res;

			src++;
			dst++;
		}

		//always convert last pixel by table (simple)
		if(sizeof(TD)==3)	//compile-time
		{
			UINT s = *src;
			*dst = ((s >> rDiff) << fDst.rShift)
			     | ((s >> gDiff) << fDst.gShift)
			     | ((s >> bDiff) << fDst.bShift);
		}
	}

protected:
	aaPixelFormat fDst;
	char rDiff, gDiff, bDiff;
};






enum InitPalette
{
	//IP_BY_PTR = 0,
	IP_R3G3B2,
	IP_GRAYSCALE
};

template<typename TD,
	char rBits, char rShift,
	char gBits, char gShift,
	char bBits, char bShift>
class TrTranslatorPalette : public TrTranslator
{
public:

	TrTranslatorPalette(const TD* palettePtr, InitPalette ip)
	{
		m_needToDelete = false;

		if(palettePtr)
			memcpy(m_table, palettePtr, 256);
		else
			for (UINT i=0; i<256; i++)
			{
				TD r,g,b;
				switch(ip)
				{
				case IP_R3G3B2:
					r = ((i>>5)&7) * rMaxDst / 7;
					g = ((i>>2)&7) * gMaxDst / 7;
					b = ((i>>0)&3) * bMaxDst / 3;
					break;

				case IP_GRAYSCALE:
					r = i >> (8-rBits);
					g = i >> (8-gBits);
					b = i >> (8-bBits);
					break;
				}

				m_table[i] = (r<<rShift) | (g<<gShift) | (b<<bShift);
			}
	}

	virtual void Translate(void* _dst, const void* _src, int nPixels)
	{
		UINT8* src = (UINT8*)_src;
		TD*    dst = (TD*)_dst;
		const UINT8* end = src + nPixels;

		while(src<end)
		{
			//*(UINT32*)dst = *(UINT*)&m_table[ *src ]; // it works worse, I don't know why. It need m_table[257]
			*dst = m_table[ *src ];
			src++;
			dst++;
		}
	}

private:
	enum{
		rMaxDst = (1<<rBits)-1,
		gMaxDst = (1<<gBits)-1,
		bMaxDst = (1<<bBits)-1,
	};
	TD m_table[256];
};







//-----------------------------------------------------------------
// convert from bitfield 2 bitfield on integral type
//-----------------------------------------------------------------
template<
	typename TS, char rSrcShift, char rSrcBits, char gSrcShift, char gSrcBits, char bSrcShift, char bSrcBits,
	typename TD, char rDstShift, char rDstBits, char gDstShift, char gDstBits, char bDstShift, char bDstBits>
class TrTranslatorField2Field : public TrTranslator
{
public:
	TrTranslatorField2Field(bool tabled)
	{
		if(sizeof(TS)==sizeof(UINT24) || sizeof(TD)==sizeof(UINT24))
			throw RLException("UINT24 can't be used as source type for TrTranslatorField2Field");

		m_needToDelete = false;
		m_useTable = tabled;

		UINT rgMax  = (rSrcBits > gSrcBits) ? rSrcBits : gSrcBits;
		UINT rgbMax = (rgMax > bSrcBits) ? rgMax : bSrcBits;
		UINT tblSize = 1<<rgbMax;


		// build table for fast convertion
		// expand or reduce bits
		// loop over maximum component source value
		// a bit slower but code size is matter
		// 51 vs 287 bytes
		for (UINT i=0; i<tblSize; i++)
		{
			UINT c;

			c = (rDstBits > rSrcBits) ? i*rDstMax/rSrcMax : i >> (rSrcBits-rDstBits);  m_table[0][i] = c << rDstShift;
			c = (gDstBits > gSrcBits) ? i*gDstMax/gSrcMax : i >> (gSrcBits-gDstBits);  m_table[1][i] = c << gDstShift;
			c = (bDstBits > bSrcBits) ? i*bDstMax/bSrcMax : i >> (bSrcBits-bDstBits);  m_table[2][i] = c << bDstShift;
		}
	}


	virtual void Translate(void* dst, const void* src, int nPixels)
	{
		if(m_useTable)
			tbl((const TS*)src, (TD*)dst, nPixels);
		else
			cpu((const TS*)src, (TD*)dst, nPixels);
	}

private:
	void cpu(const TS* src, TD* dst, int nPixels)
	{
		const TS* end = src + nPixels;
		while(src<end)
		{
			UINT s = (UINT)*src;

			UINT r,g,b;
			//if(sizeof(TD) <= sizeof(TS)) then TS r,g,b
			//working on a smaller type is a bit faster in some cases depending on compiler behaviour

			r = g = b = s;

			     if(rDiff>0) r >>=  rDiff;
			else if(rDiff<0) r <<= -rDiff;

			     if(gDiff>0) g >>=  gDiff;
			else if(gDiff<0) g <<= -gDiff;

			     if(bDiff>0) b >>=  bDiff;
			else if(bDiff<0) b <<= -bDiff;

			*dst = (r & rDstMask) | (g & gDstMask) | (b & bDstMask);

			src++;
			dst++;
		}
	}
	void tbl(const TS* src, TD* dst, int nPixels)
	{
		const TS* end = src + nPixels;
		while(src<end)
		{
			UINT s = *src;
			UINT r = (s >> rSrcShift) & rSrcMax;
			UINT g = (s >> gSrcShift) & gSrcMax;
			UINT b = (s >> bSrcShift) & bSrcMax;
			*dst = m_table[0][r] | m_table[1][g] | m_table[2][b];
			src++;
			dst++;
		}
	}

	enum {
		rSrcMax = (1<<rSrcBits)-1,
		gSrcMax = (1<<gSrcBits)-1,
		bSrcMax = (1<<bSrcBits)-1,
		rDstMax = (1<<rDstBits)-1,
		gDstMax = (1<<gDstBits)-1,
		bDstMax = (1<<bDstBits)-1,
		rDiff = rSrcShift-rDstShift + rSrcBits-rDstBits,
		gDiff = gSrcShift-gDstShift + gSrcBits-gDstBits,
		bDiff = bSrcShift-bDstShift + bSrcBits-bDstBits,
		rDstMask = rDstMax << rDstShift,
		gDstMask = gDstMax << gDstShift,
		bDstMask = bDstMax << bDstShift,
	};
	static TD m_table[3][256+1]; // should be 257 for UINT24
	bool m_useTable;
};




//-----------------------------------------------------------------
// convert from byte[] to bitfield on integral type, TD - UINT8, UINT16
//-----------------------------------------------------------------
template<char rPos, char gPos, char bPos, size_t szSrc, typename TD,
	char rBits, char rShift,
	char gBits, char gShift,
	char bBits, char bShift>
	class TrTranslatorArray2Field : public TrTranslator
{
public:

	TrTranslatorArray2Field(bool table)
	{
		m_needToDelete = false;
		m_useTable = table;
		for (int i=0; i<256; i++)
		{
			m_table[0][i] = (i>>(8-rBits)) << rShift;
			m_table[1][i] = (i>>(8-gBits)) << gShift;
			m_table[2][i] = (i>>(8-bBits)) << bShift;
		}
	}

	virtual void Translate(void* dst, const void* src, int nPixels)
	{
		if(m_useTable)
			tbl((const UINT8*)src, (TD*)dst, nPixels);
		else
			cpu((const UINT8*)src, (TD*)dst, nPixels);
	}

private:
	void cpu(const UINT8* src, TD* dst, int nPixels)
	{
		const TD* end = dst + nPixels;
		while(dst<end)
		{
			TD b = src[bPos];
			TD g = src[gPos];
			TD r = src[rPos];

			     if(bDiff>0) b >>=  bDiff; 
			else if(bDiff<0) b <<= -bDiff;
			
			     if(gDiff>0) g >>=  gDiff;
			else if(gDiff<0) g <<= -gDiff;
			
			     if(rDiff>0) r >>=  rDiff;
			else if(rDiff<0) r <<= -rDiff;

			*dst = (r & rMask) | (g & gMask) | (b & bMask);

			src += szSrc;
			dst++;
		}
	}

	void tbl(const UINT8* src, TD* dst, int nPixels)
	{
		const UINT8* end = src + nPixels*szSrc;
		while(src<end)
		{
			*dst = m_table[0][ src[rPos] ] | m_table[1][ src[gPos] ] | m_table[2][ src[bPos] ];
			src += szSrc;
			dst++;
		}
	}

	enum {
		rMax = (1<<rBits)-1,
		gMax = (1<<gBits)-1,
		bMax = (1<<bBits)-1,
		bDiff = 8-(bShift+bBits),
		gDiff = 8-(gShift+gBits),
		rDiff = 8-(rShift+rBits),
		rMask = rMax << rShift,
		gMask = gMax << gShift,
		bMask = bMax << bShift,
	};
	static TD m_table[3][256+1];  // should be 257 for UINT24
	bool m_useTable;
};




//-----------------------------------------------------------------
// convert from byte[] to byte[]
//-----------------------------------------------------------------
template<
	char rSrcPos, char gSrcPos, char bSrcPos, char aSrcPos, size_t szSrc,
	char rDstPos, char gDstPos, char bDstPos, char aDstPos, size_t szDst>
class TrTranslatorArray2Array : public TrTranslator
{
public:

	TrTranslatorArray2Array() 
	{
		m_needToDelete = false;
	}

	virtual void Translate(void* _dst, const void* _src, int nPixels)
	{
		const UINT8* src = (const UINT8*)_src;
		const UINT8* end = src + nPixels*szSrc;
		      UINT8* dst = (UINT8*)_dst;
		while(src<end)
		{
			dst[bDstPos] = src[bSrcPos];
			if(szDst>1)
			{
				if(szSrc>1) dst[gDstPos] = src[gSrcPos];
				else dst[gDstPos] = 0;
				if(szDst>2)
				{
					if(szSrc>2) dst[rDstPos] = src[rSrcPos];
					else dst[rDstPos] = 0;
					if(szDst>3)
					{
						if(szSrc>3) dst[aDstPos] = src[aSrcPos];
						else dst[aDstPos] = 0;
					}
				}
			}

			src += szSrc;
			dst += szDst;
		}
	}
};


//-----------------------------------------------------------------
// convert from field to byte[]
//-----------------------------------------------------------------
template<typename TS,	char rShift, char rBits,	char gShift, char gBits,	char bShift, char bBits,
	char rPos, char gPos, char bPos, char aPos, size_t szDst>
class TrTranslatorField2Array : public TrTranslator
{
public:

	TrTranslatorField2Array()
	{
		m_needToDelete = false;
		for (UINT i=0; i<256; i++)
		{
			m_table[0][i] = i*255/rMax;
			m_table[1][i] = i*255/gMax;
			m_table[2][i] = i*255/bMax;
		}
	}

	virtual void Translate(void* dst, const void* src, int nPixels)
	{
		tbl((const TS*)src, (UINT8*)dst, nPixels);
	}

protected:
	void tbl(const TS* src, UINT8* dst, int nPixels)
	{
		const TS* end = src + nPixels;
		while(src<end)
		{
			UINT s = *src;
			dst[rPos] = m_table[0][ (s & rMask) >> rShift];
			dst[gPos] = m_table[1][ (s & gMask) >> gShift];
			dst[bPos] = m_table[2][ (s & bMask) >> bShift];
			if(szDst>3) dst[aPos] = 0;

			src++;
			dst += szDst;
		}
	}

	enum {
		rMax = (1<<rBits)-1,
		gMax = (1<<gBits)-1,
		bMax = (1<<bBits)-1,
		rMask = rMax << rShift,
		gMask = gMax << gShift,
		bMask = bMax << bShift,
	};
	static UINT8 m_table[3][256];
};


//static tables definition
//257 is for reading UINT24 as 32 bit
UINT32 TrTranslatorField2Field<UINT16, 10,5, 5,5, 0,5, UINT32, 16,8, 8,8, 0, 8>::m_table[3][257];	//565-8888
UINT32 TrTranslatorField2Field<UINT16, 11,5, 5,6, 0,5, UINT32, 16,8, 8,8, 0, 8>::m_table[3][257];	//565-8888
UINT16 TrTranslatorField2Field<UINT16, 11,5, 5,6, 0,5, UINT16, 10,5, 5,5, 0, 5>::m_table[3][257];	//565-1555
UINT16 TrTranslatorField2Field<UINT16, 10,5, 5,5, 0,5, UINT16, 11,5, 5,6, 0, 5>::m_table[3][257];	//1555-565
UINT8  TrTranslatorField2Field<UINT16, 11,5, 5,6, 0,5, UINT8,   5,3, 2,3, 0, 2>::m_table[3][257];	//565-332
UINT8  TrTranslatorField2Field<UINT16, 10,5, 5,5, 0,5, UINT8,   5,3, 2,3, 0, 2>::m_table[3][257];	//1555-332
UINT16 TrTranslatorArray2Field<2,1,0, sizeof(UINT32),UINT16, 5, 11, 6, 5, 5, 0>::m_table[3][257];
UINT16 TrTranslatorArray2Field<2,1,0, sizeof(UINT32),UINT16, 5, 10, 5, 5, 5, 0>::m_table[3][257];
UINT8  TrTranslatorArray2Field<2,1,0, sizeof(UINT32),UINT8,  3,  5, 3, 2, 2, 0>::m_table[3][257];
UINT16 TrTranslatorArray2Field<2,1,0, sizeof(UINT24),UINT16, 5, 11, 6, 5, 5, 0>::m_table[3][257];
UINT16 TrTranslatorArray2Field<2,1,0, sizeof(UINT24),UINT16, 5, 10, 5, 5, 5, 0>::m_table[3][257];
UINT8  TrTranslatorArray2Field<2,1,0, sizeof(UINT24),UINT8,  3,  5, 3, 2, 2, 0>::m_table[3][257];

UINT8  TrTranslatorField2Array<UINT16, 11,5, 5,6, 0,5,	2,1,0,3, sizeof(UINT32)>::m_table[3][256];
UINT8  TrTranslatorField2Array<UINT16, 11,5, 5,6, 0,5,	2,1,0,0, sizeof(UINT24)>::m_table[3][256];
UINT8  TrTranslatorField2Array<UINT16, 10,5, 5,5, 0,5,	2,1,0,3, sizeof(UINT32)>::m_table[3][256];
UINT8  TrTranslatorField2Array<UINT16, 10,5, 5,5, 0,5,	2,1,0,0, sizeof(UINT24)>::m_table[3][256];


//-------------------------------------------------------------
//return appropriate translator
TrTranslator* TrTranslator::Init(const aaPixelFormat& fSrc, const aaPixelFormat& fDst, bool table, bool general)
{
	table = true; // it decrease size of EXE on 12K

	if (fSrc == fDst)
		return new TrTranslatorNone(fSrc.bitsPerPixel / 8);


	if(!general)
	{
		if(fSrc == f_x8r8g8b8)
		{
			if(fDst == f_r8g8b8)
			{
				static TrTranslatorArray2Array<2,1,0,3, sizeof(UINT32), 2,1,0,3, sizeof(UINT24)> c;
				return &c;
			}
			if(fDst == f_r5g6b5)
			{
				static TrTranslatorArray2Field<2,1,0, sizeof(UINT32),UINT16, 5, 11, 6, 5, 5, 0> c1(false), c2(true);
				return table ? &c2 : &c1;
			}
			if(fDst == f_x1r5g5b5)
			{
				static TrTranslatorArray2Field<2,1,0, sizeof(UINT32),UINT16, 5, 10, 5, 5, 5, 0> c1(false), c2(true);
				return table ? &c2 : &c1;
			}
			if(fDst == f_r3g3b2)
			{
				static TrTranslatorArray2Field<2,1,0, sizeof(UINT32),UINT8, 3, 5, 3, 2, 2, 0> c1(false), c2(true);
				return table ? &c2 : &c1;
			}
		}

		if(fSrc == f_r8g8b8)
		{
			if(fDst == f_x8r8g8b8)
			{
				static TrTranslatorArray2Array<2,1,0,3, sizeof(UINT24), 2,1,0,3, sizeof(UINT32)> c;
				return &c;
			}
			if(fDst == f_r5g6b5)
			{
				static TrTranslatorArray2Field<2,1,0, sizeof(UINT24),UINT16, 5, 11, 6, 5, 5, 0> c1(false), c2(true);
				return table ? &c2 : &c1;
			}
			if(fDst == f_x1r5g5b5)
			{
				static TrTranslatorArray2Field<2,1,0, sizeof(UINT24),UINT16, 5, 10, 5, 5, 5, 0> c1(false), c2(true);
				return table ? &c2 : &c1;
			}
			if(fDst == f_r3g3b2)
			{
				static TrTranslatorArray2Field<2,1,0, sizeof(UINT24),UINT8, 3, 5, 3, 2, 2, 0> c1(false), c2(true);
				return table ? &c2 : &c1;
			}
		}

		if(fSrc == f_r5g6b5)
		{
			if(fDst == f_x1r5g5b5)
			{
				static TrTranslatorField2Field<UINT16, 11,5, 5,6, 0,5, UINT16, 10,5, 5,5, 0,5> c1(false), c2(true);
				return table ? &c2 : &c1;
			}
			if(fDst == f_r3g3b2)
			{
				static TrTranslatorField2Field<UINT16, 11,5, 5,6, 0,5,	UINT8, 5,3, 2,3, 0,2> c1(false), c2(true);
				return table ? &c2 : &c1;
			}
			if(fDst == f_x8r8g8b8)
			{
				static TrTranslatorField2Array<UINT16, 11,5, 5,6, 0,5,	2,1,0,3, sizeof(UINT32)> c;
				return &c;
			}
			if(fDst == f_r8g8b8)
			{
				static TrTranslatorField2Array<UINT16, 11,5, 5,6, 0,5,	2,1,0,0, sizeof(UINT24)> c;
				return &c;
			}
		}

		if(fSrc == f_x1r5g5b5)
		{
			if(fDst == f_r5g6b5)
			{
				static TrTranslatorField2Field<UINT16, 10,5, 5,5, 0,5,	UINT16, 11,5, 5,6, 0,5> c1(false), c2(true);
				return table ? &c2 : &c1;
			}
			if(fDst == f_r3g3b2)
			{
				static TrTranslatorField2Field<UINT16, 10,5, 5,5, 0,5,	UINT8, 5,3, 2,3, 0,2> c1(false), c2(true);
				return table ? &c2 : &c1;
			}
			if(fDst == f_x8r8g8b8)
			{
				static TrTranslatorField2Array<UINT16, 10,5, 5,5, 0,5,	2,1,0,3, sizeof(UINT32)> c;
				return &c;
			}
			if(fDst == f_r8g8b8)
			{
				static TrTranslatorField2Array<UINT16, 10,5, 5,5, 0,5,	2,1,0,0, sizeof(UINT24)> c;
				return &c;
			}
		}

		if(fSrc == f_r3g3b2)
		{
			if(fDst == f_x1r5g5b5)
			{
				static TrTranslatorPalette<UINT16, 5, 10, 5, 5, 5, 0> c(0, IP_R3G3B2);
				return &c;
			}
			if(fDst == f_r5g6b5)
			{
				static TrTranslatorPalette<UINT16, 5, 11, 6, 5, 5, 0> c(0, IP_R3G3B2);
				return &c;
			}
			if(fDst == f_r8g8b8)
			{
				static TrTranslatorPalette<UINT24, 8, 16, 8, 8, 8, 0> c(0, IP_R3G3B2);
				return &c;
			}
			if(fDst == f_x8r8g8b8)
			{
				static TrTranslatorPalette<UINT32, 8, 16, 8, 8, 8, 0> c(0, IP_R3G3B2);
				return &c;
			}
		}

		if(fSrc == f_y8)
		{
			if(fDst == f_r3g3b2)
			{
				static TrTranslatorPalette<UINT8, 3,5, 3,2, 2,0> c(0, IP_GRAYSCALE);
				return &c;
			}
			if(fDst == f_x1r5g5b5)
			{
				static TrTranslatorPalette<UINT16, 5, 10, 5, 5, 5, 0> c(0, IP_GRAYSCALE);
				return &c;
			}
			if(fDst == f_r5g6b5)
			{
				static TrTranslatorPalette<UINT16, 5, 11, 6, 5, 5, 0> c(0, IP_GRAYSCALE);
				return &c;
			}
			if(fDst == f_r8g8b8)
			{
				static TrTranslatorPalette<UINT24, 8, 16, 8, 8, 8, 0> c(0, IP_GRAYSCALE);
				return &c;
			}
			if(fDst == f_x8r8g8b8)
			{
				static TrTranslatorPalette<UINT32, 8, 16, 8, 8, 8, 0> c(0, IP_GRAYSCALE);
				return &c;
			}
		}
	}


	char srcSize = (fSrc.type==aaGrayScale) ? 0 : fSrc.bitsPerPixel/8;
	char dstSize = (fDst.type==aaGrayScale) ? 0 : fDst.bitsPerPixel/8;

	//table = false; // use from argument
	if (fSrc.rMax < fDst.rMax) table = true;
	if (fSrc.gMax < fDst.gMax) table = true;
	if (fSrc.bMax < fDst.bMax) table = true;

	if (!table)
	{
		if (srcSize==0) {
			switch(dstSize)
			{
				case 1: return new TrTranslatorGrayscale<0, UINT8> (fDst);
				case 2: return new TrTranslatorGrayscale<0, UINT16> (fDst);
				case 3: return new TrTranslatorGrayscale<0, UINT24> (fDst);
				case 4: return new TrTranslatorGrayscale<0, UINT32> (fDst);
			}
		}
		else if (srcSize==1) {
			switch(dstSize)
			{
				case 0: return new TrTranslator2Grayscale<UINT8> (fSrc);
				case 1: return new TrTranslatorGeneral<0, UINT8, UINT8> (fSrc, fDst);
				case 2: return new TrTranslatorGeneral<0, UINT8, UINT16>(fSrc, fDst);
				case 3: return new TrTranslatorGeneral<0, UINT8, UINT24>(fSrc, fDst);
				case 4: return new TrTranslatorGeneral<0, UINT8, UINT32>(fSrc, fDst);
			}
		}
		else if (srcSize==2) {
			switch(dstSize)
			{
				case 0: return new TrTranslator2Grayscale<UINT16> (fSrc);
				case 1: return new TrTranslatorGeneral<0, UINT16, UINT8> (fSrc, fDst);
				case 2: return new TrTranslatorGeneral<0, UINT16, UINT16>(fSrc, fDst);
				case 3: return new TrTranslatorGeneral<0, UINT16, UINT24>(fSrc, fDst);
				case 4: return new TrTranslatorGeneral<0, UINT16, UINT32>(fSrc, fDst);
			}
		}
		else if (srcSize==3) {
			switch(dstSize)
			{
				case 0: return new TrTranslator2Grayscale<UINT24> (fSrc);
				case 1: return new TrTranslatorGeneral<0, UINT24, UINT8> (fSrc, fDst);
				case 2: return new TrTranslatorGeneral<0, UINT24, UINT16>(fSrc, fDst);
				case 3: return new TrTranslatorGeneral<0, UINT24, UINT24>(fSrc, fDst);
				case 4: return new TrTranslatorGeneral<0, UINT24, UINT32>(fSrc, fDst);
			}
		}
		else if (srcSize==4) {
			switch(dstSize)
			{
				case 0: return new TrTranslator2Grayscale<UINT32> (fSrc);
				case 1: return new TrTranslatorGeneral<0, UINT32, UINT8> (fSrc, fDst);
				case 2: return new TrTranslatorGeneral<0, UINT32, UINT16>(fSrc, fDst);
				case 3: return new TrTranslatorGeneral<0, UINT32, UINT24>(fSrc, fDst);
				case 4: return new TrTranslatorGeneral<0, UINT32, UINT32>(fSrc, fDst);
			}
		}
	}
	else //*/
	{
		if (srcSize==0) {
			switch(dstSize)
			{
				case 1: return new TrTranslatorGrayscale<1, UINT8> (fDst);
				case 2: return new TrTranslatorGrayscale<1, UINT16> (fDst);
				case 3: return new TrTranslatorGrayscale<1, UINT24> (fDst);
				case 4: return new TrTranslatorGrayscale<1, UINT32> (fDst);
			}
		}
		else if (srcSize==1) {
			switch(dstSize)
			{
				case 0: return new TrTranslator2Grayscale<UINT8> (fSrc);
				case 1: return new TrTranslatorGeneral<1, UINT8, UINT8> (fSrc, fDst);
				case 2: return new TrTranslatorGeneral<1, UINT8, UINT16>(fSrc, fDst);
				case 3: return new TrTranslatorGeneral<1, UINT8, UINT24>(fSrc, fDst);
				case 4: return new TrTranslatorGeneral<1, UINT8, UINT32>(fSrc, fDst);
			}
		}
		else if (srcSize==2) {
			switch(dstSize)
			{
				case 0: return new TrTranslator2Grayscale<UINT16> (fSrc);
				case 1: return new TrTranslatorGeneral<1, UINT16, UINT8> (fSrc, fDst);
				case 2: return new TrTranslatorGeneral<1, UINT16, UINT16>(fSrc, fDst);
				case 3: return new TrTranslatorGeneral<1, UINT16, UINT24>(fSrc, fDst);
				case 4: return new TrTranslatorGeneral<1, UINT16, UINT32>(fSrc, fDst);
			}
		}
		else if (srcSize==3) {
			switch(dstSize)
			{
				case 0: return new TrTranslator2Grayscale<UINT24> (fSrc);
				case 1: return new TrTranslatorGeneral<1, UINT24, UINT8> (fSrc, fDst);
				case 2: return new TrTranslatorGeneral<1, UINT24, UINT16>(fSrc, fDst);
				case 3: return new TrTranslatorGeneral<1, UINT24, UINT24>(fSrc, fDst);
				case 4: return new TrTranslatorGeneral<1, UINT24, UINT32>(fSrc, fDst);
			}
		}
		else if (srcSize==4) {
			switch(dstSize)
			{
				case 0: return new TrTranslator2Grayscale<UINT32> (fSrc);
				case 1: return new TrTranslatorGeneral<1, UINT32, UINT8> (fSrc, fDst);
				case 2: return new TrTranslatorGeneral<1, UINT32, UINT16>(fSrc, fDst);
				case 3: return new TrTranslatorGeneral<1, UINT32, UINT24>(fSrc, fDst);
				case 4: return new TrTranslatorGeneral<1, UINT32, UINT32>(fSrc, fDst);
			}
		}
	}

	throw RLException("TrTranslator::Init()#2");
}


CStringA TrTranslator::GetPixelFormatAsString(const aaPixelFormat& f)
{
	char buffer[16];
	CStringA res;
	UINT16 vMax;

	if(f.type==aaGrayScale) return "y8";

	int rBits = f.rShift + GetNumBits(f.rMax);
	int gBits = f.gShift + GetNumBits(f.gMax);
	int bBits = f.bShift + GetNumBits(f.bMax);

	int nEmptyBits = 0;
	for(int b=f.bitsPerPixel; b>1;)
	{
		const char* prefix;
		     if (rBits == b) { vMax = f.rMax; prefix = "r"; }
		else if (gBits == b) { vMax = f.gMax; prefix = "g"; }
		else if (bBits == b) { vMax = f.bMax; prefix = "b";	}
		else {
			//empty bits
			prefix = "x";
			nEmptyBits++;
		}

		//finishing empty bits field
		if(nEmptyBits>0 && prefix!="x")
		{
			sprintf(buffer, "x%d", nEmptyBits);
			res += buffer;
			nEmptyBits = 0;
		}

		if(nEmptyBits==0)
		{
			int nb = GetNumBits(vMax);
			sprintf(buffer, "%s%d", prefix, nb);
			res += buffer;
			b -= nb;
		}
		else
			b--;
	}

	//finishing empty bits field
	if(nEmptyBits>0)
	{
		sprintf(buffer, "x%d", nEmptyBits);
		res += buffer;
	}

	return res;
}
