#if !defined(_IMAGE_H__51628E7E_DB71_457D_B44D_EEBFE82312FA__INCLUDED_)
#define _IMAGE_H__51628E7E_DB71_457D_B44D_EEBFE82312FA__INCLUDED_

#include "images/resource_images.h"

//color manipulation
#pragma pack(push,1)
struct Color
{
	UINT8 b,g,r,a;
	Color(){}
	Color(UINT8 _r, UINT8 _g, UINT8 _b) :
		r(_r), g(_g), b(_b), a(255)
	{
	}

	Color(UINT8 _r, UINT8 _g, UINT8 _b, UINT8 _a) :
		r(_r), g(_g), b(_b), a(_a)
	{
	}
	Color(UINT32 c)
	{
		a = (UINT8)((c>>24)&0xFF);
		r = (UINT8)((c>>16)&0xFF);
		g = (UINT8)((c>> 8)&0xFF);
		b = (UINT8)((c>> 0)&0xFF);
	}

	int getInt() const
	{
		return (a<<24) | (r<<16) | (g<<8) | (b<<0);
	}

	friend Color operator *(Color c1, Color c2)
	{
		UINT32 r = c1.r*c2.r/255;
		UINT32 g = c1.g*c2.g/255;
		UINT32 b = c1.b*c2.b/255;
		UINT32 a = c1.a*c2.a/255;
		return Color((UINT8)r,(UINT8)g,(UINT8)b,(UINT8)a);
	}

	friend Color operator +(Color c1, Color c2)
	{
		return Color((UINT8)(c1.r+c2.r), (UINT8)(c1.g+c2.g), (UINT8)(c1.b+c2.b), (UINT8)(c1.a+c2.a));
	}

	Color operator +=(Color c)
	{
		r += c.r; g += c.g; b += c.b; a += c.a;
		return *this;
	}

	friend Color operator *(Color c1, UINT8 c)
	{
		return Color(c1.r*c/255, c1.g*c/255, c1.b*c/255, c1.a*c/255);
	}
	Color operator *=(UINT8 c)
	{
		r = r * c/255; g = g * c/255; b = b * c/255; a = a * c/255;
		return *this;
	}

	friend Color brightness(Color c1, int c)
	{
		int r = c1.r+c; if(r>255) r = 255;
		int g = c1.g+c; if(g>255) g = 255;
		int b = c1.b+c; if(b>255) b = 255;
		int a = c1.a+c; if(a>255) a = 255;
		return Color((UINT8)r, (UINT8)g, (UINT8)b, (UINT8)a);
	}
	friend Color contrast(Color c1, float c)
	{
		int r = int(c1.r*c); if(r>255) r = 255;
		int g = int(c1.g*c); if(g>255) g = 255;
		int b = int(c1.b*c); if(b>255) b = 255;
		int a = int(c1.a*c); if(a>255) a = 255;
		return Color((UINT8)r, (UINT8)g, (UINT8)b, (UINT8)a);
	}

	Color operator -=(Color c)
	{
		int _r = int(r) - c.r; if(_r<0) _r=0; r = _r;
		int _g = int(g) - c.g; if(_g<0) _g=0; g = _g;
		int _b = int(b) - c.b; if(_b<0) _b=0; b = _b;
		int _a = int(a) - c.a; if(_a<0) _a=0; a = _a;
		return *this;
	}
	Color getInverted() const
	{
		return Color(255-r, 255-g, 255-b, 255-a);
	}
};
#pragma pack(pop)


class Image
{
public:
	Color* m_ptr;
	int m_w,m_h;

	Image()
	{
		m_ptr = NULL;
		m_w = m_h = 0;
	}

	Image(int w, int h)
	{
		m_ptr = NULL;
		SetSize(w, h);
	}

	~Image()
	{
		if (m_ptr!=NULL) ::free(m_ptr);
	}

	void SetSize(int cx, int cy)
	{
		m_w = cx;
		m_h = cy;
		if (m_ptr!=NULL) ::free(m_ptr);
		m_ptr = (Color*)::malloc(cx*cy*sizeof(Color));
		if (m_ptr==NULL)
			throw RLException("SetSize(%u, %u)", cx, cy);
	}

