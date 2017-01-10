#if !defined(_FONT_WTL_INCLUDED_)
#define _FONT_WTL_INCLUDED_


template <bool t_bManaged>
class CFontT
{
public:
// Data members
	HFONT m_hFont;

// Constructor/destructor/operators
	CFontT(HFONT hFont = NULL) : m_hFont(hFont)
	{ }

	~CFontT()
	{
		if(t_bManaged && m_hFont != NULL)
			DeleteObject();
	}

	CFontT<t_bManaged>& operator =(HFONT hFont)
	{
		Attach(hFont);
		return *this;
	}

	void Attach(HFONT hFont)
	{
		if(t_bManaged && m_hFont != NULL && m_hFont != hFont)
			::DeleteObject(m_hFont);
		m_hFont = hFont;
	}

	HFONT Detach()
	{
		HFONT hFont = m_hFont;
		m_hFont = NULL;
		return hFont;
	}

	operator HFONT() const { return m_hFont; }

	bool IsNull() const { return (m_hFont == NULL); }

// Create methods
	HFONT CreateFontIndirect(const LOGFONT* lpLogFont)
	{
		m_hFont = ::CreateFontIndirect(lpLogFont);
		return m_hFont;
	}

#if !defined(_WIN32_WCE) && (_WIN32_WINNT >= 0x0500)
	HFONT CreateFontIndirectEx(CONST ENUMLOGFONTEXDV* penumlfex)
	{
		ATLASSERT(m_hFont == NULL);
		m_hFont = ::CreateFontIndirectEx(penumlfex);
		return m_hFont;
	}
#endif // !defined(_WIN32_WCE) && (_WIN32_WINNT >= 0x0500)

#if !defined(_WIN32_WCE) || (_ATL_VER >= 0x0800)
	HFONT CreateFont(int nHeight, int nWidth, int nEscapement,
			int nOrientation, int nWeight, BYTE bItalic, BYTE bUnderline,
			BYTE cStrikeOut, BYTE nCharSet, BYTE nOutPrecision,
			BYTE nClipPrecision, BYTE nQuality, BYTE nPitchAndFamily,
			LPCTSTR lpszFacename)
	{
		ATLASSERT(m_hFont == NULL);
#ifndef _WIN32_WCE
		m_hFont = ::CreateFont(nHeight, nWidth, nEscapement,
			nOrientation, nWeight, bItalic, bUnderline, cStrikeOut,
			nCharSet, nOutPrecision, nClipPrecision, nQuality,
			nPitchAndFamily, lpszFacename);
#else // CE specific
		m_hFont = ATL::CreateFont(nHeight, nWidth, nEscapement,
			nOrientation, nWeight, bItalic, bUnderline, cStrikeOut,
			nCharSet, nOutPrecision, nClipPrecision, nQuality,
			nPitchAndFamily, lpszFacename);
#endif // _WIN32_WCE
		return m_hFont;
	}
#endif // !defined(_WIN32_WCE) || (_ATL_VER >= 0x0800)

	HFONT CreatePointFont(int nPointSize, LPCTSTR lpszFaceName, HDC hDC = NULL, bool bBold = false, bool bItalic = false)
	{
		LOGFONT logFont = { 0 };
		logFont.lfCharSet = DEFAULT_CHARSET;
		logFont.lfHeight = nPointSize;
//		SecureHelper::strncpy_x(logFont.lfFaceName, _countof(logFont.lfFaceName), lpszFaceName, _TRUNCATE);

		if(bBold)
			logFont.lfWeight = FW_BOLD;
		if(bItalic)
			logFont.lfItalic = (BYTE)TRUE;

		return CreatePointFontIndirect(&logFont, hDC);
	}

	HFONT CreatePointFontIndirect(const LOGFONT* lpLogFont, HDC hDC = NULL)
	{
		HDC hDC1 = (hDC != NULL) ? hDC : ::GetDC(NULL);

		// convert nPointSize to logical units based on hDC
		LOGFONT logFont = *lpLogFont;
#ifndef _WIN32_WCE
		POINT pt = { 0, 0 };
		pt.y = ::MulDiv(::GetDeviceCaps(hDC1, LOGPIXELSY), logFont.lfHeight, 720);   // 72 points/inch, 10 decipoints/point
		::DPtoLP(hDC1, &pt, 1);
		POINT ptOrg = { 0, 0 };
		::DPtoLP(hDC1, &ptOrg, 1);
		logFont.lfHeight = -abs(pt.y - ptOrg.y);
#else // CE specific
		// DP and LP are always the same on CE
		logFont.lfHeight = -abs(::MulDiv(::GetDeviceCaps(hDC1, LOGPIXELSY), logFont.lfHeight, 720));   // 72 points/inch, 10 decipoints/point
#endif // _WIN32_WCE

		if(hDC == NULL)
			::ReleaseDC(NULL, hDC1);

		return CreateFontIndirect(&logFont);
	}

	BOOL DeleteObject()
	{
		BOOL bRet = ::DeleteObject(m_hFont);
		if(bRet)
			m_hFont = NULL;
		return bRet;
	}

// Attributes
	int GetLogFont(LOGFONT* pLogFont) const
	{
		return ::GetObject(m_hFont, sizeof(LOGFONT), pLogFont);
	}

	bool GetLogFont(LOGFONT& LogFont) const
	{
		//ATLASSERT(m_hFont != NULL);
		return (::GetObject(m_hFont, sizeof(LOGFONT), &LogFont) == sizeof(LOGFONT));
	}
};

//typedef CFontT<false>   CFontHandle1;
typedef CFontT<true>    CFontWTL;

#endif //_FONT_WTL_INCLUDED_