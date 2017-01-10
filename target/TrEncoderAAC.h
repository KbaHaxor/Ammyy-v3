#ifndef __TR_ENCODER_AAC_H__568_INCLUDED_
#define __TR_ENCODER_AAC_H__568_INCLUDED_


#include "TrEncoder.h"
#include "TrEncoderAACPalette.h"
#include "TrJpegCompressor.h"


#define AAC_RECT_SOLID				100
#define AAC_RECT_MONO				101
#define AAC_RECT_INDEXED			102
#define AAC_RECT_FULLCOLOR			104
#define AAC_RECT_JPEG				105

#define AAC_ZSTREAM_RAW				0
#define AAC_ZSTREAM_MONO			1
#define AAC_ZSTREAM_IDX				2

#define AAC_JPEG_MIN_RECT_SIZE		4096
#define AAC_JPEG_MIN_RECT_CX		8
#define AAC_JPEG_MIN_RECT_CY		8

#define AAC_MAX_HEADER_SIZE			(sz_aaFramebufferUpdateRectHeader + 32)


class TrEncoderAAC : public TrEncoder 
{
public:
	TrEncoderAAC() { }

	~TrEncoderAAC() { }

	//=================================================================================================================================================

	virtual void EncodeRect(const RECT &rect)
	{
		//EncodeRectSimple(rect);
		//return;
		
		int maxSize = m_conf[m_compresslevel].maxRectSize;
		int rectCX = rect.right - rect.left;
		int rectArea  = rectCX * (rect.bottom - rect.top);

		if (rectCX <= 2048 && rectArea <= maxSize / 4) {
			EncodeRectSimple(rect);
			return;
		}

		int maxCX = m_conf[m_compresslevel].maxRectWidth;
		if (maxCX > rectCX) {
			maxCX = rectCX;
		}
		int maxCY = maxSize / maxCX;

		RECT r;

		for (r.top = rect.top; r.top < rect.bottom; r.top += maxCY) 
		{
			r.bottom = min(r.top + maxCY, rect.bottom);
			for (r.left = rect.left; r.left < rect.right; r.left += maxCX) {
				r.right = min( r.left + maxCX, rect.right);
				EncodeRectSimple(r);
			}
		}
	}

	void EncodeRectSimple(const RECT &rect)
	{
		const int rectSize = (rect.right - rect.left) * (rect.bottom - rect.top);

		const int rawDataSize = rectSize * m_bytesppR + 1024;

		m_bufferV.Allocate2(rawDataSize);

		m_headerSize = m_dataSize = 0;
		m_dataBuffer = (BYTE*)m_bufferV.m_buff;

		SendHeader(rect);

		switch(m_frmL.bitsPerPixel){
			case 8:  SendAnyRect(rect, (UINT8*)NULL);  break;
			case 16: SendAnyRect(rect, (UINT16*)NULL); break;
			case 24: SendAnyRect(rect, (UINT24*)NULL); break;
			case 32: SendAnyRect(rect, (UINT32*)NULL); break;
			default:
				throw RLException("AAC: Unsupported bpp");
		}

		if (m_headerSize<=0 || m_dataSize<=0)
			throw RLException("AAC: ERROR#88");

		Out(m_headerBuffer, m_headerSize);
		Out(m_dataBuffer, m_dataSize);
	}

private:
	//=================================================================================================================================================
	template <typename PIXEL>
	void SendAnyRect(const RECT &rect, PIXEL *__typeptr)
	{
		int rectCX = rect.right - rect.left;
		int rectCY = rect.bottom - rect.top;
		int rectSize = rectCX * rectCY;

		int maxColors = rectSize / m_conf[m_compresslevel].idxMaxColorsDivisor;
		if (maxColors < 2){
			if (rectSize >= m_conf[m_compresslevel].monoMinRectSize)
				maxColors = 2;
			else
				maxColors = 1;
		}

		FillPalette(rect, maxColors, __typeptr);
		int numColors = m_Palette.GetNumColors();

		if (numColors == 1){
			SendSolidRect(rect);
		}
		else if (numColors == 2){
			SendMonoRect(rect, __typeptr);
		}
		else if (sizeof(PIXEL) > 1 && numColors != 0){
			SendIndexedRect(rect, __typeptr);
		}		
		else if (sizeof(PIXEL) > 1 &&
			m_qualitylevel!=aaJpegOFF &&
			m_bytesppR >= 2 &&
			rectSize >= AAC_JPEG_MIN_RECT_SIZE &&
			rectCX >= AAC_JPEG_MIN_RECT_CX &&
			rectCY >= AAC_JPEG_MIN_RECT_CY){
			SendJpegRect(rect);
		}		
		else {
			SendFullColorRect(rect, __typeptr);
		}
	}

