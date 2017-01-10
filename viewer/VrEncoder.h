#ifndef _VR_ENCODERS_H__INCLUDED_
#define _VR_ENCODERS_H__INCLUDED_

#include "../main/InteropViewer.h"
#include "../main/aaDesktop.h"
#include "../common/zlib/zlib.h"
#include "../target/TrTranslate.h"

extern "C" {
#include "../common/libjpeg/jpeglib.h"
}

class VrEncoders: public InteropViewer
{
public:
	class ZlibStream
	{
	public:
		ZlibStream() 
		{
			m_active = false;
		}

	private:
		inline void Init()
		{
			if (!m_active) {
				m_zlib.total_in = 0;
				m_zlib.total_out = 0;
				m_zlib.zalloc = Z_NULL;
				m_zlib.zfree  = Z_NULL;
				m_zlib.opaque = Z_NULL;
				int err = inflateInit(&m_zlib);
				if (err != Z_OK) {
					LPCSTR err_str = (m_zlib.msg != NULL) ? m_zlib.msg : "(NULL)";
					throw RLException("zlib inflateInit() error: %d %s", err, err_str);
				}
				m_active = true;
			}
		}

	public:
		int Decompress(void* bufferIn, void* bufferOut, UINT sizeIn, UINT sizeOut)
		{
			// Insure the inflator is initialized
			this->Init();

			m_zlib.next_in   = (BYTE*)bufferIn;
			m_zlib.avail_in  = (UINT)sizeIn;
			m_zlib.next_out  = (BYTE*)bufferOut;
			m_zlib.avail_out = (UINT)sizeOut;
			m_zlib.data_type = Z_BINARY;

			int inflateResult = inflate( &m_zlib, Z_SYNC_FLUSH );
			if ( inflateResult < 0 )
				throw RLException("zlib inflate error: %d", inflateResult);

			return sizeOut - m_zlib.avail_out;
		}

		void DeInit()
		{
			if (m_active) {
				int err = inflateEnd(&m_zlib);
				if (err != Z_OK) {
					LPCSTR err_str = (m_zlib.msg != NULL) ? m_zlib.msg : "(NULL)";
					throw RLException("zlib inflateEnd() error: %d %s", err, err_str);
				}
				m_active = false;
			}
		}

		z_stream m_zlib;
	private:
		bool m_active;
	};

public:
	VrEncoders();
	virtual ~VrEncoders();

	virtual void InvalidateScreenRect(int x, int y, int cx, int cy) = 0;

	aaPixelFormat m_frmR; // format in which Target sends data
	aaPixelFormat m_frmR_org; // original format of Target
	aaPixelFormat m_frmL; // format of local bitmap

protected:
	void ResetEncoders();
	void ResetTranslators();
	int  ReadCompactLen(); // for AAC & Tight
	void DecompressJpegRect(int len, int x, int y, int w, int h); // for AAC & Tight


	void ReadExact(void *buf, int bytes);
	void ReadRawRect(aaFramebufferUpdateRectHeader *pfburh);
	void ReadRRERect(aaFramebufferUpdateRectHeader *pfburh);
	void ReadCoRRERect(aaFramebufferUpdateRectHeader *pfburh);
	void ReadZlibRect(aaFramebufferUpdateRectHeader *pfburh);
	void ReadJpegRect(aaFramebufferUpdateRectHeader *pfburh);
	void ReadRectAAC(aaFramebufferUpdateRectHeader *pfburh);

	// VrEncoderZlibHex.cpp
	/*
	void ReadZlibHexRect(aaFramebufferUpdateRectHeader *pfburh);
private:
	void HandleZlibHexEncoding8 (int x, int y, int w, int h);
	void HandleZlibHexEncoding16(int x, int y, int w, int h);
	void HandleZlibHexEncoding32(int x, int y, int w, int h);
	void HandleZlibHexSubencodingStream8 (int x, int y, int w, int h, int subencoding);
	void HandleZlibHexSubencodingStream16(int x, int y, int w, int h, int subencoding);
	void HandleZlibHexSubencodingStream32(int x, int y, int w, int h, int subencoding);
	void HandleZlibHexSubencodingBuf8 (int x, int y, int w, int h, int subencoding, unsigned char * buffer);
	void HandleZlibHexSubencodingBuf16(int x, int y, int w, int h, int subencoding, unsigned char * buffer);
	void HandleZlibHexSubencodingBuf32(int x, int y, int w, int h, int subencoding, unsigned char * buffer);	
	*/

