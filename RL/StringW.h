#ifndef __CSTRING_W_H__INCLUDED__
#define __CSTRING_W_H__INCLUDED__

#ifndef __cplusplus
	#error StringMFC requires C++ compilation (use a .cpp suffix)
#endif


struct CStringDataW
{
	long nRefs;             // reference count
	int nDataLength;        // length of data (including terminator)
	int nAllocLength;       // length of allocation
	// WCHAR data[nAllocLength]

	WCHAR* data() { return (WCHAR*)(this+1); }	// WCHAR* to managed data
};

class CStringW
{
public:
// Constructors

	CStringW();								// constructs empty CStringW
	CStringW(const CStringW& stringSrc);	// copy constructor
	CStringW(WCHAR ch, int nRepeat = 1);	// from a single character
	CStringW(LPCSTR lpsz);					// from an ANSI string (converts to WCHAR)
	CStringW(LPCWSTR lpsz);					// from a UNICODE string (converts to WCHAR)
	CStringW(LPCSTR lpch, int nLength);		// subset of characters from an ANSI string (converts to WCHAR)
	CStringW(LPCWSTR lpch, int nLength);	// subset of characters from a UNICODE string (converts to WCHAR)
	CStringW(const unsigned char* psz);		// from unsigned characters

// Attributes & Operations

	int GetLength() const;	// get data length
	BOOL IsEmpty() const;	// TRUE if zero length
	void Empty();			// clear contents to empty

	// return single character at zero-based index
	WCHAR GetAt(int nIndex) const;
	// return single character at zero-based index
	WCHAR operator[](int nIndex) const;
	// set a single character at zero-based index
	void SetAt(int nIndex, WCHAR ch);
	// return pointer to const string
	operator LPCWSTR() const;

	// overloaded assignment

	// ref-counted copy from another CStringW
	const CStringW& operator=(const CStringW& stringSrc);
	// set string content to single character
	const CStringW& operator=(WCHAR ch);

	const CStringW& operator=(char ch);

	const CStringW& operator=(LPCSTR lpsz);		// copy string content from ANSI string (converts to WCHAR)
	const CStringW& operator=(LPCWSTR lpsz);	// copy string content from UNICODE string (converts to WCHAR)
	const CStringW& operator=(const unsigned char* psz);	// copy string content from unsigned chars

	// string concatenation

	const CStringW& operator+=(const CStringW& string);	// concatenate from another CStringW
	const CStringW& operator+=(WCHAR ch);				// concatenate a single character
	const CStringW& operator+=(char ch);				// concatenate an ANSI character after converting it to WCHAR
	const CStringW& operator+=(LPCWSTR lpsz);			// concatenate a UNICODE character after converting it to WCHAR

	friend CStringW operator+(const CStringW& string1, const CStringW& string2);
	friend CStringW operator+(const CStringW& string, WCHAR ch);
	friend CStringW operator+(WCHAR ch, const CStringW& string);
	friend CStringW operator+(const CStringW& string, char ch);
	friend CStringW operator+(char ch, const CStringW& string);
	friend CStringW operator+(const CStringW& string, LPCWSTR lpsz);
	friend CStringW operator+(LPCWSTR lpsz, const CStringW& string);

	// string comparison

	int Compare(LPCWSTR lpsz) const;		// straight character comparison
	int CompareNoCase(LPCWSTR lpsz) const;	// compare ignoring case
	int Collate(LPCWSTR lpsz) const;		// NLS aware comparison, case sensitive
	int CollateNoCase(LPCWSTR lpsz) const;	// NLS aware comparison, case insensitive

	// simple sub-string extraction

	CStringW Mid(int nFirst, int nCount) const;		// return nCount characters starting at zero-based nFirst
	CStringW Mid(int nFirst) const;					// return all characters starting at zero-based nFirst
	CStringW Left(int nCount) const;				// return first nCount characters in string
//	CStringW Right(int nCount) const;				// return nCount characters from end of string

	CStringW SpanIncluding(LPCWSTR lpszCharSet) const;	// characters from beginning that are also in passed string
	CStringW SpanExcluding(LPCWSTR lpszCharSet) const;	// characters from beginning that are not also in passed string

	// upper/lower/reverse conversion

	void MakeUpper();	// NLS aware conversion to uppercase
	void MakeLower();	// NLS aware conversion to lowercase
	void MakeReverse();	// reverse string right-to-left

	// trimming whitespace (either side)

	void TrimRight();	// remove whitespace starting from right edge
	void TrimLeft();	// remove whitespace starting from left side

	// trimming anything (either side)

/*
	void TrimRight(WCHAR chTarget);		// remove continuous occurrences of chTarget starting from right
	void TrimRight(LPCWSTR lpszTargets);// remove continuous occurrences of characters in passed string, starting from right
	void TrimLeft(WCHAR chTarget);		// remove continuous occurrences of chTarget starting from left
	void TrimLeft(LPCWSTR lpszTargets);	// remove continuous occurrences of characters in passed string, starting from left
*/

