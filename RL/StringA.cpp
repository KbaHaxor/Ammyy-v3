#include "stdafx.h"
#include "StringA.h"
#ifdef _WIN32
#include <malloc.h>
#endif
#include <stdlib.h>
#include <limits.h>

#ifdef _DEBUG
//#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#pragma warning(disable: 4390)  // empty controlled statement


#ifndef _WIN32

#define _stricmp strcasecmp
#include <wchar.h>
#include <ctype.h>
#define _alloca alloca

#include "win_linux.h"

#endif



//#define TRACE0(sz)
//#define TRACE1(sz, p1)

// Standard constants
#undef FALSE
#undef NULL

#define FALSE   0
#define NULL    0



/////////////////////////////////////////////////////////////////////////////
// static class data, special inlines

// afxChNil is left for backward compatibility
char afxChNil = '\0';


// For an empty string, m_pchData will point here, (note: avoids special case of checking for NULL m_pchData)
int _afxInitData[] = { -1, 0, 0, 0 };
CStringDataA* _afxDataNil = (CStringDataA*)&_afxInitData;
LPCSTR _afxPchNil = (LPCSTR)(((BYTE*)&_afxInitData)+sizeof(CStringDataA));



CStringA::CStringA()
{
	Init();
}

CStringA::CStringA(const CStringA& stringSrc)
{
	ASSERT(stringSrc.GetData()->nRefs != 0);
	if (stringSrc.GetData()->nRefs >= 0)
	{
		ASSERT(stringSrc.GetData() != _afxDataNil);
		m_pchData = stringSrc.m_pchData;
		InterlockedIncrement(&GetData()->nRefs);
	}
	else
	{
		Init();
		*this = stringSrc.m_pchData;
	}
}


// always allocate one extra character for '\0' termination
// assumes [optimistically] that data length will equal allocation length
void CStringA::AllocBuffer(int nLen)
{
	ASSERT(nLen >= 0);
	ASSERT(nLen <= INT_MAX-1);    // max size (enough room for 1 extra)

	if (nLen == 0)
		Init();
	else
	{
		CStringDataA* pData;

		pData = (CStringDataA*) new BYTE[sizeof(CStringDataA) + (nLen+1)*sizeof(char)];
		pData->nAllocLength = nLen;
		pData->nRefs = 1;
		pData->data()[nLen] = '\0';
		pData->nDataLength = nLen;
		m_pchData = pData->data();
	}
}

void CStringA::FreeData(CStringDataA* pData)
{
	delete[] (BYTE*)pData;
}

void CStringA::Release()
{
	if (GetData() != _afxDataNil)
	{
		ASSERT(GetData()->nRefs != 0);
		if (InterlockedDecrement(&GetData()->nRefs) <= 0)
			FreeData(GetData());
		Init();
	}
}

void CStringA::Release(CStringDataA* pData)
{
	if (pData != _afxDataNil)
	{
		ASSERT(pData->nRefs != 0);
		if (InterlockedDecrement(&pData->nRefs) <= 0)
			FreeData(pData);
	}
}

void CStringA::Empty()
{
	if (GetData()->nDataLength == 0)
		return;
	if (GetData()->nRefs >= 0)
		Release();
	else
		*this = &afxChNil;
	ASSERT(GetData()->nDataLength == 0);
	ASSERT(GetData()->nRefs < 0 || GetData()->nAllocLength == 0);
}

void CStringA::CopyBeforeWrite()
{
	if (GetData()->nRefs > 1)
	{
		CStringDataA* pData = GetData();
		Release();
		AllocBuffer(pData->nDataLength);
		memcpy(m_pchData, pData->data(), (pData->nDataLength+1)*sizeof(char));
	}
	ASSERT(GetData()->nRefs <= 1);
}

void CStringA::AllocBeforeWrite(int nLen)
{
	if (GetData()->nRefs > 1 || nLen > GetData()->nAllocLength)
	{
		Release();
		AllocBuffer(nLen);
	}
	ASSERT(GetData()->nRefs <= 1);
}

CStringA::~CStringA()
//  free any attached data
{
	if (GetData() != _afxDataNil)
	{
		if (InterlockedDecrement(&GetData()->nRefs) <= 0)
			FreeData(GetData());
	}
}

//////////////////////////////////////////////////////////////////////////////
// Helpers for the rest of the implementation

