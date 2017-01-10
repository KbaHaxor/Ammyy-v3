#ifndef __TR_JPEG_COMPRESSOR_A457EF_H_INCLUDED__
#define __TR_JPEG_COMPRESSOR_A457EF_H_INCLUDED__

extern "C" {
#include "../common/libjpeg/jpeglib.h"
}

#include "../main/aaDesktop.h"
#include "TrTranslate.h"

class TrJpegCompressor
{
public:
	TrJpegCompressor();
	virtual ~TrJpegCompressor();

	void SetQuality(int level);
	//void ResetQuality(){ m_newQuality = DEFAULT_JPEG_QUALITY; }

	void Compress(const void *src, TrTranslator* translator, int w, int h, int bytesPerRowSrc);

	inline BYTE* GetOutputBuffer() { return m_outputBuffer  + GetHeaderSize(); }
	inline UINT  GetOutputLength() { return m_numBytesReady - GetHeaderSize(); }


	static UINT GetHeaderSize() { return 319; }
	static void FillHeader(RLStream& header, int quality);
	static void SetHeader(void* pHeader, int cx, int cy);


public:
	void initDestination();
	bool emptyOutputBuffer();
	void termDestination();

private:
	static const int ALLOC_CHUNK_SIZE;
	static const int DEFAULT_JPEG_QUALITY;

	BYTE* m_outputBuffer;

	int m_quality;
	int m_newQuality;

	struct jpeg_compress_struct m_cinfo;
	struct jpeg_error_mgr m_jerr;
	
	size_t m_numBytesAllocated;
	size_t m_numBytesReady;
};

#endif 