	void Clear(Color c)
	{
		for(int y=0; y<m_h; y++)
		for(int x=0; x<m_w; x++)
			Set(x, y, c);
	}

	void CopyTo(int x, int y, int cx, int cy, Image& out)
	{
		out.SetSize(cx, cy);

		Color* pSrc = &Get(x, y);
		Color* pDst = &out.Get(0, 0);

		for (;cy>0; cy--) {
			memcpy(pDst, pSrc, cx*sizeof(Color));
			pSrc -= m_w;
			pDst -= cx;
		}
	}

	void FromResource(const GIMPBMP& res)
	{
		Color c = Color(255, 255, 255);
		this->SetSize(res.cx, res.cy);
		this->DrawPicture1(res, 0, 0, c.getInt(), Image::Mode::SET);
	}

	void FromResourceByCenter(const GIMPBMP& res, int cx, int cy)
	{
		Color c = Color(255, 255, 255);
		int x_off = (cx - res.cx)/2;
		int y_off = (cy - res.cy)/2;
		this->SetSize(cx, cy);
		this->Clear(c);
		this->DrawPicture1(res, x_off, y_off, c.getInt(), Image::Mode::SET);
	}

	void MakeBW()
	{
		Color* ptr = m_ptr;
		Color* ptr_end = ptr + m_w * m_h;
		while (ptr<ptr_end)
		{
			UINT8 gray = (UINT8)((ptr->r*30 + ptr->g*59 + ptr->b*11) / 100);
			ptr->r = ptr->g = ptr->b = gray;
			ptr++;
		}
	}

	void FlipVertical()
	{
		int cy = m_h/2;
		int y2 = m_h-1; // last row
		for (int y1=0; y1<cy; y1++, y2--) 
		{
			 for (int x=0; x<m_w; x++) {
				 Color c1 = Get(x, y1);
				 Color c2 = Get(x, y2);
				 Set(x, y1, c2);
				 Set(x, y2, c1);
			 }
		}
	}


	void DrawGradientV(UINT8 clrTop, UINT8 clrBottom, int y, int h)
	{
		for(int y1=0; y1<h; y1++)
		{
			int c = clrTop - y1*(clrTop-clrBottom) / h;
			Color c1(c,c,c,255);
			for(int x=0; x<m_w; x++)
				Set(x, y+y1, c1);
		}
	}

	void DrawGradientH(UINT8 colorL, UINT8 colorR, int xL, int xR, int yT, int cy)
	{
		int cx = xR - xL;

		for(int x=0; x<cx; x++)
		{
			int c = colorL - x*(colorL-colorR) / cx;
			Color c1(c,c,c,255);			
			Set(x+xL, yT, c1);
		}

		StretchVertical(xL, yT, xR-xL, cy);
	}

	void DrawRect(int x, int y, int w, int h, UINT32 color)
	{
		Color c(color);

		int x2 = x+w-1;
		int y2 = y+h-1;

		for(int y1=y; y1<=y2; y1++) {
		for(int x1=x; x1<=x2; x1++) {
			Blend(x1, y1,  c);
		}
		}
	}

	void DrawBox(int x, int y, int w, int h, UINT32 color)
	{
		Color c(color);

		int x2 = x+w-1;
		int y2 = y+h-1;

		for(int x1=x; x1<=x2; x1++)
		{
			Blend(x1, y,  c);
			Blend(x1, y2, c);
		}
		for(int y1=y+1; y1<=y2-1; y1++)
		{
			Blend(x,  y1, c);
			Blend(x2, y1, c);
		}		
	}

	// stretch 1 vertical line horizontal
	void StretchHorizontal(int x_src, int x_dst, int y1, int y2)
	{
		int x1, x2;

		if (x_dst<x_src) {
			x1 = x_dst;
			x2 = x_src-1;
		}
		else {
			x1 = x_src+1;
			x2 = x_dst;
		}

		for (int y=y1; y<=y2; y++)
		{
			Color c = Get(x_src,y);
			for (int x=x1; x<=x2; x++) Set(x, y, c);
		}
	}

