// TrEncoder - Object used to encode data

#include "stdafx.h"
#include "TrEncoder.h"
#include "TrDesktop.h"


TrEncoder::TrVector::TrVector()
{
	m_buff = NULL;
	m_size = 0;
}

TrEncoder::TrVector::~TrVector()
{
	if (m_buff != NULL) {
		delete m_buff;
		m_buff = NULL;
	}
	m_size = 0;
}


void TrEncoder::TrVector::Allocate1(UINT size)
{
	if (m_size==size) return;
	
	
	if (m_buff != NULL)
	{
		delete[] m_buff;
		m_buff = NULL;
	}
	m_size = 0;

	m_buff = new BYTE [size];
	if (m_buff == NULL)
		throw RLException("Unable to allocate client buffer[%d]", size);

    m_size = size;

    ::ZeroMemory(m_buff, m_size);	
}


void TrEncoder::TrVector::Allocate2(UINT size)
{
	if (m_size >= size) return;

	if (m_buff != NULL) {
		delete[] m_buff;
		m_size = 0;
	}

	m_buff = new BYTE [size+1];
	if (m_buff == NULL)
		throw RLException("failed to allocate memory %d bytes", size+1);

	m_size = size;
}



// The base (RAW) encoder class

TrEncoder::TrEncoder()
{
	::ZeroMemory(&m_frmR, sizeof(m_frmR));
	::ZeroMemory(&m_frmL, sizeof(m_frmL));

	m_compresslevel = 6;
	m_qualitylevel = aaJpegOFF;

	m_translator = NULL;
	m_translatorJPEG   = NULL;

	// statistics
	m_sizeRaw = 0;
	m_sizeSent = 0;
}

TrEncoder::~TrEncoder()
{
	TrTranslator::Free(m_translator);
	TrTranslator::Free(m_translatorJPEG);
}

void TrEncoder::LogStats()
{
	_log2.Print(LL_INF, VTCLOG("%s encoder stats: data=%d, sent=%d"),
				 GetEncodingName(), m_sizeRaw, m_sizeSent);

	if (m_sizeRaw != 0) {
		_log2.Print(LL_INF, VTCLOG("%s encoder efficiency: %.3f%%"), GetEncodingName(),
					 (double)((double)((m_sizeRaw - m_sizeSent) * 100) / m_sizeRaw));
	}
}


UINT TrEncoder::RequiredBuffSize(UINT cx, UINT cy)
{
	return sz_aaFramebufferUpdateRectHeader + (cx * cy * m_bytesppR);
}

// Translate a rectangle
void TrEncoder::Translate(void *dst, const RECT &rect)
{
	BYTE* _src = (BYTE*)this->GetSrcBufferPtr(rect.left, rect.top);
	BYTE* _dst = (BYTE*)dst;
	
	int cx = rect.right-rect.left;
	int cy = rect.bottom-rect.top;

	int bytesPerLineDst = m_bytesppR * cx;
	int bytesPerLineSrc = m_bytesPerRowL;

    while (true) {
		m_translator->Translate(_dst, _src, cx);
		if (--cy==0) break; // check it here to avoid dst & src increament on last loop
		_dst += bytesPerLineDst;
		_src += bytesPerLineSrc;
    }
}

// Encode a rectangle
void TrEncoder::EncodeRect(const RECT &rect)
{
	BYTE *dest = m_dest.m_buff;

	const int cx = rect.right - rect.left;
	      int cy = rect.bottom - rect.top;

	int rowSize = cx * m_bytesppR;

	int maxRows = (128*1024 / rowSize); if (maxRows==0) maxRows=1; // at least 1 row
	
	// divide into many rectangles
	//
	RECT rect1 = rect;
	
	while (cy>0) {
		int rows = min(cy, maxRows);

		// Create the header for the update in the destination area
		aaFramebufferUpdateRectHeader *surh = (aaFramebufferUpdateRectHeader *)dest;
		surh->encoder = aaEncoderRaw;
		surh->r.x = rect1.left;
		surh->r.y = rect1.top;
		surh->r.w = cx;
		surh->r.h = rows;

		rect1.bottom = rect1.top + rows;

		// Translate the data in place in the output buffer
		Translate(dest + sz_aaFramebufferUpdateRectHeader, rect1);

		this->Out(dest, sz_aaFramebufferUpdateRectHeader + rowSize*rows);

		rect1.top += rows;
		cy		  -= rows;
	}
	

	 //without dividing!
	/*
	aaFramebufferUpdateRectHeader *surh = (aaFramebufferUpdateRectHeader *)dest;
	surh->encoder = aaEncoderRaw;
	surh->r.x = rect.left;
	surh->r.y = rect.top;
	surh->r.w = cx;
	surh->r.h = cy;		

	// Translate the data in place in the output buffer
	Translate(dest + sz_aaFramebufferUpdateRectHeader, rect);

	this->Out(dest, sz_aaFramebufferUpdateRectHeader + cy * rowSize);
	*/
}

