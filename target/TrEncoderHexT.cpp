// This file implements the TrEncoder-derived TrEncoderHexT class.
// This class overrides some TrEncoder functions to produce a
// Hextile encoder.  Hextile splits all top-level update rectangles
// into smaller, 16x16 rectangles and encodes these using
// the optimised Hextile sub-encodings.

#include "stdafx.h"
#include "TrEncoderHexT.h"



TrEncoderHexT::TrEncoderHexT() {}
TrEncoderHexT::~TrEncoderHexT() {}


UINT TrEncoderHexT::RequiredBuffSize(UINT width, UINT height)
{
	//maximp: 6 added for subrectEncode
	return TrEncoder::RequiredBuffSize(width, height) + (((width/16)+1) * ((height/16)+1)) + 6;
}


void TrEncoderHexT::EncodeRect(const RECT &rect)
{
    switch (m_bytesppR)
	{
		case 1: EncodeHextiles(rect, (UINT8*) NULL); break;
		case 2: EncodeHextiles(rect, (UINT16*)NULL); break;
		case 3: EncodeHextiles(rect, (UINT24*)NULL); break;
		case 4: EncodeHextiles(rect, (UINT32*)NULL); break;
		default: 
			throw RLException("Invalid bpp=%u", m_bytesppR);
    }
}

