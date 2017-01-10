#ifndef __CSTRING_A_H__INCLUDED__
#define __CSTRING_A_H__INCLUDED__

#ifndef __cplusplus
	#error StringA requires C++ compilation (use a .cpp suffix)
#endif


struct CStringDataA
{
	long nRefs;             // reference count
	int nDataLength;        // length of data (including terminator)
	int nAllocLength;       // length of allocation
	// char data[nAllocLength]

	char* data() { return (char*)(this+1); }	// char* to managed data
};

class CStringA
{
public:
// Constructors

	// constructs empty CStringA
	CStringA();
	// copy constructor
	CStringA(const CStringA& stringSrc);
	// from a single character
	CStringA(char ch, int nRepeat = 1);
	// from an ANSI string (converts to char)
	CStringA(LPCSTR lpsz);
	// from a UNICODE string (converts to char)
	CStringA(LPCWSTR lpsz);
	// subset of characters from an ANSI string (converts to char)
	CStringA(LPCSTR lpch, int nLength);
	// subset of characters from a UNICODE string (converts to char)
	CStringA(LPCWSTR lpch, int nLength);
	// from unsigned characters
	CStringA(const unsigned char* psz);

// Attributes & Operations

	// get data length
	int GetLength() const;
	// TRUE if zero length
	BOOL IsEmpty() const;
	// clear contents to empty
	void Empty();

	// return single character at zero-based index
	char GetAt(int nIndex) const;
	// return single character at zero-based index
	char operator[](int nIndex) const;
	// set a single character at zero-based index
	void SetAt(int nIndex, char ch);
	// return pointer to const string
	operator LPCSTR() const;

	// overloaded assignment

	// ref-counted copy from another CStringA
	const CStringA& operator=(const CStringA& stringSrc);
	// set string content to single character
	const CStringA& operator=(char ch);
	// copy string content from ANSI string (converts to char)
	const CStringA& operator=(LPCSTR lpsz);
	// copy string content from UNICODE string (converts to char)
	const CStringA& operator=(LPCWSTR lpsz);
	// copy string content from unsigned chars
	const CStringA& operator=(const unsigned char* psz);

	// string concatenation

	// concatenate from another CStringA
	const CStringA& operator+=(const CStringA& string);

	// concatenate a single character
	const CStringA& operator+=(char ch);
	// concatenate a UNICODE character after converting it to char
	const CStringA& operator+=(LPCSTR lpsz);

	friend CStringA operator+(const CStringA& string1, const CStringA& string2);
	friend CStringA operator+(const CStringA& string, char ch);
	friend CStringA operator+(char ch, const CStringA& string);
	friend CStringA operator+(const CStringA& string, LPCSTR lpsz);
	friend CStringA operator+(LPCSTR lpsz, const CStringA& string);

	// string comparison

	// straight character comparison
	int Compare(LPCSTR lpsz) const;
	// compare ignoring case
	int CompareNoCase(LPCSTR lpsz) const;
	// NLS aware comparison, case sensitive
	int Collate(LPCSTR lpsz) const;
	// NLS aware comparison, case insensitive
	int CollateNoCase(LPCSTR lpsz) const;

	// simple sub-string extraction

	// return nCount characters starting at zero-based nFirst
	CStringA Mid(int nFirst, int nCount) const;
	// return all characters starting at zero-based nFirst
	CStringA Mid(int nFirst) const;
	// return first nCount characters in string
	CStringA Left(int nCount) const;
	// return nCount characters from end of string
	CStringA Right(int nCount) const;

	//  characters from beginning that are also in passed string
	CStringA SpanIncluding(LPCSTR lpszCharSet) const;
	// characters from beginning that are not also in passed string
	CStringA SpanExcluding(LPCSTR lpszCharSet) const;

	// upper/lower/reverse conversion

	// NLS aware conversion to uppercase
	void MakeUpper();
	// NLS aware conversion to lowercase
	void MakeLower();
	// reverse string right-to-left
	void MakeReverse();

	// trimming whitespace (either side)

	// remove whitespace starting from right edge
	void TrimRight();
	// remove whitespace starting from left side
	void TrimLeft();

	// trimming anything (either side)

/*
	// remove continuous occurrences of chTarget starting from right
	void TrimRight(char chTarget);
	// remove continuous occcurrences of characters in passed string,
	// starting from right
	void TrimRight(LPCSTR lpszTargets);
	// remove continuous occurrences of chTarget starting from left
	void TrimLeft(char chTarget);
	// remove continuous occcurrences of characters in
	// passed string, starting from left
	void TrimLeft(LPCSTR lpszTargets);
*/

	// advanced manipulation

