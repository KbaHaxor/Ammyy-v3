#include <stdafx.h>
#include "VrEncoder.h"
#include "../target/TrEncoderAAC.h"
#include <sstream>
#include <iomanip>


class VrEncoderAAC : public VrEncoders
{
public:

template <typename PIXEL>
void ReadRectAAC(aaFramebufferUpdateRectHeader *pfburh, PIXEL *__typeptr)
{
	BYTE* netbuf = (BYTE*)m_netBuffer.GetBuffer1(pfburh->r.w * pfburh->r.h * sizeof(PIXEL));

	BYTE streamId;
	ReadExact(&streamId, 1);

	switch(streamId)
	{
		//-----------------------------------------------------
		//compactLen--jpegStream
		//-----------------------------------------------------
		case AAC_RECT_JPEG:
		{
			int compressedLen = ReadCompactLen();
			if (compressedLen <= 0)
				throw RLException("VTCT2i encoding: bad data received from server");
			DecompressJpegRect(compressedLen, pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h);
		}
		break;


		case AAC_RECT_INDEXED:
		{
			BYTE nColors;
			ReadExact(&nColors, 1);

			int pixels = pfburh->r.w * pfburh->r.h;
			
			PIXEL* palette = (PIXEL*)m_buffer1AAC.GetBuffer1(sizeof(PIXEL)*nColors + pixels+pixels*sizeof(PIXEL));
			AACStreamDecompress(sizeof(PIXEL)*nColors, (BYTE*)palette, AAC_ZSTREAM_RAW);

			UINT8* decoded = (UINT8*)m_buffer1AAC.GetBuffer() + sizeof(PIXEL)*nColors;
			AACStreamDecompress(pixels, decoded, AAC_ZSTREAM_IDX);
			UINT8* dst = decoded + pixels;
			PIXEL* dst1 = (PIXEL*)dst;

			while (decoded<dst) { // do it "pixel" times;
				UINT8 iColor = *decoded++;
				*dst1++ = (PIXEL)palette[iColor];
			}

			SETPIXELS(dst, pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h);
		}
		break;


		//-----------------------------------------------------
		//COLOR[2]--Z::byte[(w/8)*h]
		//-----------------------------------------------------
		case AAC_RECT_MONO:
		{
			PIXEL clr[2];
			ReadExact(clr, sizeof(clr));

			//decompressed data buffer for zlib is a BGR color's
			int p1 = (pfburh->r.w+7)/8; // byter per row
			int dataLen = p1 * pfburh->r.h;
			UINT8* decoded = (UINT8*)m_buffer1AAC.GetBuffer1(dataLen+sizeof(PIXEL)*pfburh->r.h*pfburh->r.w);
			PIXEL* dst     = (PIXEL*)(decoded+dataLen);
			AACStreamDecompress(dataLen, decoded, AAC_ZSTREAM_MONO);

			PIXEL* dst1 = dst;

			for(int y=0; y<pfburh->r.h; y++) {
			for(int x=0; x<pfburh->r.w; x++) {
				UINT8 b = decoded[ p1*y + (x>>3) ];
				*dst1++ = clr[ (b >> (x&7)) & 1 ];
			}
			}

			SETPIXELS(dst, pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h);
		}
		break;


		case AAC_RECT_SOLID:
		{
			PIXEL clr;
			ReadExact(&clr, sizeof(clr));
			FillSolidRect(pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h, &clr);
		}
		break;


		case AAC_RECT_FULLCOLOR:
		{
			//decompressed data buffer for zlib
			int dataLen = pfburh->r.w * pfburh->r.h* sizeof(PIXEL);
			UINT8* dst = (UINT8*)m_buffer1AAC.GetBuffer1(dataLen);
			AACStreamDecompress(dataLen, dst, AAC_ZSTREAM_RAW);

			SETPIXELS(dst, pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h);
		}
		break;

		default:
			throw RLException("AAC error: invalid rect type");
	}
}


private:
	void AACStreamDecompress(int outDataLen, unsigned char* dataOut, int streamIdx)
	{
		int compressedLen = ReadCompactLen();
		if (compressedLen <= 0)
			throw RLException("VTCT2i encoding: bad data received from server");

		//allocate buffer for compressed data and read the data
		RLStream buffer2(compressedLen);
		Bytef* zb = (Bytef*)buffer2.GetBuffer();
		ReadExact(zb, compressedLen);

		m_streamAAC[streamIdx].Decompress(zb, dataOut, compressedLen, outDataLen);
	}
};



void VrEncoders::ReadRectAAC(aaFramebufferUpdateRectHeader *pfburh)
{
	VrEncoderAAC* ptr = (VrEncoderAAC*)this;
	
	switch (m_frmR.bitsPerPixel) {
		case 8:  { ptr->ReadRectAAC(pfburh, (UINT8*) 0); break; }
		case 16: { ptr->ReadRectAAC(pfburh, (UINT16*)0); break; }
		case 24: { ptr->ReadRectAAC(pfburh, (UINT24*)0); break; }
		case 32: { ptr->ReadRectAAC(pfburh, (UINT32*)0); break; }
	}
}