void CStringA::AllocCopy(CStringA& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const
{
	// will clone the data attached to this string
	// allocating 'nExtraLen' characters
	// Places results in uninitialized string 'dest'
	// Will copy the part or all of original data to start of new string

	int nNewLen = nCopyLen + nExtraLen;
	if (nNewLen == 0)
	{
		dest.Init();
	}
	else
	{
		dest.AllocBuffer(nNewLen);
		memcpy(dest.m_pchData, m_pchData+nCopyIndex, nCopyLen*sizeof(char));
	}
}

//////////////////////////////////////////////////////////////////////////////
// More sophisticated construction

CStringA::CStringA(LPCSTR lpsz)
{
	Init();
	if (lpsz != NULL && HIWORD(lpsz) == NULL)
	{
		//UINT nID = LOWORD((DWORD)lpsz);
		//if (!LoadString(nID))
		//	TRACE1("Warning: implicit LoadString(%u) failed\n", nID);
	}
	else
	{
		int nLen = SafeStrlen(lpsz);
		if (nLen != 0)
		{
			AllocBuffer(nLen);
			memcpy(m_pchData, lpsz, nLen*sizeof(char));
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion constructors

#ifdef _WIN32
CStringA::CStringA(LPCWSTR lpsz)
{
	Init();
	int nSrcLen = lpsz != NULL ? wcslen(lpsz) : 0;
	if (nSrcLen != 0)
	{
		AllocBuffer(nSrcLen*2);
		_wcstombsz(m_pchData, lpsz, (nSrcLen*2)+1);
		ReleaseBuffer();
	}
}
#endif
//////////////////////////////////////////////////////////////////////////////
// Diagnostic support

//////////////////////////////////////////////////////////////////////////////
// Assignment operators
//  All assign a new value to the string
//      (a) first see if the buffer is big enough
//      (b) if enough room, copy on top of old buffer, set size and type
//      (c) otherwise free old string data, and create a new one
//
//  All routines return the new string (but as a 'const CStringA&' so that
//      assigning it again will cause a copy, eg: s1 = s2 = "hi there".
//

void CStringA::AssignCopy(int nSrcLen, LPCSTR lpszSrcData)
{
	AllocBeforeWrite(nSrcLen);
	memcpy(m_pchData, lpszSrcData, nSrcLen*sizeof(char));
	GetData()->nDataLength = nSrcLen;
	m_pchData[nSrcLen] = '\0';
}

const CStringA& CStringA::operator=(const CStringA& stringSrc)
{
	if (m_pchData != stringSrc.m_pchData)
	{
		if ((GetData()->nRefs < 0 && GetData() != _afxDataNil) ||
			stringSrc.GetData()->nRefs < 0)
		{
			// actual copy necessary since one of the strings is locked
			AssignCopy(stringSrc.GetData()->nDataLength, stringSrc.m_pchData);
		}
		else
		{
			// can just copy references around
			Release();
			ASSERT(stringSrc.GetData() != _afxDataNil);
			m_pchData = stringSrc.m_pchData;
			InterlockedIncrement(&GetData()->nRefs);
		}
	}
	return *this;
}

const CStringA& CStringA::operator=(LPCSTR lpsz)
{
	AssignCopy(SafeStrlen(lpsz), lpsz);
	return *this;
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion assignment

#ifdef _WIN32
const CStringA& CStringA::operator=(LPCWSTR lpsz)
{
	int nSrcLen = lpsz != NULL ? wcslen(lpsz) : 0;
	AllocBeforeWrite(nSrcLen*2);
	_wcstombsz(m_pchData, lpsz, (nSrcLen*2)+1);
	ReleaseBuffer();
	return *this;
}
#endif

//////////////////////////////////////////////////////////////////////////////
// concatenation

// NOTE: "operator+" is done as friend functions for simplicity
//      There are three variants:
//          CStringA + CStringA
// and for ? = char, LPCSTR
//          CStringA + ?
//          ? + CStringA

void CStringA::ConcatCopy(int nSrc1Len, LPCSTR lpszSrc1Data, int nSrc2Len, LPCSTR lpszSrc2Data)
{
  // -- master concatenation routine
  // Concatenate two sources
  // -- assume that 'this' is a new CStringA object

	int nNewLen = nSrc1Len + nSrc2Len;
	if (nNewLen != 0)
	{
		AllocBuffer(nNewLen);
		memcpy(m_pchData, lpszSrc1Data, nSrc1Len*sizeof(char));
		memcpy(m_pchData+nSrc1Len, lpszSrc2Data, nSrc2Len*sizeof(char));
	}
}

CStringA operator+(const CStringA& s1, const CStringA& s2)
{
	CStringA s;
	s.ConcatCopy(s1.GetData()->nDataLength, s1.m_pchData, s2.GetData()->nDataLength, s2.m_pchData);
	return s;
}

CStringA operator+(const CStringA& string, LPCSTR lpsz)
{
	CStringA s;
	s.ConcatCopy(string.GetData()->nDataLength, string.m_pchData, CStringA::SafeStrlen(lpsz), lpsz);
	return s;
}

CStringA operator+(LPCSTR lpsz, const CStringA& string)
{
	CStringA s;
	s.ConcatCopy(CStringA::SafeStrlen(lpsz), lpsz, string.GetData()->nDataLength, string.m_pchData);
	return s;
}

//////////////////////////////////////////////////////////////////////////////
// concatenate in place

void CStringA::ConcatInPlace(int nSrcLen, LPCSTR lpszSrcData)
{
	//  -- the main routine for += operators

	// concatenating an empty string is a no-op!
	if (nSrcLen == 0)
		return;

	// if the buffer is too small, or we have a width mis-match, just
	//   allocate a new buffer (slow but sure)
	if (GetData()->nRefs > 1 || GetData()->nDataLength + nSrcLen > GetData()->nAllocLength)
	{
		// we have to grow the buffer, use the ConcatCopy routine
		CStringDataA* pOldData = GetData();
		ConcatCopy(GetData()->nDataLength, m_pchData, nSrcLen, lpszSrcData);
		ASSERT(pOldData != NULL);
		CStringA::Release(pOldData);
	}
	else
	{
		// fast concatenation when buffer big enough
		memcpy(m_pchData+GetData()->nDataLength, lpszSrcData, nSrcLen*sizeof(char));
		GetData()->nDataLength += nSrcLen;
		ASSERT(GetData()->nDataLength <= GetData()->nAllocLength);
		m_pchData[GetData()->nDataLength] = '\0';
	}
}

const CStringA& CStringA::operator+=(LPCSTR lpsz)
{
	ConcatInPlace(SafeStrlen(lpsz), lpsz);
	return *this;
}

const CStringA& CStringA::operator+=(char ch)
{
	ConcatInPlace(1, &ch);
	return *this;
}

const CStringA& CStringA::operator+=(const CStringA& string)
{
	ConcatInPlace(string.GetData()->nDataLength, string.m_pchData);
	return *this;
}

///////////////////////////////////////////////////////////////////////////////
// Advanced direct buffer access

LPSTR CStringA::GetBuffer(int nMinBufLength)
{
	ASSERT(nMinBufLength >= 0);

	if (GetData()->nRefs > 1 || nMinBufLength > GetData()->nAllocLength)
	{
//#ifdef _DEBUG
//		// give a warning in case locked string becomes unlocked
//		if (GetData() != _afxDataNil && GetData()->nRefs < 0)
//			TRACE0("Warning: GetBuffer on locked CStringA creates unlocked CStringA!\n");
//#endif
		// we have to grow the buffer
		CStringDataA* pOldData = GetData();
		int nOldLen = GetData()->nDataLength;   // AllocBuffer will tromp it
		if (nMinBufLength < nOldLen)
			nMinBufLength = nOldLen;
		AllocBuffer(nMinBufLength);
		memcpy(m_pchData, pOldData->data(), (nOldLen+1)*sizeof(char));
		GetData()->nDataLength = nOldLen;
		CStringA::Release(pOldData);
	}
	ASSERT(GetData()->nRefs <= 1);

	// return a pointer to the character storage for this string
	ASSERT(m_pchData != NULL);
	return m_pchData;
}

void CStringA::ReleaseBuffer(int nNewLength)
{
	CopyBeforeWrite();  // just in case GetBuffer was not called

	if (nNewLength == -1)
		nNewLength = lstrlenA(m_pchData); // zero terminated

	ASSERT(nNewLength <= GetData()->nAllocLength);
	GetData()->nDataLength = nNewLength;
	m_pchData[nNewLength] = '\0';
}

LPSTR CStringA::GetBufferSetLength(int nNewLength)
{
	ASSERT(nNewLength >= 0);

	GetBuffer(nNewLength);
	GetData()->nDataLength = nNewLength;
	m_pchData[nNewLength] = '\0';
	return m_pchData;
}

void CStringA::FreeExtra()
{
	ASSERT(GetData()->nDataLength <= GetData()->nAllocLength);
	if (GetData()->nDataLength != GetData()->nAllocLength)
	{
		CStringDataA* pOldData = GetData();
		AllocBuffer(GetData()->nDataLength);
		memcpy(m_pchData, pOldData->data(), pOldData->nDataLength*sizeof(char));
		ASSERT(m_pchData[GetData()->nDataLength] == '\0');
		CStringA::Release(pOldData);
	}
	ASSERT(GetData() != NULL);
}

LPSTR CStringA::LockBuffer()
{
	LPSTR lpsz = GetBuffer(0);
	GetData()->nRefs = -1;
	return lpsz;
}

void CStringA::UnlockBuffer()
{
	ASSERT(GetData()->nRefs == -1);
	if (GetData() != _afxDataNil)
		GetData()->nRefs = 1;
}

///////////////////////////////////////////////////////////////////////////////
// Commonly used routines (rarely used routines in STREX.CPP)

int CStringA::Find(char ch) const
{
	return Find(ch, 0);
}

int CStringA::Find(char ch, int nStart) const
{
	int nLength = GetData()->nDataLength;
	if (nStart >= nLength)
		return -1;

	// find first single character
	LPSTR lpsz = strchr(m_pchData + nStart, ch);

	// return -1 if not found and index otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

int CStringA::FindOneOf(LPCSTR lpszCharSet) const
{
	LPSTR lpsz = strpbrk(m_pchData, lpszCharSet);
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

void CStringA::MakeUpper()
{
	CopyBeforeWrite();
	strupr(m_pchData);
}

void CStringA::MakeLower()
{
	CopyBeforeWrite();
	strlwr(m_pchData);
}

void CStringA::MakeReverse()
{
	CopyBeforeWrite();
	strrev(m_pchData);
}

void CStringA::SetAt(int nIndex, char ch)
{
	ASSERT(nIndex >= 0);
	ASSERT(nIndex < GetData()->nDataLength);

	CopyBeforeWrite();
	m_pchData[nIndex] = ch;
}

/*
void CStringA::AnsiToOem()
{
	CopyBeforeWrite();
	::AnsiToOem(m_pchData, m_pchData);
}
void CStringA::OemToAnsi()
{
	CopyBeforeWrite();
	::OemToAnsi(m_pchData, m_pchData);
}
*/

///////////////////////////////////////////////////////////////////////////////
// CStringA conversion helpers (these use the current system locale)

#ifdef _WIN32
int _wcstombsz(char* mbstr, const wchar_t* wcstr, size_t count)
{
	if (count == 0 && mbstr != NULL)
		return 0;

	int result = ::WideCharToMultiByte(CP_ACP, 0, wcstr, -1, mbstr, count, NULL, NULL);
	ASSERT(mbstr == NULL || result <= (int)count);
	if (result > 0)
		mbstr[result-1] = 0;
	return result;
}

int _mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count)
{
	if (count == 0 && wcstr != NULL)
		return 0;

	int result = ::MultiByteToWideChar(CP_ACP, 0, mbstr, -1, wcstr, count);
	ASSERT(wcstr == NULL || result <= (int)count);
	if (result > 0)
		wcstr[result-1] = 0;
	return result;
}
#endif


///////////////////////////////////////////////////////////////////////////////


CStringDataA* CStringA::GetData() const
	{ ASSERT(m_pchData != NULL); return ((CStringDataA*)m_pchData)-1; }

void CStringA::Init()
	{ m_pchData = ((CStringA*)&_afxPchNil)->m_pchData; }

CStringA::CStringA(const unsigned char* lpsz)
	{ Init(); *this = (LPCSTR)lpsz; }

const CStringA& CStringA::operator=(const unsigned char* lpsz)
	{ *this = (LPCSTR)lpsz; return *this; }

int CStringA::GetLength() const
	{ return GetData()->nDataLength; }

int CStringA::GetAllocLength() const
	{ return GetData()->nAllocLength; }

BOOL CStringA::IsEmpty() const
{ 
	return GetData()->nDataLength == 0;
}

CStringA::operator LPCSTR() const
	{ return m_pchData; }

int CStringA::SafeStrlen(LPCSTR lpsz)
	{ return (lpsz == NULL) ? 0 : lstrlenA(lpsz); }

// CStringA support (windows specific)
int CStringA::Compare(LPCSTR lpsz) const
	{ return strcmp(m_pchData, lpsz); }    // MBCS/Unicode aware

int CStringA::CompareNoCase(LPCSTR lpsz) const
	{ return _stricmp(m_pchData, lpsz); }   // MBCS/Unicode aware

// CStringA::Collate is often slower than Compare but is MBSC/Unicode
//  aware as well as locale-sensitive with respect to sort order.
int CStringA::Collate(LPCSTR lpsz) const
	{ return strcoll(m_pchData, lpsz); }   // locale sensitive

#ifdef _WIN32
int CStringA::CollateNoCase(LPCSTR lpsz) const
	{ return _stricoll(m_pchData, lpsz); }   // locale sensitive
#endif

char CStringA::GetAt(int nIndex) const
{
	ASSERT(nIndex >= 0);
	ASSERT(nIndex < GetData()->nDataLength);
	return m_pchData[nIndex];
}

char CStringA::operator[](int nIndex) const
{
	// same as GetAt
	ASSERT(nIndex >= 0);
	ASSERT(nIndex < GetData()->nDataLength);
	return m_pchData[nIndex];
}

bool operator==(const CStringA& s1, const CStringA& s2)
{ 
	return s1.Compare(s2) == 0; 
}

bool operator==(const CStringA& s1, LPCSTR s2)
{ 
	return s1.Compare(s2) == 0; 
}

bool operator==(LPCSTR s1, const CStringA& s2)
{ 
	return s2.Compare(s1) == 0; 
}

bool operator!=(const CStringA& s1, const CStringA& s2)
{ 
	return s1.Compare(s2) != 0; 
}

bool operator!=(const CStringA& s1, LPCSTR s2)
{ 
	return s1.Compare(s2) != 0; 
}

bool operator!=(LPCSTR s1, const CStringA& s2)
{ 
	return s2.Compare(s1) != 0; 
}

bool operator<(const CStringA& s1, const CStringA& s2)
{ 
	return s1.Compare(s2) < 0; 
}

bool operator<(const CStringA& s1, LPCSTR s2)
{ 
	return s1.Compare(s2) < 0; 
}

bool operator<(LPCSTR s1, const CStringA& s2)
{ 
	return s2.Compare(s1) > 0; 
}

bool operator>(const CStringA& s1, const CStringA& s2)
{ 
	return s1.Compare(s2) > 0; 
}

bool operator>(const CStringA& s1, LPCSTR s2)
{ 
	return s1.Compare(s2) > 0; 
}

bool operator>(LPCSTR s1, const CStringA& s2)
{ 
	return s2.Compare(s1) < 0; 
}

bool operator<=(const CStringA& s1, const CStringA& s2)
{ 
	return s1.Compare(s2) <= 0; 
}

bool operator<=(const CStringA& s1, LPCSTR s2)
{ 
	return s1.Compare(s2) <= 0; 
}

bool operator<=(LPCSTR s1, const CStringA& s2)
{ 
	return s2.Compare(s1) >= 0; 
}

bool operator>=(const CStringA& s1, const CStringA& s2)
{ 
	return s1.Compare(s2) >= 0; 
}

bool operator>=(const CStringA& s1, LPCSTR s2)
{ 
	return s1.Compare(s2) >= 0; 
}

bool operator>=(LPCSTR s1, const CStringA& s2)
{ 
	return s2.Compare(s1) <= 0; 
}


//////////////////////////////////////////////////////////////////////////////
// More sophisticated construction

CStringA::CStringA(char ch, int nLength)
{
	Init();
	if (nLength >= 1)
	{
		AllocBuffer(nLength);
		memset(m_pchData, ch, nLength);
	}
}

CStringA::CStringA(LPCSTR lpch, int nLength)
{
	Init();
	if (nLength != 0)
	{
		AllocBuffer(nLength);
		memcpy(m_pchData, lpch, nLength*sizeof(char));
	}
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion constructors

#ifdef _WIN32
CStringA::CStringA(LPCWSTR lpsz, int nLength)
{
	Init();
	if (nLength != 0)
	{
		AllocBuffer(nLength*2);
		int n = ::WideCharToMultiByte(CP_ACP, 0, lpsz, nLength, m_pchData,
			(nLength*2)+1, NULL, NULL);
		ReleaseBuffer(n >= 0 ? n : -1);
	}
}
#endif

//////////////////////////////////////////////////////////////////////////////
// Assignment operators

const CStringA& CStringA::operator=(char ch)
{
	AssignCopy(1, &ch);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////
// less common string expressions

CStringA operator+(const CStringA& string1, char ch)
{
	CStringA s;
	s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData, 1, &ch);
	return s;
}

CStringA operator+(char ch, const CStringA& string)
{
	CStringA s;
	s.ConcatCopy(1, &ch, string.GetData()->nDataLength, string.m_pchData);
	return s;
}

//////////////////////////////////////////////////////////////////////////////
// Advanced manipulation

int CStringA::Delete(int nIndex, int nCount /* = 1 */)
{
	if (nIndex < 0)
		nIndex = 0;
	int nNewLength = GetData()->nDataLength;
	if (nCount > 0 && nIndex < nNewLength)
	{
		CopyBeforeWrite();
		int nBytesToCopy = nNewLength - (nIndex + nCount) + 1;

		memcpy(m_pchData + nIndex,
			m_pchData + nIndex + nCount, nBytesToCopy * sizeof(char));
		GetData()->nDataLength = nNewLength - nCount;
	}

	return nNewLength;
}

int CStringA::Insert(int nIndex, char ch)
{
	CopyBeforeWrite();

	if (nIndex < 0)
		nIndex = 0;

	int nNewLength = GetData()->nDataLength;
	if (nIndex > nNewLength)
		nIndex = nNewLength;
	nNewLength++;

	if (GetData()->nAllocLength < nNewLength)
	{
		CStringDataA* pOldData = GetData();
		LPSTR pstr = m_pchData;
		AllocBuffer(nNewLength);
		memcpy(m_pchData, pstr, (pOldData->nDataLength+1)*sizeof(char));
		CStringA::Release(pOldData);
	}

	// move existing bytes down
	memcpy(m_pchData + nIndex + 1,
		m_pchData + nIndex, (nNewLength-nIndex)*sizeof(char));
	m_pchData[nIndex] = ch;
	GetData()->nDataLength = nNewLength;

	return nNewLength;
}

int CStringA::Insert(int nIndex, LPCSTR pstr)
{
	if (nIndex < 0)
		nIndex = 0;

	int nInsertLength = SafeStrlen(pstr);
	int nNewLength = GetData()->nDataLength;
	if (nInsertLength > 0)
	{
		CopyBeforeWrite();
		if (nIndex > nNewLength)
			nIndex = nNewLength;
		nNewLength += nInsertLength;

		if (GetData()->nAllocLength < nNewLength)
		{
			CStringDataA* pOldData = GetData();
			LPSTR pstr = m_pchData;
			AllocBuffer(nNewLength);
			memcpy(m_pchData, pstr, (pOldData->nDataLength+1)*sizeof(char));
			CStringA::Release(pOldData);
		}

		// move existing bytes down
		memcpy(m_pchData + nIndex + nInsertLength,
			m_pchData + nIndex,
			(nNewLength-nIndex-nInsertLength+1)*sizeof(char));
		memcpy(m_pchData + nIndex,
			pstr, nInsertLength*sizeof(char));
		GetData()->nDataLength = nNewLength;
	}

	return nNewLength;
}

int CStringA::Replace(char chOld, char chNew)
{
	int nCount = 0;

	// short-circuit the nop case
	if (chOld != chNew)
	{
		// otherwise modify each character that matches in the string
		CopyBeforeWrite();
		LPSTR psz = m_pchData;
		LPSTR pszEnd = psz + GetData()->nDataLength;
		while (psz < pszEnd)
		{
			// replace instances of the specified character only
			if (*psz == chOld)
			{
				*psz = chNew;
				nCount++;
			}
			psz++;
		}
	}
	return nCount;
}

int CStringA::Replace(LPCSTR lpszOld, LPCSTR lpszNew)
{
	// can't have empty or NULL lpszOld

	int nSourceLen = SafeStrlen(lpszOld);
	if (nSourceLen == 0)
		return 0;
	int nReplacementLen = SafeStrlen(lpszNew);

	// loop once to figure out the size of the result string
	int nCount = 0;
	LPSTR lpszStart = m_pchData;
	LPSTR lpszEnd = m_pchData + GetData()->nDataLength;
	LPSTR lpszTarget;
	while (lpszStart < lpszEnd)
	{
		while ((lpszTarget = strstr(lpszStart, lpszOld)) != NULL)
		{
			nCount++;
			lpszStart = lpszTarget + nSourceLen;
		}
		lpszStart += lstrlenA(lpszStart) + 1;
	}

	// if any changes were made, make them
	if (nCount > 0)
	{
		CopyBeforeWrite();

		// if the buffer is too small, just
		//   allocate a new buffer (slow but sure)
		int nOldLength = GetData()->nDataLength;
		int nNewLength =  nOldLength + (nReplacementLen-nSourceLen)*nCount;
		if (GetData()->nAllocLength < nNewLength || GetData()->nRefs > 1)
		{
			CStringDataA* pOldData = GetData();
			LPSTR pstr = m_pchData;
			AllocBuffer(nNewLength);
			memcpy(m_pchData, pstr, pOldData->nDataLength*sizeof(char));
			CStringA::Release(pOldData);
		}
		// else, we just do it in-place
		lpszStart = m_pchData;
		lpszEnd = m_pchData + GetData()->nDataLength;

		// loop again to actually do the work
		while (lpszStart < lpszEnd)
		{
			while ( (lpszTarget = strstr(lpszStart, lpszOld)) != NULL)
			{
				int nBalance = nOldLength - (lpszTarget - m_pchData + nSourceLen);
				memmove(lpszTarget + nReplacementLen, lpszTarget + nSourceLen,
					nBalance * sizeof(char));
				memcpy(lpszTarget, lpszNew, nReplacementLen*sizeof(char));
				lpszStart = lpszTarget + nReplacementLen;
				lpszStart[nBalance] = '\0';
				nOldLength += (nReplacementLen - nSourceLen);
			}
			lpszStart += lstrlenA(lpszStart) + 1;
		}
		ASSERT(m_pchData[nNewLength] == '\0');
		GetData()->nDataLength = nNewLength;
	}

	return nCount;
}

int CStringA::Remove(char chRemove)
{
	CopyBeforeWrite();

	LPSTR pstrSource = m_pchData;
	LPSTR pstrDest = m_pchData;
	LPSTR pstrEnd = m_pchData + GetData()->nDataLength;

	while (pstrSource < pstrEnd)
	{
		if (*pstrSource != chRemove)
		{
			*pstrDest = *pstrSource;
			pstrDest++;
		}
		pstrSource++;
	}
	*pstrDest = '\0';
	int nCount = pstrSource - pstrDest;
	GetData()->nDataLength -= nCount;

	return nCount;
}

//////////////////////////////////////////////////////////////////////////////
// Very simple sub-string extraction

CStringA CStringA::Mid(int nFirst) const
{
	return Mid(nFirst, GetData()->nDataLength - nFirst);
}

CStringA CStringA::Mid(int nFirst, int nCount) const
{
	// out-of-bounds requests return sensible things
	if (nFirst < 0)
		nFirst = 0;
	if (nCount < 0)
		nCount = 0;

	if (nFirst + nCount > GetData()->nDataLength)
		nCount = GetData()->nDataLength - nFirst;
	if (nFirst > GetData()->nDataLength)
		nCount = 0;

	ASSERT(nFirst >= 0);
	ASSERT(nFirst + nCount <= GetData()->nDataLength);

	// optimize case of returning entire string
	if (nFirst == 0 && nFirst + nCount == GetData()->nDataLength)
		return *this;

	CStringA dest;
	AllocCopy(dest, nCount, nFirst, 0);
	return dest;
}

CStringA CStringA::Right(int nCount) const
{
	if (nCount < 0)
		nCount = 0;
	if (nCount >= GetData()->nDataLength)
		return *this;

	CStringA dest;
	AllocCopy(dest, nCount, GetData()->nDataLength-nCount, 0);
	return dest;
}

CStringA CStringA::Left(int nCount) const
{
	if (nCount < 0)
		nCount = 0;
	if (nCount >= GetData()->nDataLength)
		return *this;

	CStringA dest;
	AllocCopy(dest, nCount, 0, 0);
	return dest;
}

/*
// strspn equivalent
CStringA CStringA::SpanIncluding(LPCSTR lpszCharSet) const
{
	return Left(_tcsspn(m_pchData, lpszCharSet));
}

// strcspn equivalent
CStringA CStringA::SpanExcluding(LPCSTR lpszCharSet) const
{
	return Left(_tcscspn(m_pchData, lpszCharSet));
}
*/

//////////////////////////////////////////////////////////////////////////////
// Finding

int CStringA::ReverseFind(char ch) const
{
	// find last single character
	LPSTR lpsz = strrchr(m_pchData, ch);

	// return -1 if not found, distance from beginning otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

// find a sub-string (like strstr)
int CStringA::Find(LPCSTR lpszSub) const
{
	return Find(lpszSub, 0);
}

int CStringA::Find(LPCSTR lpszSub, int nStart) const
{
	int nLength = GetData()->nDataLength;
	if (nStart > nLength)
		return -1;

	// find first matching substring
	LPSTR lpsz = strstr(m_pchData + nStart, lpszSub);

	// return -1 for not found, distance from beginning otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}


/////////////////////////////////////////////////////////////////////////////
// CStringA formatting


#define FORCE_ANSI      0x10000
#define FORCE_UNICODE   0x20000
#define FORCE_INT64     0x40000

void CStringA::FormatV(LPCSTR lpszFormat, va_list argList)
{
	va_list argListSave = argList;

	// make a guess at the maximum length of the resulting string
	int nMaxLen = 0;
	for (LPCSTR lpsz = lpszFormat; *lpsz != '\0'; lpsz++)
	{
		// handle '%' character, but watch out for '%%'
		if (*lpsz != '%' || *(++lpsz) == '%')
		{
			nMaxLen += strlen(lpsz);
			continue;
		}

		int nItemLen = 0;

		// handle '%' character with format
		int nWidth = 0;
		for (; *lpsz != '\0'; lpsz++)
		{
			// check for valid flags
			if (*lpsz == '#')
				nMaxLen += 2;   // for '0x'
			else if (*lpsz == '*')
				nWidth = va_arg(argList, int);
			else if (*lpsz == '-' || *lpsz == '+' || *lpsz == '0' ||
				*lpsz == ' ')
				;
			else // hit non-flag character
				break;
		}
		// get width and skip it
		if (nWidth == 0)
		{
			// width indicated by
			nWidth = atoi(lpsz);
			for (; *lpsz != '\0' && isdigit(*lpsz); lpsz++)
				;
		}
		ASSERT(nWidth >= 0);

		int nPrecision = 0;
		if (*lpsz == '.')
		{
			// skip past '.' separator (width.precision)
			lpsz++;

			// get precision and skip it
			if (*lpsz == '*')
			{
				nPrecision = va_arg(argList, int);
				lpsz++;
			}
			else
			{
				nPrecision = atoi(lpsz);
				for (; *lpsz != '\0' && isdigit(*lpsz); lpsz++)
					;
			}
			ASSERT(nPrecision >= 0);
		}

		// should be on type modifier or specifier
		int nModifier = 0;
		if (strncmp(lpsz, "I64", 3) == 0)
		{
			lpsz += 3;
			nModifier = FORCE_INT64;
		}
		else
		{
			switch (*lpsz)
			{
			// modifiers that affect size
			case 'h':
				nModifier = FORCE_ANSI;
				lpsz++;
				break;
			case 'l':
				nModifier = FORCE_UNICODE;
				lpsz++;
				break;

			// modifiers that do not affect size
			case 'F':
			case 'N':
			case 'L':
				lpsz++;
				break;
			}
		}

		// now should be on specifier
		switch (*lpsz | nModifier)
		{
		// single characters
		case 'c':
		case 'C':
			nItemLen = 2;
			va_arg(argList, int); // char
			break;
		case 'c'|FORCE_ANSI:
		case 'C'|FORCE_ANSI:
			nItemLen = 2;
			va_arg(argList, int); // char
			break;
		case 'c'|FORCE_UNICODE:
		case 'C'|FORCE_UNICODE:
			nItemLen = 2;
			va_arg(argList, int); //WCHAR
			break;

		// strings
		case 's':
			{
				LPCSTR pstrNextArg = va_arg(argList, LPCSTR);
				if (pstrNextArg == NULL)
				   nItemLen = 6;  // "(null)"
				else
				{
				   nItemLen = lstrlenA(pstrNextArg);
				   nItemLen = max(1, nItemLen);
				}
			}
			break;

		case 'S':
			{
				LPWSTR pstrNextArg = va_arg(argList, LPWSTR);
				if (pstrNextArg == NULL)
				   nItemLen = 6;  // "(null)"
				else
				{
				   nItemLen = wcslen(pstrNextArg);
				   nItemLen = max(1, nItemLen);
				}
			}
			break;

		case 's'|FORCE_ANSI:
		case 'S'|FORCE_ANSI:
			{
				LPCSTR pstrNextArg = va_arg(argList, LPCSTR);
				if (pstrNextArg == NULL)
				   nItemLen = 6; // "(null)"
				else
				{
				   nItemLen = lstrlenA(pstrNextArg);
				   nItemLen = max(1, nItemLen);
				}
			}
			break;

		case 's'|FORCE_UNICODE:
		case 'S'|FORCE_UNICODE:
			{
				LPWSTR pstrNextArg = va_arg(argList, LPWSTR);
				if (pstrNextArg == NULL)
				   nItemLen = 6; // "(null)"
				else
				{
				   nItemLen = wcslen(pstrNextArg);
				   nItemLen = max(1, nItemLen);
				}
			}
			break;
		}

		// adjust nItemLen for strings
		if (nItemLen != 0)
		{
			if (nPrecision != 0)
				nItemLen = min(nItemLen, nPrecision);
			nItemLen = max(nItemLen, nWidth);
		}
		else
		{
			switch (*lpsz)
			{
			// integers
			case 'd':
			case 'i':
			case 'u':
			case 'x':
			case 'X':
			case 'o':
				if (nModifier & FORCE_INT64)
					va_arg(argList, __int64);
				else
					va_arg(argList, int);
				nItemLen = 32;
				nItemLen = max(nItemLen, nWidth+nPrecision);
				break;

			case 'e':
			case 'g':
			case 'G':
				va_arg(argList, double);
				nItemLen = 128;
				nItemLen = max(nItemLen, nWidth+nPrecision);
				break;

			case 'f':
				{
					double f;
					LPSTR pszTemp;

					// 312 == strlen("-1+(309 zeroes).")
					// 309 zeroes == max precision of a double
					// 6 == adjustment in case precision is not specified,
					//   which means that the precision defaults to 6
					pszTemp = (LPSTR)_alloca(max(nWidth, 312+nPrecision+6));

					f = va_arg(argList, double);
					sprintf( pszTemp, "%*.*f", nWidth, nPrecision+6, f );
					nItemLen = strlen(pszTemp);
				}
				break;

			case 'p':
				va_arg(argList, void*);
				nItemLen = 32;
				nItemLen = max(nItemLen, nWidth+nPrecision);
				break;

			// no output
			case 'n':
				va_arg(argList, int*);
				break;

			default:
				ASSERT(FALSE);  // unknown formatting option
			}
		}

		// adjust nMaxLen for output nItemLen
		nMaxLen += nItemLen;
	}

	GetBuffer(nMaxLen);
	VERIFY(vsprintf(m_pchData, lpszFormat, argListSave) <= GetAllocLength());
	ReleaseBuffer();

	va_end(argListSave);
}

// formatting (using wsprintf style formatting)
void CStringA::Format(LPCSTR lpszFormat, ...)
{
	va_list argList;
	va_start(argList, lpszFormat);
	FormatV(lpszFormat, argList);
	va_end(argList);
}

/*
void CStringA::Format(UINT nFormatID, ...)
{
	CStringA strFormat;
	VERIFY(strFormat.LoadString(nFormatID) != 0);

	va_list argList;
	va_start(argList, nFormatID);
	FormatV(strFormat, argList);
	va_end(argList);
}


// formatting (using FormatMessage style formatting)
void CStringA::FormatMessage(LPCSTR lpszFormat, ...)
{
	// format message into temporary buffer lpszTemp
	va_list argList;
	va_start(argList, lpszFormat);
	LPSTR lpszTemp;

	if (::FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
		lpszFormat, 0, 0, (LPSTR)&lpszTemp, 0, &argList) == 0 ||
		lpszTemp == NULL)
	{
		//AfxThrowMemoryException();
		return;
	}

	// assign lpszTemp into the resulting string and free the temporary
	*this = lpszTemp;
	LocalFree(lpszTemp);
	va_end(argList);
}
*/

/*
void CStringA::FormatMessage(UINT nFormatID, ...)
{
	// get format string from string table
	CStringA strFormat;
	VERIFY(strFormat.LoadString(nFormatID) != 0);

	// format message into temporary buffer lpszTemp
	va_list argList;
	va_start(argList, nFormatID);
	LPSTR lpszTemp;
	if (::FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
		strFormat, 0, 0, (LPSTR)&lpszTemp, 0, &argList) == 0 ||
		lpszTemp == NULL)
	{
		//AfxThrowMemoryException();
		return;
	}

	// assign lpszTemp into the resulting string and free lpszTemp
	*this = lpszTemp;
	LocalFree(lpszTemp);
	va_end(argList);
}
*/

/*
void CStringA::TrimRight(LPCSTR lpszTargetList)
{
	// find beginning of trailing matches
	// by starting at beginning (DBCS aware)

	CopyBeforeWrite();
	LPSTR lpsz = m_pchData;
	LPSTR lpszLast = NULL;

	while (*lpsz != '\0')
	{
		if (strchr(lpszTargetList, *lpsz) != NULL)
		{
			if (lpszLast == NULL)
				lpszLast = lpsz;
		}
		else
			lpszLast = NULL;
		lpsz = _tcsinc(lpsz);
	}

	if (lpszLast != NULL)
	{
		// truncate at left-most matching character
		*lpszLast = '\0';
		GetData()->nDataLength = lpszLast - m_pchData;
	}
}

void CStringA::TrimRight(char chTarget)
{
	// find beginning of trailing matches
	// by starting at beginning (DBCS aware)

	CopyBeforeWrite();
	LPSTR lpsz = m_pchData;
	LPSTR lpszLast = NULL;

	while (*lpsz != '\0')
	{
		if (*lpsz == chTarget)
		{
			if (lpszLast == NULL)
				lpszLast = lpsz;
		}
		else
			lpszLast = NULL;
		lpsz = _tcsinc(lpsz);
	}

	if (lpszLast != NULL)
	{
		// truncate at left-most matching character
		*lpszLast = '\0';
		GetData()->nDataLength = lpszLast - m_pchData;
	}
}
*/

void CStringA::TrimRight()
{
	// find beginning of trailing spaces by starting at beginning (DBCS aware)

	CopyBeforeWrite();
	LPSTR lpsz = m_pchData;
	LPSTR lpszLast = NULL;

	while (*lpsz != '\0')
	{
		if (isspace(*lpsz))
		{
			if (lpszLast == NULL)
				lpszLast = lpsz;
		}
		else
			lpszLast = NULL;
		lpsz++;
	}

	if (lpszLast != NULL)
	{
		// truncate at trailing space start
		*lpszLast = '\0';
		GetData()->nDataLength = lpszLast - m_pchData;
	}
}

/*
void CStringA::TrimLeft(LPCSTR lpszTargets)
{
	// if we're not trimming anything, we're not doing any work
	if (SafeStrlen(lpszTargets) == 0)
		return;

	CopyBeforeWrite();
	LPCSTR lpsz = m_pchData;

	while (*lpsz != '\0')
	{
		if (strchr(lpszTargets, *lpsz) == NULL)
			break;
		lpsz = _tcsinc(lpsz);
	}

	if (lpsz != m_pchData)
	{
		// fix up data and length
		int nDataLength = GetData()->nDataLength - (lpsz - m_pchData);
		memmove(m_pchData, lpsz, (nDataLength+1)*sizeof(char));
		GetData()->nDataLength = nDataLength;
	}
}

void CStringA::TrimLeft(char chTarget)
{
	// find first non-matching character

	CopyBeforeWrite();
	LPCSTR lpsz = m_pchData;

	while (chTarget == *lpsz)
		lpsz = _tcsinc(lpsz);

	if (lpsz != m_pchData)
	{
		// fix up data and length
		int nDataLength = GetData()->nDataLength - (lpsz - m_pchData);
		memmove(m_pchData, lpsz, (nDataLength+1)*sizeof(char));
		GetData()->nDataLength = nDataLength;
	}
}
*/

void CStringA::TrimLeft()
{
	// find first non-space character
	CopyBeforeWrite();
	LPCSTR lpsz = m_pchData;

	while (isspace(*lpsz)) lpsz++;

	if (lpsz != m_pchData)
	{
		// fix up data and length
		int nDataLength = GetData()->nDataLength - (lpsz - m_pchData);
		memmove(m_pchData, lpsz, (nDataLength+1)*sizeof(char));
		GetData()->nDataLength = nDataLength;
	}
}


//#ifdef __wtypes_h__
//BSTR CStringA::AllocSysString() const
//{
//	int nLen = MultiByteToWideChar(CP_ACP, 0, m_pchData, GetData()->nDataLength, NULL, NULL);
//	BSTR bstr = ::SysAllocStringLen(NULL, nLen);
//	ASSERT(bstr != NULL)
//		
//	MultiByteToWideChar(CP_ACP, 0, m_pchData, GetData()->nDataLength, bstr, nLen);
//
//	return bstr;
//}
//#endif
