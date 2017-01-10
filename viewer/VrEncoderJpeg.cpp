#include "stdafx.h"
#include "vrMain.h"
#include "vrClient.h"
#include "../target/TrJpegCompressor.h"


#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


class VrEncoderJpeg
{
public:
	// the "Source manager" for the JPEG library.

	static void JpegInitSource(j_decompress_ptr cinfo)
	{
		//VrEncoders::JPEG* _this = (VrEncoders::JPEG*)cinfo->src;
	}

	static boolean JpegFillInputBuffer(j_decompress_ptr cinfo)
	{
		throw RLException("called JpegFillInputBuffer");
	}

	static void JpegSkipInputData(j_decompress_ptr cinfo, long num_bytes)
	{
		throw RLException("called JpegSkipInputData");
	}

	static void JpegTermSource(j_decompress_ptr cinfo)
	{
	}

	// the "Error handlers"

	static void throw_exception(j_common_ptr cinfo)
	{
		//VrEncoderJpegError* myerr = (VrEncoderJpegError*)cinfo->err;

		char buffer[JMSG_LENGTH_MAX];
		(cinfo->err->format_message) (cinfo, buffer);
		throw RLException("%s", buffer); // or cinfo->err->msg_code
	}

	static void emit_message(j_common_ptr cinfo, int msg_level)
	{
		if (msg_level >= 0)	return;
		throw_exception(cinfo);
	}

	static void error_exit(j_common_ptr cinfo)
	{
		//Control must NOT return to the caller; generally this routine will exit() or longjmp() somewhere
		throw_exception(cinfo);
	}
};


void VrEncoders::DecompressJpegRect(int compressedLen, int x, int y, int w, int h)
{
	if (m_translatorJPEG==NULL) {
		m_translatorJPEG = TrTranslator::Init(TrTranslator::f_r8g8b8, m_frmL);
	
		TrJpegCompressor::FillHeader(m_jpeg.header, m_jpeg.quality);

		if (m_jpeg.header.GetLen()==0)
			throw RLException("VrEncoderJPEG %u", __LINE__);
	}

	TrJpegCompressor::SetHeader(m_jpeg.header.GetBuffer(), w, h);

	int header_len = m_jpeg.header.GetLen();

	char* buffer1 = (char*)m_netBuffer.GetBuffer1(compressedLen + header_len);
	memcpy(buffer1, m_jpeg.header.GetBuffer(), header_len);
	ReadExact(buffer1 + header_len, compressedLen);

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	//setup error handling
	jpeg_std_error(&jerr);
	jerr.error_exit	  = VrEncoderJpeg::error_exit;
	jerr.emit_message = VrEncoderJpeg::emit_message;
	cinfo.err = &jerr;

	try {	
		jpeg_create_decompress(&cinfo);

		m_jpeg.jpegBufferPtr = (JOCTET *)buffer1;
		m_jpeg.jpegBufferLen = (size_t)compressedLen + header_len;
		m_jpeg.jpegSrcManager.init_source       = VrEncoderJpeg::JpegInitSource;
		m_jpeg.jpegSrcManager.fill_input_buffer = VrEncoderJpeg::JpegFillInputBuffer;
		m_jpeg.jpegSrcManager.skip_input_data   = VrEncoderJpeg::JpegSkipInputData;
		m_jpeg.jpegSrcManager.term_source       = VrEncoderJpeg::JpegTermSource;
		m_jpeg.jpegSrcManager.resync_to_restart = jpeg_resync_to_restart;
		m_jpeg.jpegSrcManager.next_input_byte   = m_jpeg.jpegBufferPtr;
		m_jpeg.jpegSrcManager.bytes_in_buffer   = m_jpeg.jpegBufferLen;
		cinfo.src = (jpeg_source_mgr*)&this->m_jpeg;

		jpeg_read_header(&cinfo, TRUE);
		cinfo.out_color_space = JCS_RGB;

		jpeg_start_decompress(&cinfo);
		if ((int)cinfo.output_width != w || (int)cinfo.output_height != h || cinfo.output_components != 3) 
			throw RLException("JPEG invalid dimension %u %u %u %u", (int)cinfo.output_width, (int)cinfo.output_height, (int)w, (int)h);

		//RLMutexLock l(m_bitmapdcMutex);

		BYTE* src = (BYTE*)m_zlibBuffer.GetBuffer1(w*4); // for one scanlines, TODO: may be *3

		JSAMPROW rowPointer[1] = { (JSAMPROW)src };

		for (int y1=y; cinfo.output_scanline < cinfo.output_height; y1++) 
		{
			jpeg_read_scanlines(&cinfo, rowPointer, 1);

			this->SetPixelsOneRow(m_translatorJPEG, src, x, y1, w);
		}

		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
	}
	catch(RLException& ex) 
	{
		jpeg_destroy_decompress(&cinfo);
		throw RLException("Error '%s' in DecompressJpegRect()", ex.GetDescription());
	}
}


/*
void VrEncoders::ReadJpegRect(aaFramebufferUpdateRectHeader *pfburh)
{
	aaJpegHeader jheader;
	ReadExact(&jheader, sizeof(jheader));

	if (jheader.nBytes==0) return;

	DecompressJpegRect(jheader.nBytes, pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h);
}
*/