#include "stdafx.h"
#include "VrEncoder.h"
#include "VrClient.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

VrEncoders::VrEncoders()
{
	m_hBitmapDC = NULL;
	m_hBitmap = NULL;

	m_translator       = NULL;
	m_translatorJPEG   = NULL;

	// TODO: need to optimize it
	m_frmL = TrTranslator::f_x8r8g8b8;
	m_bytesppL = m_frmL.bitsPerPixel / 8;
}


VrEncoders::~VrEncoders()
{
	try {
		this->ResetTranslators();
		this->ResetEncoders();
	}
	catch(RLException&) {}
}

void VrEncoders::ResetTranslators()
{
	TrTranslator::Free(m_translator);
	TrTranslator::Free(m_translatorJPEG);
}

void VrEncoders::ResetEncoders()
{
	int i;
	//for (i=0; i<COUNTOF(m_tightZlibStream); i++) m_tightZlibStream[i].DeInit();
	for (i=0; i<COUNTOF(m_streamAAC);       i++) m_streamAAC[i].DeInit();

	//m_decompStream.DeInit();
}


// Reads the number of bytes specified into the buffer given
void VrEncoders::ReadExact(void *inbuf, int wanted)
{
	if (wanted==0) return;

	m_transport->ReadExact(inbuf, wanted);
}

int VrEncoders::ReadCompactLen()
{
	UINT8 len_byte;
	ReadExact(&len_byte, 1);
	int compressedLen = (int)len_byte & 0x7F;
	if (len_byte & 0x80) {
		ReadExact(&len_byte, 1);
		compressedLen |= ((int)len_byte & 0x7F) << 7;
		if (len_byte & 0x80) {
			ReadExact(&len_byte, 1);
			compressedLen |= ((int)len_byte & 0xFF) << 14;
		}
	}
	return compressedLen;
}


//__________________________________________________________________________________________________________________

void VrEncoders::ReadRawRect(aaFramebufferUpdateRectHeader *pfburh)
{
	UINT numbytes = pfburh->r.w * pfburh->r.h * m_bytesppR;	// this assumes at least one byte per pixel. Naughty.
	
	// Read in the whole thing
	BYTE* buffer1 = (BYTE*)m_netBuffer.GetBuffer1(numbytes);
	ReadExact(buffer1, numbytes);

	{
		//RLMutexLock l(m_bitmapdcMutex);
		SETPIXELS(buffer1, pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h);
	}
}

// __________________________________________________________________________________________________________________________

/*
void VrEncoders::ReadRRERect(aaFramebufferUpdateRectHeader *pfburh)
{
	// An RRE rect is always followed by a background color
	// For speed's sake we read them together into a buffer.
	char tmpbuf[sz_aaRREHeader+4];			// biggest pixel is 4 bytes long
    aaRREHeader *prreh = (aaRREHeader *) tmpbuf;
	UINT8 *pcolor = (UINT8 *) tmpbuf + sz_aaRREHeader;
	ReadExact(tmpbuf, sz_aaRREHeader + m_bytesppR);
	
	// Draw the background of the rectangle
	{
		//RLMutexLock l(m_bitmapdcMutex);
		FillSolidRect(pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h, pcolor);
	}
		
    if (prreh->nSubrects == 0) return;
	
	// Draw the sub-rectangles

	// The size of an RRE subrect including color info
	int subRectSize = m_bytesppR + sizeof(aaRectangle);
    
	// Read subrects into the buffer 
	BYTE* p = (BYTE*)m_netBuffer.GetBuffer1(subRectSize * prreh->nSubrects);
    ReadExact(p, subRectSize * prreh->nSubrects);

	// No other threads can use bitmap DC
	//RLMutexLock l(m_bitmapdcMutex);

	for (int i=0; i<prreh->nSubrects; i++)
	{
		aaRectangle* pRect = (aaRectangle *) (p + m_bytesppR);
				
		int x = pRect->x + pfburh->r.x;
		int y = pRect->y + pfburh->r.y;
		
		FillSolidRect(x, y, pRect->w, pRect->h, p);
		p += subRectSize;
	}
}
*/

// __________________________________________________________________________________________________________________________

/*
void VrEncoders::ReadCoRRERect(aaFramebufferUpdateRectHeader *pfburh)
{
	// An RRE rect is always followed by a background color
	// For speed's sake we read them together into a buffer.
	char tmpbuf[sz_aaRREHeader+4];			// biggest pixel is 4 bytes long
    aaRREHeader *prreh = (aaRREHeader *) tmpbuf;
	UINT8 *pcolor = (UINT8 *) tmpbuf + sz_aaRREHeader;
	ReadExact(tmpbuf, sz_aaRREHeader + m_bytesppR);

    // Draw the background of the rectangle
	{
		//RLMutexLock l(m_bitmapdcMutex);
		FillSolidRect(pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h, pcolor);
	}

    if (prreh->nSubrects == 0) return;

	// The size of an CoRRE subrect including color info
	int subRectSize = m_bytesppR + sz_aaCoRRERectangle;

	// Read subrects into the buffer 
	BYTE* p = (BYTE*)m_netBuffer.GetBuffer1(subRectSize * prreh->nSubrects);
    ReadExact(p, subRectSize * prreh->nSubrects);

	// No other threads can use bitmap DC
	//RLMutexLock l(m_bitmapdcMutex);

	for (int i=0; i<prreh->nSubrects; i++)
	{
		aaCoRRERectangle* pRect = (aaCoRRERectangle *) (p + m_bytesppR);
						
		int x = pRect->x + pfburh->r.x;
		int y = pRect->y + pfburh->r.y;

		FillSolidRect(x, y, pRect->w, pRect->h, p);
		p += subRectSize;
	}
}
*/

// __________________________________________________________________________________________________________________________


/*
void VrEncoders::ReadZlibRect(aaFramebufferUpdateRectHeader *pfburh) 
{
	UINT numRawBytes = pfburh->r.w * pfburh->r.h * m_bytesppR;
	
	aaZlibHeader hdr;
	ReadExact(&hdr, sizeof(hdr));

	UINT numCompBytes = hdr.nBytes;

	// Read in the compressed data
	BYTE* src = (BYTE*)m_netBuffer.GetBuffer1(numCompBytes);
	ReadExact(src, numCompBytes);

	// Verify enough buffer space for screen update.
	BYTE* dst = (BYTE*)m_zlibBuffer.GetBuffer1(numRawBytes);
		
	m_decompStream.Decompress(src, dst, numCompBytes, numRawBytes);

	{
		//RLMutexLock l(m_bitmapdcMutex);
		SETPIXELS(dst,  pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h);
	}
}
*/
