#include "stdafx.h"
#include "VrEncoder.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

template <typename PIXEL>
class VrEncoderHextile : public VrEncoders
{
public:

void __forceinline ReadRectHextile(int rx, int ry, int rw, int rh)
{
	PIXEL bgcolor = 0;
	PIXEL fgcolor = 0;

    BYTE* buf = (BYTE*)m_netBuffer.GetBuffer1(256 * (sizeof(PIXEL) + 2));

    for (int y = ry; y < ry+rh; y += 16) {		

			int h = min(ry+rh-y, 16);

	for (int x = rx; x < rx+rw; x += 16) {
            int w = min(rx+rw-x, 16);

			UINT8 subencoding;
            ReadExact(&subencoding, 1);

            if (subencoding & aaHextileRaw) {
                ReadExact(buf, w * h * sizeof(PIXEL));
                SETPIXELS(buf, x,y,w,h);
                continue;
            }

		    if (subencoding & aaHextileBackgroundSpecified) {
                ReadExact(&bgcolor, sizeof(PIXEL));
			}
            
			FillSolidRect(x,y,w,h,&bgcolor);

            if (subencoding & aaHextileForegroundSpecified)  {
                ReadExact(&fgcolor, sizeof(PIXEL));
			}

            if (!(subencoding & aaHextileAnySubrects)) continue;

			UINT8 nSubrects;
            ReadExact(&nSubrects, 1);

            UINT8* ptr = (UINT8 *)buf;

            if (subencoding & aaHextileSubrectsColoured) 
			{
                ReadExact(ptr, nSubrects * (2 + sizeof(PIXEL)));

                for (int i = 0; i < nSubrects; i++) {
                    PIXEL* fgcolor1 = (PIXEL*)ptr;
					ptr += sizeof(PIXEL);
                    int sx = *ptr >> 4;
                    int sy = *ptr++ & 0x0f;
                    int sw = (*ptr >> 4) + 1;
                    int sh = (*ptr++ & 0x0f) + 1;
                    FillSolidRect(x+sx, y+sy, sw, sh, fgcolor1);
                }
            } else {
                ReadExact(ptr, nSubrects * 2);

                for (int i = 0; i < nSubrects; i++) {
                    int sx = *ptr >> 4;
                    int sy = *ptr++ & 0x0f;
                    int sw = (*ptr >> 4) + 1;
                    int sh = (*ptr++ & 0x0f) + 1;
                    FillSolidRect(x+sx, y+sy, sw, sh, &fgcolor);
                }
            }
    }
	this->InvalidateScreenRect(rx, y, rw, h);
    }
}

};


void VrEncoders::ReadHextileRect(aaFramebufferUpdateRectHeader *pfburh)
{
	int x = pfburh->r.x;
	int y = pfburh->r.y;
	int w = pfburh->r.w;
	int h = pfburh->r.h;

	switch (m_frmR.bitsPerPixel/8) {
		case 1: ((VrEncoderHextile<UINT8> *)this)->ReadRectHextile(x, y, w, h); break;
		case 2: ((VrEncoderHextile<UINT16>*)this)->ReadRectHextile(x, y, w, h); break;
		case 3: ((VrEncoderHextile<UINT24>*)this)->ReadRectHextile(x, y, w, h); break;
		case 4: ((VrEncoderHextile<UINT32>*)this)->ReadRectHextile(x, y, w, h); break;
	}
}
