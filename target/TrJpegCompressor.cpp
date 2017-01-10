#include <stdafx.h>
#include "TrJpegCompressor.h"

const int TrJpegCompressor::ALLOC_CHUNK_SIZE = 65536;
const int TrJpegCompressor::DEFAULT_JPEG_QUALITY = 75;

typedef struct {
	struct jpeg_destination_mgr pub;
	TrJpegCompressor *_this;
} my_destination_mgr;

static void init_destination(j_compress_ptr cinfo)
{
	my_destination_mgr *dest_ptr = (my_destination_mgr *)cinfo->dest;
	dest_ptr->_this->initDestination();
}

static boolean empty_output_buffer (j_compress_ptr cinfo)
{
	my_destination_mgr *dest_ptr = (my_destination_mgr *)cinfo->dest;
	return (boolean)dest_ptr->_this->emptyOutputBuffer();
}

static void term_destination (j_compress_ptr cinfo)
{
	my_destination_mgr *dest_ptr = (my_destination_mgr *)cinfo->dest;
	dest_ptr->_this->termDestination();
}

TrJpegCompressor::TrJpegCompressor()
  : m_quality(-1), 
    m_newQuality(DEFAULT_JPEG_QUALITY),
    m_outputBuffer(0),
    m_numBytesAllocated(0),
    m_numBytesReady(0)
{
	m_cinfo.err = jpeg_std_error(&m_jerr);
	jpeg_create_compress(&m_cinfo);

	my_destination_mgr *dest = new my_destination_mgr;
	dest->pub.init_destination = init_destination;
	dest->pub.empty_output_buffer = empty_output_buffer;
	dest->pub.term_destination = term_destination;
	dest->_this = this;
	
	m_cinfo.dest = (jpeg_destination_mgr *)dest;
	m_cinfo.input_components = 3;
	m_cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&m_cinfo);

	m_cinfo.dct_method = JDCT_FASTEST;
	//m_cinfo.dct_method = JDCT_ISLOW;
	jpeg_set_colorspace(&m_cinfo, JCS_RGB);
	//jpeg_set_colorspace(&cinfo, JCS_YCbCr);

	m_cinfo.write_Adobe_marker = FALSE;
}

TrJpegCompressor::~TrJpegCompressor()
{
	if (m_outputBuffer)
		free(m_outputBuffer);

	delete m_cinfo.dest;
	m_cinfo.dest = NULL;

	jpeg_destroy_compress(&m_cinfo);
}

void TrJpegCompressor::initDestination()
{
	if (!m_outputBuffer) {
		size_t newSize = ALLOC_CHUNK_SIZE;
		m_outputBuffer = (unsigned char *)malloc(newSize);
		m_numBytesAllocated = newSize;
	}

	m_numBytesReady = 0;
	m_cinfo.dest->next_output_byte = m_outputBuffer;
	m_cinfo.dest->free_in_buffer =  m_numBytesAllocated;
}

bool TrJpegCompressor::emptyOutputBuffer()
{
	size_t oldSize = m_numBytesAllocated;
	size_t newSize = oldSize + ALLOC_CHUNK_SIZE;

	m_outputBuffer = (unsigned char *)realloc(m_outputBuffer, newSize);
	m_numBytesAllocated = newSize;

	m_cinfo.dest->next_output_byte = &m_outputBuffer[oldSize];
	m_cinfo.dest->free_in_buffer = newSize - oldSize;

	return true;
}

void TrJpegCompressor::termDestination()
{
	m_numBytesReady = m_numBytesAllocated - m_cinfo.dest->free_in_buffer;
}

void TrJpegCompressor::SetQuality(int level)
{
	if (level < 0 || level > 100)
		throw RLException("SetQuality() invalid value");

	m_newQuality = level;
}

void TrJpegCompressor::Compress(const void *_src, TrTranslator* translator, int w, int h, int bytesPerRowSrc)
{
	m_cinfo.image_width = w;
	m_cinfo.image_height = h;

	if (m_newQuality != m_quality) {
		jpeg_set_quality(&m_cinfo, m_newQuality, true);
		m_quality = m_newQuality;
	}

	jpeg_start_compress(&m_cinfo, TRUE);

	const char *src = (const char *)_src;

	RLStream dstWrapper;

	JSAMPLE *rgb = (JSAMPLE*)dstWrapper.GetBuffer1(w * 3 * 8);
	JSAMPROW rowPointer[8];
	for (int i = 0; i < 8; i++)
		rowPointer[i] = &rgb[w * 3 * i];

	while (m_cinfo.next_scanline < m_cinfo.image_height) {
		int maxRows = m_cinfo.image_height - m_cinfo.next_scanline;
		if (maxRows > 8) 
			maxRows = 8;
		
		for (int dy = 0; dy < maxRows; dy++)
		{
			translator->Translate(rowPointer[dy], src, w);
			src += bytesPerRowSrc;
		}
		jpeg_write_scanlines(&m_cinfo, rowPointer, maxRows);
	}

	jpeg_finish_compress(&m_cinfo);
}


void TrJpegCompressor::FillHeader(RLStream& header, int quality)
{
	UINT32 pixel = 0;

	TrTranslator* translator = TrTranslator::Init(TrTranslator::f_x8r8g8b8, TrTranslator::f_r8g8b8, true);

	TrJpegCompressor m_compressor;
	m_compressor.SetQuality(quality);
	m_compressor.Compress(&pixel, translator, 1, 1, sizeof(pixel));

	TrTranslator::Free(translator);

	header.Reset();
	header.AddRaw(m_compressor.m_outputBuffer, GetHeaderSize());
}


void TrJpegCompressor::SetHeader(void* pHeader, int cx, int cy)
{
	*((UINT16*)pHeader+76/2) = ((cy&0xFF)<<8) | (cy>>8);	//little- big-endian conversion
	*((UINT16*)pHeader+78/2) = ((cx&0xFF)<<8) | (cx>>8);
}
