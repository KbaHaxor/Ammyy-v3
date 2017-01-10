#include "stdafx.h"
#include "RLBase64Coder.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

RLBase64Coder::RLBase64Coder(LPCSTR key)
{
	if (key==NULL) key="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; // standard key
	memcpy(m_key, key, 64);
};


// not reset pStreamOut
//
void RLBase64Coder::Encode(RLStream* pStreamIn, RLStream* pStreamOut)
{	
	DWORD dwLenIn     = pStreamIn->GetLen();
	BYTE* pDataIn     = (BYTE*)pStreamIn->GetBuffer();
	BYTE* pDataInEnd  = pDataIn + dwLenIn;
	
	pStreamOut->SetMinExtraCapasity(dwLenIn/3*4+3);
	BYTE* pDataOutBeg = (BYTE*)pStreamOut->GetBuffer();
	BYTE* pDataOut    = pDataOutBeg + pStreamOut->GetLen();

	DWORD accum = 0;
	DWORD shift = 0;	// # of excess bits stored in accum
	
	while(pDataIn<pDataInEnd)
	{
		accum <<= 8;
		accum += *pDataIn++;
		shift += 8;

		do 
		{
			shift -= 6;

			DWORD d;
			
			switch(shift){
				case 0: d=accum>>0; accum &= 0x00; break;
				case 2: d=accum>>2; accum &= 0x03; break;
				case 4: d=accum>>4; accum &= 0x0F; break;
				case 6: d=accum>>6; accum &= 0x3F; break;
			}

			*pDataOut++ = m_key[d];
		}while(shift>=6);
	}

	// last char
	if (shift>0) {
		accum <<= (6-shift);
		*pDataOut++ = m_key[accum];
	}

	pStreamOut->SetLen(pDataOut-pDataOutBeg);
}


// not reset pStreamOut
//
void RLBase64Coder::Decode(RLStream* pStreamIn, RLStream* pStreamOut)
{
	DWORD dwReadPos	  = pStreamIn->GetReadPos();
	DWORD dwLenIn     = pStreamIn->GetLen() - dwReadPos;
	BYTE* pDataIn     = (BYTE*)pStreamIn->GetBuffer() + dwReadPos;
	BYTE* pDataInEnd  = pDataIn + dwLenIn;
	
	pStreamOut->SetMinExtraCapasity(dwLenIn/4*3+2);
	BYTE* pDataOutBeg = (BYTE*)pStreamOut->GetBuffer();
	BYTE* pDataOut    = pDataOutBeg + pStreamOut->GetLen();
	//
	// lookup table for converting base64 characters to value in range 0..63
	//
	signed char* codes;	

	// Initialize Decoding table.
	if (m_decode_codes.GetLen()==0) {
		m_decode_codes.SetMinCapasity(256);
		m_decode_codes.SetLen(256);

		codes = (signed char*)m_decode_codes.GetBuffer();

		for (int i1=0; i1<256; i1++) codes[i1] = -1;		
		for (int i2=0; i2<64;  i2++) codes[m_key[i2]] = (char)i2;
	}
	else {
		codes = (signed char*)m_decode_codes.GetBuffer();
	}
		
	
	int shift = 0;	// # of excess bits stored in accum
	int accum = 0;	// excess bits
	
	// decoding ...
	while(pDataIn<pDataInEnd) 
	{	
		BYTE b = *pDataIn++;

		int value = codes[b];
		if ( value >= 0 ) {			// skip over non-code		
			accum <<= 6;			// bits shift up by 6 each time thru
			shift += 6; 			// loop, with new bits being put in
			accum |= value; 		// at the bottom.
			if ( shift >= 8 ) {		// whenever there are 8 or more shifted in,			
				shift -= 8;
				*pDataOut++ = (BYTE)(( accum >> shift ) & 0xff);							
			}
		}
	}
	
	pStreamOut->SetLen(pDataOut-pDataOutBeg);
}


CStringA RLBase64Coder::Encode(CStringA strIn)
{
	RLStream streamIn;
	RLStream streamOut;

	streamIn.AddString0A(strIn);

	Encode(&streamIn, &streamOut);

	CStringA strOut = streamOut.GetString0A();
	return strOut;
}


CStringA RLBase64Coder::Decode(CStringA strIn)
{
	RLStream streamIn;
	RLStream streamOut;

	streamIn.AddString0A(strIn);

	Decode(&streamIn, &streamOut);

	CStringA strOut = streamOut.GetString0A();
	return strOut;
}