	// stretch 1 horizontal line vertically
	void StretchVertical(int x1, int y1, int cx, int cy)
	{
		int step = (cx>0) ? +1 : -1;

		int x2 = x1+cx;

		for (int x=x1; x!=x2; x+=step) 
		{
			Color c = Get(x,y1);
			for (int y=1; y<cy; y++) Set(x, y1+y, c);
		}
	}

	
	// uses alpha-channel to detect inside area
	//
	void DrawPicture2(const GIMPBMP& img, int px, int py, UINT32 color, UINT32 fillColor)
	{

		for(int x=0; x<img.cx; x++)
		{
			int y1,y2;
			UINT8 prev = 0;
			for(y1=0; y1<img.cy; y1++)
			{
				const UINT8* b = img.GetPixelAddr(x, y1);
				if(*b>prev) prev = *b;
				else
					if(*b<prev) break;
			}
			prev = 0;
			for(y2=img.cy-1; y2>=0; y2--)
			{
				const UINT8* b = img.GetPixelAddr(x, y2);
				if(*b>prev) prev = *b;
				else
					if(*b<prev) break;
			}

			for(int y=0; y<img.cy; y++)
			{
				const UINT8* b = img.GetPixelAddr(x, y);
				Color c = color;

				c.a = *b;
				if(y>=y1 && y<=y2)
				{
					c = Color(fillColor) * (255-*b) + c*(*b);
					c.a = 255;
				}		
				Blend(px+x, py+y, c);
			}
		}
	}

	enum Mode { SET, BLEND, SUBTRACT };
	
	void DrawPicture1(const GIMPBMP& img, int px, int py, UINT32 color, Mode mode)
	{

		for(int x=0; x<img.cx; x++)
		{
		for(int y=0; y<img.cy; y++)
		{
			const UINT8* b = img.GetPixelAddr(x, y);
			Color c = 0;

			switch(img.bytes_per_pixel)
			{
				case 1: c = color;  c.a = b[0]; break;
				case 3: c = Color(b[2],b[1],b[0],255) * Color(color);  c.a = 255;  break;
				case 4: c = Color(b[2],b[1],b[0],255) * Color(color);  c.a = b[3]; break;				
			}

			int x1 = px+x;
			int y1 = py+y;
			switch(mode)
			{
				case SET:	   Set     (x1, y1, c*c.a); break;
				case BLEND:	   Blend   (x1, y1, c);     break;
				case SUBTRACT: Subtract(x1, y1, c);     break;
			}			
		}
		}
	}

	void DrawIndicator01(int px, int py, UINT32 color)
	{
		GIMPBMP& img_a = indicator01_a; // alpha
		GIMPBMP& img_b = indicator01_b; // brightness

		for(int x=0; x<img_a.cx; x++)
		{
		for(int y=0; y<img_a.cy; y++)
		{
			const UINT8 b = *img_b.GetPixelAddr(x, y);

			Color c = color;
			c *= b;
			c.a = *img_a.GetPixelAddr(x, y);
			Blend(px+x, py+y, c);


		}
		}
	}

	inline Color& Get(int x, int y)
	{
		return m_ptr[ (m_h-y-1)*m_w + x ];
	}

	void Set(int x, int y, Color p)
	{
		Color& c = Get(x,y);
		c = p;
	}
	//alpha blending
	void Blend(int x, int y, Color p)
	{
		Color& c = Get(x,y);
		c *= 255-p.a;
		c += p*p.a;
	}
	void Subtract(int x, int y, Color p)
	{
		Color& c = Get(x,y);
		c -= p*p.a;
	}

	HBITMAP GetBitmap()
	{
		HDC hdc = ::GetDC(0);

		BITMAPINFO tBMI = {0};
		tBMI.bmiHeader.biSize = sizeof(tBMI.bmiHeader);
		tBMI.bmiHeader.biWidth  = m_w;
		tBMI.bmiHeader.biHeight = m_h;
		tBMI.bmiHeader.biPlanes = 1;
		tBMI.bmiHeader.biBitCount = 32;
		tBMI.bmiHeader.biCompression = BI_RGB;
		HBITMAP hBitmap = ::CreateDIBitmap(hdc, &tBMI.bmiHeader, CBM_INIT, m_ptr, &tBMI, DIB_RGB_COLORS);
		ASSERT(hBitmap!=NULL);

		::ReleaseDC(0, hdc);

		return hBitmap;
	}
};

#endif // !defined(_IMAGE_H__51628E7E_DB71_457D_B44D_EEBFE82312FA__INCLUDED_)