	//============================================
	void SendHeader(const RECT &rect)
	{
		aaFramebufferUpdateRectHeader* header = (aaFramebufferUpdateRectHeader*)&m_headerBuffer;
		header->encoder = aaEncoderAAC;
		header->r.x = rect.left;
		header->r.y = rect.top;
		header->r.w = rect.right - rect.left;
		header->r.h = rect.bottom - rect.top;

		m_headerSize += sizeof(aaFramebufferUpdateRectHeader);
	}

	//============================================
	template <class PIXEL_T>
	void FillPalette(const RECT &rect, int maxColors, PIXEL_T *p=0)	
	{
		m_Palette.Reset();
		m_Palette.SetMaxColors(maxColors);

		const PIXEL_T *pixels = (const PIXEL_T*)m_source;
		const int cx = m_cx;

		for(int y = rect.top;  y < rect.bottom; y++){
		for(int x = rect.left; x < rect.right;  x++){
			UINT32 pixel = pixels[y * cx + x];
			if (m_Palette.Insert(pixel, 1) == 0) return;
		}
		}
	}	

	//============================================
	void SendSolidRect(const RECT &rect)
	{
		m_dataBuffer[m_dataSize++] = AAC_RECT_SOLID;
		m_translator->Translate(&m_dataBuffer[m_dataSize], this->GetSrcBufferPtr(rect.left, rect.top), 1);
		m_dataSize += m_bytesppR;
	}

	//============================================
	template <typename PIXEL>
	void SendMonoRect(const RECT &rect, PIXEL * __typeptr = 0)
	{
		const int zlibLevel = m_conf[m_compresslevel].monoZlibLevel;

		m_dataBuffer[m_dataSize++] = AAC_RECT_MONO;

		const int rectWidth = rect.right - rect.left;
		const int rectHeight = rect.bottom - rect.top;

		int dataLen = (rectWidth + 7) / 8 * rectHeight;

		PIXEL palette[2] = {
			(PIXEL)m_Palette.GetEntry(0),
			(PIXEL)m_Palette.GetEntry(1)
		};		

		m_translator->Translate(&m_dataBuffer[m_dataSize], &palette[0], 2);
		m_dataSize += m_bytesppR * 2;

		m_buffer1.SetMinCapasity(dataLen);
		unsigned char* encoded = (unsigned char*)m_buffer1.GetBuffer();
		EncodeMonoRect(rect, encoded, (PIXEL*)NULL);
		SendCompressed(encoded, dataLen, AAC_ZSTREAM_MONO, zlibLevel);
	}

	//============================================
	template <typename PIXEL>
	void EncodeMonoRect(const RECT &rect, BYTE *dst, PIXEL *__typeptr)
	{
		const UINT8* src = this->GetSrcBufferPtr(rect.left, rect.top);

		const int cx = rect.right - rect.left;
		const int cy = rect.bottom - rect.top;

		const PIXEL bg = (PIXEL)m_Palette.GetEntry(0);

		for(int y = 0; y < cy; y++)
		{
			for(int x = 0; x < cx; x++)
			{
				if( (x&7)==0 )
					*dst = 0;

				if (*((PIXEL*)&src[y*m_bytesPerRowL + x*m_bytesppL]) != bg)
					*dst |= 1<<(x&7);

				if( (x&7)==7 )
					dst++;

			}
			if( (cx&7) )
				dst++;
		}

	}

	//======================================================================================================================================
	void SendCompressed(const BYTE *data, size_t dataLen, int streamId, int compressionLevel)
	{
		/* it saves about 0.02%, but makes code more complex
		if (dataLen < 12) {
			memcpy(&m_dataBuffer[m_dataSize], data, dataLen);
			m_dataSize += dataLen;
			return;
		}*/

		size_t dstSize = dataLen + dataLen / 100 + 16;
		RLStream compressedDataWrapper;
		BYTE *dst = (BYTE*)compressedDataWrapper.GetBuffer1(dstSize);
		size_t compressedLength = m_zsStruct[streamId].Compress(dst, dstSize, data, dataLen, compressionLevel);
		
		SendCompactLength(compressedLength);
		memcpy(&m_dataBuffer[m_dataSize], dst, compressedLength);
		m_dataSize += compressedLength;
	}