// Encode a rectangle directly to the output stream.
// This implementation may not be the best, but it will work with all
// of the existing EncodeRect(BYTE *, BYTE *, const RECT &) implementations.
// Note, that the returned value is that of any data in the dest buffer that
// was not yet transmitted on the outConn.
// The primary justification for adding this method is to allow encodings to
// transmit partial data during the encoding process.  This can improve
// performance considerably for slower (more complex) encoding algorithms.
//inline 
void TrEncoder::EncodeRectBase(const RECT &rect)
{
	m_sizeRaw += (rect.right-rect.left) * (rect.bottom-rect.top) * m_bytesppR;
	this->EncodeRect(rect);
}

void TrEncoder::Out(LPCVOID buffer, int len)
{ 
	m_sizeSent += len;
	m_pOutBuffer->SendExact((LPVOID)buffer, len);
}


void TrEncoder::SetFormats(aaPixelFormat frmL, aaPixelFormat frmR, int cx, int cy)
{
	m_frmR = frmR;
	m_frmL = frmL;

	m_cx = cx;
	m_bytesppR = m_frmR.bitsPerPixel/8;
	m_bytesppL = m_frmL.bitsPerPixel/8;
	m_bytesPerRowL = cx * m_bytesppL;

	// AllocateBuffers(cx, cy);
	{
		ASSERT(cx<32*1024);
		ASSERT(cy<32*1024);

		m_dest.Allocate1(this->RequiredBuffSize(cx, cy));
	}

	TrTranslator::Free(m_translator);
	TrTranslator::Free(m_translatorJPEG);  // reset this converted, it'll be created when it required

	m_translator     = TrTranslator::Init(m_frmL, m_frmR);
}

void TrEncoder::Pack32to24(BYTE* src, BYTE* dst, int pixels)
{
	BYTE* dst_end = dst + pixels*3;

	while (dst<dst_end) {
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = *src++;
				  src++;
	}
}


UINT TrEncoder::ZlibStream::Compress(void* dst, UINT dst_len, const void* src, UINT src_len, int compressionLevel)
{
	if (!m_active){
		m_zlib.total_in = 0;
		m_zlib.total_out = 0;
		m_zlib.zalloc = Z_NULL;
		m_zlib.zfree = Z_NULL;
		m_zlib.opaque = Z_NULL;

		int err = deflateInit2(&m_zlib, compressionLevel, Z_DEFLATED, MAX_WBITS, MAX_MEM_LEVEL, m_zlibStrategy);
		if (err != Z_OK)
			throw RLException("deflateInit2() error %d", err); // %s = this->msg

		m_active = true;
		m_level  = compressionLevel;
	}

	m_zlib.next_in = (Bytef*)src;
	m_zlib.avail_in = src_len;
	m_zlib.next_out = (Bytef*)dst;
	m_zlib.avail_out = dst_len;
	m_zlib.data_type = Z_BINARY;

	if (compressionLevel != m_level) {
		int err = deflateParams(&m_zlib, compressionLevel, m_zlibStrategy);
		if (err != Z_OK)
			throw RLException("deflateParams() error %d", err);
			
		m_level = compressionLevel;
	}

	int err = deflate(&m_zlib, Z_SYNC_FLUSH);
	if (err != Z_OK || m_zlib.avail_in != 0 || m_zlib.avail_out == 0)
		throw RLException("deflate() error %d", err);

	return dst_len - m_zlib.avail_out;
}