	// advanced manipulation

	int Replace(WCHAR chOld, WCHAR chNew);			// replace occurrences of chOld with chNew
	int Replace(LPCWSTR lpszOld, LPCWSTR lpszNew);	// replace occurrences of substring lpszOld with lpszNew; empty lpszNew removes instances of lpszOld
	int Remove(WCHAR chRemove);						// remove occurrences of chRemove
	// insert character at zero-based index; concatenates
	// if index is past end of string
	int Insert(int nIndex, WCHAR ch);
	// insert substring at zero-based index; concatenates
	// if index is past end of string
	int Insert(int nIndex, LPCWSTR pstr);
	// delete nCount characters starting at zero-based index
	int Delete(int nIndex, int nCount = 1);

	// searching

	int Find(WCHAR ch) const;					// find character starting at left, -1 if not found
	int ReverseFind(WCHAR ch) const;			// find character starting at right
	int Find(WCHAR ch, int nStart) const;		// find character starting at zero-based index and going right
	int FindOneOf(LPCWSTR lpszCharSet) const;	// find first instance of any character in passed string
	int Find(LPCWSTR lpszSub) const;			// find first instance of substring
	int Find(LPCWSTR lpszSub, int nStart) const;// find first instance of substring starting at zero-based index

	// simple formatting

	// printf-like formatting using passed string
	void Format(LPCWSTR lpszFormat, ...);
	// printf-like formatting using referenced string resource
	//void Format(UINT nFormatID, ...);
	// printf-like formatting using variable arguments parameter
	void FormatV(LPCWSTR lpszFormat, va_list argList);


	// formatting for localization (uses FormatMessage API)

	// format using FormatMessage API on passed string
	void FormatMessage(LPCWSTR lpszFormat, ...);
	// format using FormatMessage API on referenced string resource
	void FormatMessage(UINT nFormatID, ...);

	// load from string resource
	BOOL LoadString(UINT nID);


	// Access to string implementation buffer as "C" character array

	// get pointer to modifiable buffer at least as long as nMinBufLength
	LPWSTR GetBuffer(int nMinBufLength);
	// release buffer, setting length to nNewLength (or to first nul if -1)
	void ReleaseBuffer(int nNewLength = -1);
	// get pointer to modifiable buffer exactly as long as nNewLength
	LPWSTR GetBufferSetLength(int nNewLength);
	// release memory allocated to but unused by string
	void FreeExtra();

	// Use LockBuffer/UnlockBuffer to turn refcounting off

	// turn refcounting back on
	LPWSTR LockBuffer();
	// turn refcounting off
	void UnlockBuffer();

// Implementation
public:
	~CStringW();
	int GetAllocLength() const;

protected:
	LPWSTR m_pchData;   // pointer to ref counted string data

	// implementation helpers
	CStringDataW* GetData() const;
	void Init();
	void AllocCopy(CStringW& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
	void AllocBuffer(int nLen);
	void AssignCopy(int nSrcLen, LPCWSTR lpszSrcData);
	void ConcatCopy(int nSrc1Len, LPCWSTR lpszSrc1Data, int nSrc2Len, LPCWSTR lpszSrc2Data);
	void ConcatInPlace(int nSrcLen, LPCWSTR lpszSrcData);
	void CopyBeforeWrite();
	void AllocBeforeWrite(int nLen);
	void Release();
	static void PASCAL Release(CStringDataW* pData);
	static int PASCAL SafeStrlen(LPCWSTR lpsz);
	static void FreeData(CStringDataW* pData);
};

// Compare helpers
bool operator==(const CStringW& s1, const CStringW& s2);
bool operator==(const CStringW& s1, LPCWSTR s2);
bool operator==(LPCWSTR s1, const CStringW& s2);
bool operator!=(const CStringW& s1, const CStringW& s2);
bool operator!=(const CStringW& s1, LPCWSTR s2);
bool operator!=(LPCWSTR s1, const CStringW& s2);
bool operator<(const CStringW& s1, const CStringW& s2);
bool operator<(const CStringW& s1, LPCWSTR s2);
bool operator<(LPCWSTR s1, const CStringW& s2);
bool operator>(const CStringW& s1, const CStringW& s2);
bool operator>(const CStringW& s1, LPCWSTR s2);
bool operator>(LPCWSTR s1, const CStringW& s2);
bool operator<=(const CStringW& s1, const CStringW& s2);
bool operator<=(const CStringW& s1, LPCWSTR s2);
bool operator<=(LPCWSTR s1, const CStringW& s2);
bool operator>=(const CStringW& s1, const CStringW& s2);
bool operator>=(const CStringW& s1, LPCWSTR s2);
bool operator>=(LPCWSTR s1, const CStringW& s2);

// conversion helpers
//int _wcstombsz(char* mbstr, const wchar_t* wcstr, size_t count);
//int _mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count);

#endif // __CSTRING_W_H__INCLUDED__