	//============================================
	void SendCompactLength(size_t length)
	{
		m_dataBuffer[m_dataSize++] = length & 0x7f;
		if (length > 0x7f){
			m_dataBuffer[m_dataSize - 1] |= 0x80;
			m_dataBuffer[m_dataSize++] = (length >> 7) & 0x7f;
			if (length > 0x3fff){
				m_dataBuffer[m_dataSize - 1] |= 0x80;
				m_dataBuffer[m_dataSize++] = (length >> 14) & 0xff;
			}
		}
	}

	//============================================
	template <typename PIXEL>
	void SendIndexedRect(const RECT &rect, PIXEL * __typeptr = 0)
	{
		m_dataBuffer[m_dataSize++] = AAC_RECT_INDEXED;

		size_t numColors = m_Palette.GetNumColors();
		m_dataBuffer[m_dataSize++] = numColors;

		PIXEL palette[256];
		for (int i=0; i<numColors; i++) palette[i] = (PIXEL)m_Palette.GetEntry(i);

		int size1 = numColors * m_bytesppR;
		BYTE* c = (BYTE*)m_buffer1.GetBuffer1(size1);
		m_translator->Translate(c, &palette[0], numColors);
		SendCompressed(c, size1, AAC_ZSTREAM_RAW, m_conf[m_compresslevel].idxZlibLevel);

		int pixels = (rect.right - rect.left) * (rect.bottom - rect.top);
		m_buffer1.SetMinCapasity(pixels);
		BYTE* dst = (BYTE*)m_buffer1.GetBuffer();
		EncodeIndexedRect(rect, dst, (PIXEL*)NULL);
		SendCompressed(dst, pixels, AAC_ZSTREAM_IDX, m_conf[m_compresslevel].idxZlibLevel);
	}

	//============================================
	template <typename PIXEL>
	void EncodeIndexedRect(const RECT &rect, BYTE* dst, PIXEL * __typeptr = 0)
	{
		const PIXEL* src = (const PIXEL*)this->GetSrcBufferPtr(rect.left, rect.top);		

		const int cx = rect.right - rect.left;
		const int cy = rect.bottom - rect.top;

		const int skipPixels = m_cx - cx;

		for(int y=0; y<cy; y++) {
		for(int x=0; x<cx; x++) {
			PIXEL rgb = *src++;
			*dst++ = m_Palette.GetIndex(rgb);
		}
		src += skipPixels;
		}
	}

	//============================================
	void SendJpegRect(const RECT &rect)
	{		
		m_compressor.SetQuality(m_qualitylevel);

		const void *ptr = this->GetSrcBufferPtr(rect.left, rect.top);

		const int cx = rect.right - rect.left;
		const int cy = rect.bottom - rect.top;

		if (m_translatorJPEG==NULL) 
			m_translatorJPEG = TrTranslator::Init(m_frmL, TrTranslator::f_r8g8b8);

		m_compressor.Compress(ptr, this->m_translatorJPEG, cx, cy, m_bytesPerRowL);
		size_t dataLength = m_compressor.GetOutputLength();

		m_dataBuffer[m_dataSize++] = AAC_RECT_JPEG;
		SendCompactLength(dataLength);
		memcpy(&m_dataBuffer[m_dataSize], m_compressor.GetOutputBuffer(), m_compressor.GetOutputLength());
		m_dataSize += m_compressor.GetOutputLength();
	}

	//============================================
	template <typename PIXEL>
	void SendFullColorRect(const RECT &rect, PIXEL *__typeptr = 0)
	{
		m_dataBuffer[m_dataSize++] = AAC_RECT_FULLCOLOR;

		int cx = rect.right - rect.left;
		int cy = rect.bottom - rect.top;
		int dstLength = cx * cy * m_bytesppR;

		void *dst = m_buffer1.GetBuffer1(dstLength);

		this->Translate(dst, rect);

		SendCompressed((BYTE*)dst, dstLength, AAC_ZSTREAM_RAW, m_conf[m_compresslevel].rawZlibLevel);
	}

	//============================================
	
private:
	static const struct Conf {
		int		maxRectSize;
		int		maxRectWidth;
		int		monoMinRectSize;
		int		idxZlibLevel;
		int		monoZlibLevel;
		int		rawZlibLevel;
		int		idxMaxColorsDivisor;
	} m_conf[10];

	TrEncoderAACPalette	m_Palette;

	RLStream			m_buffer1; // additional buffer

	BYTE				m_headerBuffer[AAC_MAX_HEADER_SIZE];
	UINT				m_headerSize;

	BYTE*				m_dataBuffer;
	UINT				m_dataSize;

	ZlibStream			m_zsStruct[3];

	TrJpegCompressor	m_compressor;
};


#endif // __TR_ENCODER_AAC_H__568_INCLUDED_