	// VrEncoderJpeg.cpp
public:
	struct JPEG
	{
		jpeg_source_mgr jpegSrcManager;
		//bool	jpegError;
		JOCTET* jpegBufferPtr;
		size_t  jpegBufferLen;
		int		quality;
		RLStream header;
	} m_jpeg;

	void SetQualityLevel(int quality)
	{
		m_jpeg.quality = quality;
		m_jpeg.header.Reset();
	}	
	
	// VrEncoderTight.cpp
/*
protected:
	void ReadTightRect(aaFramebufferUpdateRectHeader *pfburh);
private:
	int InitFilterCopy (int rw, int rh);
	int InitFilterGradient (int rw, int rh);
	int InitFilterPalette (int rw, int rh);
	void FilterCopy8 (int numRows);
	void FilterCopy16 (int numRows);
	void FilterCopy24 (int numRows);
	void FilterCopy32 (int numRows);
	void FilterGradient8 (int numRows);
	void FilterGradient16 (int numRows);
	void FilterGradient24 (int numRows);
	void FilterGradient32 (int numRows);
	void FilterPalette (int numRows);

	// Variables used by VrEncoderTight.cpp
	// Four independent compression streams for zlib library.
	ZlibStream m_tightZlibStream[4];

	// Tight filter stuff. Should be initialized by filter initialization code.
	tightFilterFunc m_tightCurrentFilter;
	bool m_tightCutZeros;
	int m_tightRectWidth, m_tightRectColors;
	COLORREF m_tightPalette[256];
	UINT8 m_tightPrevRow[2048*3*sizeof(UINT16)];
*/


	// VrEncoderAAC.cpp
protected:
	ZlibStream m_streamAAC[3];
	RLStream m_buffer1AAC;

	// VrEncoderHexT.cpp
protected:
	void ReadHextileRect(aaFramebufferUpdateRectHeader *pfburh);

protected:

	__forceinline void* GetScreenPtr(int x, int y, int bytesPP)
	{
		int offset1 = (m_desktop_cy - y -1)*m_desktop_cx + x;
		return ((BYTE*)m_pvBits) + offset1*bytesPP;
	}

	__forceinline void CopyPixelDDB(void* dst, const void* src)
	{
		*(UINT32*)dst = *(UINT32*)src;

		// TODO: not supported now
		/*
		switch(m_bytesppL) {
			case 4: { *(UINT32*)dst = *(UINT32*)src; break; }
			case 3: { *(UINT24*)dst = *(UINT24*)src; break; } // TODO: not supported
			case 2: { *(UINT16*)dst = *(UINT16*)src; break; }
		}//*/
	}


	__forceinline void SetPixelDDB(int x, int y, const void* ptr)
	{
		void* dst = this->GetScreenPtr(x, y, m_bytesppL);
		CopyPixelDDB(dst, ptr);
	}

	__forceinline void SETPIXELS(void* buffer, int x, int y, int w, int h)
	{
		BYTE* src = (BYTE*)buffer;
		BYTE* dst = (BYTE*)this->GetScreenPtr(x, y, m_bytesppL);
		int   dst_inc = m_bytesppL * m_desktop_cx;
		int   src_inc = m_bytesppR * w;
		while (h>0) {
			m_translator->Translate(dst, src, w);
			if (--h==0) break; // check it here to avoid dst & src increament on last loop
			dst -= dst_inc;
			src += src_inc;
		}
	}

	// one line only
	__forceinline void SetPixelsOneRow(TrTranslator* translator, void* src, int x, int y, int w)
	{
		void* dst = this->GetScreenPtr(x, y, m_bytesppL);
		translator->Translate(dst, src, w);
	}


private:
	template<typename PIXEL>
	void __forceinline FillSolidRectDDB(int x, int y, int w, int h, PIXEL* pColor)
	{
		PIXEL c =  *pColor;

		PIXEL* p = (PIXEL*)GetScreenPtr(x, y, sizeof(PIXEL));
		int skip = m_desktop_cx + w;

		int y2 = y+h;
		for (int y0 = y; y0<y2; y0++) {
			PIXEL* p_end = p + w;
			while (p<p_end) *p++ = c;
			p-=skip;
		}
	}

protected:
	// These draw a solid rectangle of colour on the bitmap
	void __fastcall FillSolidRect(int x, int y, int w, int h, void* pColor)
	{
		UINT32 ddbColor; // max size 32 bit
		m_translator->Translate(&ddbColor, pColor, 1);

		switch(m_bytesppL) {
			case 4: FillSolidRectDDB(x, y, w, h, (UINT32*)&ddbColor); break;
			//case 3: FillSolidRectDDB(x, y, w, h, (UINT24*)&ddbColor); break;
			//case 2: FillSolidRectDDB(x, y, w, h, (UINT16*)&ddbColor); break;
		}
	};
	
protected:
	TrTranslator* 	m_translator;
	TrTranslator* 	m_translatorJPEG;
	