	// replace occurrences of chOld with chNew
	int Replace(char chOld, char chNew);
	// replace occurrences of substring lpszOld with lpszNew;
	// empty lpszNew removes instances of lpszOld
	int Replace(LPCSTR lpszOld, LPCSTR lpszNew);
	// remove occurrences of chRemove
	int Remove(char chRemove);
	// insert character at zero-based index; concatenates
	// if index is past end of string
	int Insert(int nIndex, char ch);
	// insert substring at zero-based index; concatenates
	// if index is past end of string
	int Insert(int nIndex, LPCSTR pstr);
	// delete nCount characters starting at zero-based index
	int Delete(int nIndex, int nCount = 1);

	// searching

	// find character starting at left, -1 if not found
	int Find(char ch) const;
	// find character starting at right
	int ReverseFind(char ch) const;
	// find character starting at zero-based index and going right
	int Find(char ch, int nStart) const;
	// find first instance of any character in passed string
	int FindOneOf(LPCSTR lpszCharSet) const;
	// find first instance of substring
	int Find(LPCSTR lpszSub) const;
	// find first instance of substring starting at zero-based index
	int Find(LPCSTR lpszSub, int nStart) const;

	// simple formatting

	// printf-like formatting using passed string
	void Format(LPCSTR lpszFormat, ...);
	// printf-like formatting using referenced string resource
	//void Format(UINT nFormatID, ...);
	// printf-like formatting using variable arguments parameter
	void FormatV(LPCSTR lpszFormat, va_list argList);


	// formatting for localization (uses FormatMessage API)

	// format using FormatMessage API on passed string
	void FormatMessage(LPCSTR lpszFormat, ...);
	// format using FormatMessage API on referenced string resource
	void FormatMessage(UINT nFormatID, ...);

	// load from string resource
	BOOL LoadString(UINT nID);

	// ANSI <-> OEM support (convert string in place)

	// convert string from ANSI to OEM in-place
	void AnsiToOem();
	// convert string from OEM to ANSI in-place
	void OemToAnsi();



	// Access to string implementation buffer as "C" character array

	// get pointer to modifiable buffer at least as long as nMinBufLength
	LPSTR GetBuffer(int nMinBufLength);
	// release buffer, setting length to nNewLength (or to first nul if -1)
	void ReleaseBuffer(int nNewLength = -1);
	// get pointer to modifiable buffer exactly as long as nNewLength
	LPSTR GetBufferSetLength(int nNewLength);
	// release memory allocated to but unused by string
	void FreeExtra();

	// Use LockBuffer/UnlockBuffer to turn refcounting off

	// turn refcounting back on
	LPSTR LockBuffer();
	// turn refcounting off
	void UnlockBuffer();

#ifdef __wtypes_h__
//	BSTR AllocSysString() const;
#endif

// Implementation
public:
	~CStringA();
	int GetAllocLength() const;

protected:
	LPSTR m_pchData;   // pointer to ref counted string data

	// implementation helpers
	CStringDataA* GetData() const;
	void Init();
	void AllocCopy(CStringA& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
	void AllocBuffer(int nLen);
	void AssignCopy(int nSrcLen, LPCSTR lpszSrcData);
	void ConcatCopy(int nSrc1Len, LPCSTR lpszSrc1Data, int nSrc2Len, LPCSTR lpszSrc2Data);
	void ConcatInPlace(int nSrcLen, LPCSTR lpszSrcData);
	void CopyBeforeWrite();
	void AllocBeforeWrite(int nLen);
	void Release();
	static void Release(CStringDataA* pData);
	static int  SafeStrlen(LPCSTR lpsz);
	static void FreeData(CStringDataA* pData);
};

// Compare helpers
bool operator==(const CStringA& s1, const CStringA& s2);
bool operator==(const CStringA& s1, LPCSTR s2);
bool operator==(LPCSTR s1, const CStringA& s2);
bool operator!=(const CStringA& s1, const CStringA& s2);
bool operator!=(const CStringA& s1, LPCSTR s2);
bool operator!=(LPCSTR s1, const CStringA& s2);
bool operator<(const CStringA& s1, const CStringA& s2);
bool operator<(const CStringA& s1, LPCSTR s2);
bool operator<(LPCSTR s1, const CStringA& s2);
bool operator>(const CStringA& s1, const CStringA& s2);
bool operator>(const CStringA& s1, LPCSTR s2);
bool operator>(LPCSTR s1, const CStringA& s2);
bool operator<=(const CStringA& s1, const CStringA& s2);
bool operator<=(const CStringA& s1, LPCSTR s2);
bool operator<=(LPCSTR s1, const CStringA& s2);
bool operator>=(const CStringA& s1, const CStringA& s2);
bool operator>=(const CStringA& s1, LPCSTR s2);
bool operator>=(LPCSTR s1, const CStringA& s2);

// conversion helpers
int _wcstombsz(char* mbstr, const wchar_t* wcstr, size_t count);
int _mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count);

#endif // __CSTRING_A_H__INCLUDED__


