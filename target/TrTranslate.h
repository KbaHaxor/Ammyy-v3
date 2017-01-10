#if !defined(_TR_TRANSLATE_H_7846A2H_INCLUDED_)
#define _TR_TRANSLATE_H_7846A2H_INCLUDED_

#include "../main/aaProtocol.h"
#include "../main/aaDesktop.h"

//-----------------------------------------------------------------
// converter interface
// a suitable class can be retrieved by calling Init
//-----------------------------------------------------------------
class TrTranslator
{
public:
	virtual void Translate(void* dst, const void* src, int nPixels) = 0; // where nPixels>0 !

	TrTranslator()
	{
		m_needToDelete = false; // true only for Generic & None
	}

	//	"table" - method used for reducing bits or shift-mask. expanding bits always uses tables
	//	"general" - use general or optimized conversion routine
	static TrTranslator* Init(const aaPixelFormat& fSrc, const aaPixelFormat& fDst, bool table = true, bool general = false);

	static CStringA GetPixelFormatAsString(const aaPixelFormat& f);

	static void Free(TrTranslator*& pTranslate)
	{
		if (pTranslate!=NULL) {
			if (pTranslate->m_needToDelete) delete pTranslate;
			pTranslate = NULL;
		}
	}	

	static aaPixelFormat f_x8r8g8b8;
	static aaPixelFormat f_r8g8b8;
	static aaPixelFormat f_x1r5g5b5;
	static aaPixelFormat f_r5g6b5;
	static aaPixelFormat f_r3g3b2;
	static aaPixelFormat f_y8;


protected:
	virtual ~TrTranslator() {}

	static int GetNumBits(UINT16 value)
	{
		for (int i=0; i<=16; i++) {
			if (value < (1<<i)) return i;
		}
		throw RLException("GetNumBits() ERROR");
	}

protected:
public:
	bool m_needToDelete;
};


#endif // !defined(_TR_TRANSLATE_H_7846A2H_INCLUDED_)