	RLStream    m_zlibBuffer; // Buffer for zlib decompression.	
	RLStream	m_netBuffer;
	//ZlibStream m_decompStream;

	UINT		 m_bytesppR;		// Bytes per pixel Remote ( Client)
	UINT		 m_bytesppL;		// Bytes per pixel Local

	UINT32		m_ddbColors[2];		// special color in DDB format used for cursor

	// Bitmap for local copy of screen, and DC for writing to it.
	RLMutex		m_bitmapdcMutex;
	HDC			m_hBitmapDC;
	HBITMAP		m_hBitmap;
	HBITMAP		m_hBitmapOld;
	int			m_desktop_cx;
	int			m_desktop_cy;
	VOID*		m_pvBits;		// pointer to DIB

public:
	/*
	void SaveScreen(LPCSTR filename) 
	{
		this->SaveScreen(filename, 0, 0, m_desktop_cx, m_desktop_cy);
	}
	void SaveScreen(LPCSTR filename, int x, int y, int w, int h)
	{
		//HDC m_hBitmapDC, HBITMAP m_hBitmap;

		RECT rect;
		rect.left = x;
		rect.right = x+w;
		rect.top = y;
		rect.bottom = y+h;


		HDC winDC	= m_hBitmapDC;

		 // bitmap dimensions
		int bitmap_dx = rect.right - rect.left;
		int bitmap_dy = rect.bottom - rect.top;

		// create file
		FILE* file = fopen(filename, "wb");
		if(file==NULL) return;

		// save bitmap file headers
		BITMAPFILEHEADER fileHeader;
		BITMAPINFOHEADER infoHeader;

		fileHeader.bfType      = 0x4d42;
		fileHeader.bfSize      = 0;
		fileHeader.bfReserved1 = 0;
		fileHeader.bfReserved2 = 0;
		fileHeader.bfOffBits   = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		infoHeader.biSize          = sizeof(infoHeader);
		infoHeader.biWidth         = bitmap_dx;
		infoHeader.biHeight        = bitmap_dy;
		infoHeader.biPlanes        = 1;
		infoHeader.biBitCount      = 32;
		infoHeader.biCompression   = BI_RGB;
		infoHeader.biSizeImage     = 0;
		infoHeader.biXPelsPerMeter = 0;
		infoHeader.biYPelsPerMeter = 0;
		infoHeader.biClrUsed       = 0;
		infoHeader.biClrImportant  = 0;

		fwrite((char*)&fileHeader, 1, sizeof(fileHeader), file);
		fwrite((char*)&infoHeader, 1, sizeof(infoHeader), file);

		// dibsection information
		BITMAPINFO info;
		info.bmiHeader = infoHeader; 

		// create a dibsection and blit the window contents to the bitmap
		// HDC winDC = GetWindowDC(window);

		HGDIOBJ hOldObj = ::SelectObject(winDC, m_hBitmap);

		HDC memDC = CreateCompatibleDC(winDC);
		BYTE* memory = 0;
		HBITMAP bitmap = CreateDIBSection(winDC, &info, DIB_RGB_COLORS, (void**)&memory, 0, 0);
		SelectObject(memDC, bitmap);
		BitBlt(memDC, 0, 0, bitmap_dx, bitmap_dy, winDC, rect.left, rect.top, SRCCOPY);
		DeleteDC(memDC);

		// save dibsection data
		int bytes = (((32*bitmap_dx + 31) & (~31))/8)*bitmap_dy;
		fwrite(memory, 1, bytes, file);
		fclose(file);

		::DeleteObject(bitmap);
		::SelectObject(winDC, hOldObj);
	}
	*/
};



#endif