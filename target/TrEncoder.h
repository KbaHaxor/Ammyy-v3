// The TrEncoder object encodes regions of a display buffer

#if !defined(_TR_ENCODER__H__345C12EA__INCLUDED_)
#define _TR_ENCODER__H__345C12EA__INCLUDED_

#include "TrTranslate.h"
#include "../main/Transport.h"
#include "../common/zlib/zlib.h"

class TrDesktop;

class TrEncoder
{
public:
	class ZlibStream
	{
	public:
		ZlibStream() 
		{
			m_active = false;
			m_zlibStrategy = Z_DEFAULT_STRATEGY;
		}

		~ZlibStream()
		{
			if (m_active){
				deflateEnd(&m_zlib);
				m_active = false;
			}
		}

		UINT Compress(void* dst, UINT dst_len, const void* src, UINT src_len, int compressionLevel);

	public:
		int  m_zlibStrategy;

	private:
		z_stream m_zlib;
		bool m_active;
		int  m_level;		
	};

	class TrVector
	{
	public:
		TrVector();
		~TrVector();
		void Allocate1(UINT size);
		void Allocate2(UINT size);

		BYTE* m_buff;
		UINT  m_size;
	};
public:
	TrEncoder();
	virtual ~TrEncoder();

	// A method to return the encoding name, used by the LogStats() method
	virtual const char* GetEncodingName() { return "Raw"; }

	virtual void LogStats();	// Central method for outputing encoding statistics

	virtual void OnBegin() {}; // Encoder stats used by the buffer object

	void EncodeRectBase(const RECT &rect);

	// Translation handling
	void SetFormats(aaPixelFormat frmL, aaPixelFormat frmR, int cx, int cy);

	static __forceinline void CopyPixelDDB(void* dst, const void* src, int bytesPP)
	{		
		switch(bytesPP) {
			case 4: { *(UINT32*)dst = *(UINT32*)src; break; }
			case 3: { *(UINT24*)dst = *(UINT24*)src; break; }
			case 2: { *(UINT16*)dst = *(UINT16*)src; break; }
			case 1: { *(UINT8* )dst = *(UINT8*) src; break; }
		}
	}


protected:
			void Out(LPCVOID buffer, int len);
			void Translate (void *dest, const RECT &rect);
	virtual void EncodeRect (const RECT &rect);
	virtual UINT RequiredBuffSize(UINT cx, UINT cy);

	inline BYTE* GetSrcBufferPtr(int x, int y) { return m_source + (m_bytesPerRowL * y)+(x * m_bytesppL); }
	static void Pack32to24(BYTE* src, BYTE* dst, int pixels);
	inline static void Pack32to24(BYTE* ptr, int pixels) { Pack32to24(ptr, ptr, pixels); }


public:
	Transport* m_pOutBuffer;
	BYTE* m_source;			// is the base address of the ENTIRE SCREEN buffer for encoding

protected:
	TrVector m_dest;

	// used in TrEncoderRRE, TrEncoderCoRRE, TrEncoderTight, TrEncoderZlib
	// it makes bigger exe, if we place it in children
	TrVector	m_bufferV;


public:
	TrTranslator* 	m_translator;
protected:
	TrTranslator* 	m_translatorJPEG;
public:
	aaPixelFormat		m_frmL;					// Local pixel format
	aaPixelFormat		m_frmR;					// Remote format what we send, Client pixel format info, 
	int					m_bytesppR;				// Bytes per pixel Remote
	int					m_bytesppL;				// Bytes per pixel Local - m_frmL
	
protected:
	int					m_cx;					// width of screen
	int					m_bytesPerRowL;			// Number of bytes per row FOR m_frmL
	
	int					m_sizeSent;				// Total amount of data sent
	int					m_sizeRaw;				// Total size of raw data encoded

public:
	int					m_compresslevel;		// Encoding-specific compression level (if needed).
	int					m_qualitylevel;			// Image quality level for lossy JPEG compression, "-1" - is off


	friend class CAmmyyApp;
};

#endif // _TR_ENCODER__H__345C12EA__INCLUDED_
