#include "stdafx.h"
#include "StringW.h"
#include <malloc.h>
#include <stdlib.h>
#include <limits.h>

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#pragma warning(disable: 4390)  // empty controlled statement


//#define TRACE0(sz)
//#define TRACE1(sz, p1)

// Standard constants
#undef FALSE
#undef NULL

#define FALSE   0
#define NULL    0


/////////////////////////////////////////////////////////////////////////////
// static class data, special inlines

// afxChNilW is left for backward compatibility
WCHAR afxChNilW = '\0';

// For an empty string, m_pchData will point here
// (note: avoids special case of checking for NULL m_pchData)
// empty string data (and locked)
int _afxInitDataW[] = { -1, 0, 0, 0 };
CStringDataW* _afxDataNilW = (CStringDataW*)&_afxInitDataW;
LPCWSTR _afxPchNilW = (LPCWSTR)(((BYTE*)&_afxInitDataW)+sizeof(CStringDataW));


//const CStringW& AfxGetEmptyString() { return *(CStringW*)&_afxPchNilW; }

//////////////////////////////////////////////////////////////////////////////
// Construction/Destruction


CStringW::CStringW()
{
	Init();
}

CStringW::CStringW(const CStringW& stringSrc)
{
	ASSERT(stringSrc.GetData()->nRefs != 0);
	if (stringSrc.GetData()->nRefs >= 0)
	{
		ASSERT(stringSrc.GetData() != _afxDataNilW);
		m_pchData = stringSrc.m_pchData;
		InterlockedIncrement(&GetData()->nRefs);
	}
	else
	{
		Init();
		*this = stringSrc.m_pchData;
	}
}


void CStringW::AllocBuffer(int nLen)
// always allocate one extra character for '\0' termination
// assumes [optimistically] that data length will equal allocation length
{
	ASSERT(nLen >= 0);
	ASSERT(nLen <= INT_MAX-1);    // max size (enough room for 1 extra)

	if (nLen == 0)
		Init();
	else
	{
		CStringDataW* pData;

		{
			pData = (CStringDataW*) new BYTE[sizeof(CStringDataW) + (nLen+1)*sizeof(WCHAR)];
			pData->nAllocLength = nLen;
		}
		pData->nRefs = 1;
		pData->data()[nLen] = '\0';
		pData->nDataLength = nLen;
		m_pchData = pData->data();
	}
}

void CStringW::FreeData(CStringDataW* pData)
{
	delete[] (BYTE*)pData;
}

void CStringW::Release()
{
	if (GetData() != _afxDataNilW)
	{
		ASSERT(GetData()->nRefs != 0);
		if (InterlockedDecrement(&GetData()->nRefs) <= 0)
			FreeData(GetData());
		Init();
	}
}

void PASCAL CStringW::Release(CStringDataW* pData)
{
	if (pData != _afxDataNilW)
	{
		ASSERT(pData->nRefs != 0);
		if (InterlockedDecrement(&pData->nRefs) <= 0)
			FreeData(pData);
	}
}

void CStringW::Empty()
{
	if (GetData()->nDataLength == 0)
		return;
	if (GetData()->nRefs >= 0)
		Release();
	else
		*this = &afxChNilW;
	ASSERT(GetData()->nDataLength == 0);
	ASSERT(GetData()->nRefs < 0 || GetData()->nAllocLength == 0);
}

void CStringW::CopyBeforeWrite()
{
	if (GetData()->nRefs > 1)
	{
		CStringDataW* pData = GetData();
		Release();
		AllocBuffer(pData->nDataLength);
		memcpy(m_pchData, pData->data(), (pData->nDataLength+1)*sizeof(WCHAR));
	}
	ASSERT(GetData()->nRefs <= 1);
}

void CStringW::AllocBeforeWrite(int nLen)
{
	if (GetData()->nRefs > 1 || nLen > GetData()->nAllocLength)
	{
		Release();
		AllocBuffer(nLen);
	}
	ASSERT(GetData()->nRefs <= 1);
}

CStringW::~CStringW()
//  free any attached data
{
	if (GetData() != _afxDataNilW)
	{
		if (InterlockedDecrement(&GetData()->nRefs) <= 0)
			FreeData(GetData());
	}
}

//////////////////////////////////////////////////////////////////////////////
// Helpers for the rest of the implementation

void CStringW::AllocCopy(CStringW& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const
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
		memcpy(dest.m_pchData, m_pchData+nCopyIndex, nCopyLen*sizeof(WCHAR));
	}
}

//////////////////////////////////////////////////////////////////////////////
// More sophisticated construction

CStringW::CStringW(LPCWSTR lpsz)
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
			memcpy(m_pchData, lpsz, nLen*sizeof(WCHAR));
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion constructors



CStringW::CStringW(LPCSTR lpsz)
{
	Init();
	int nSrcLen = lpsz != NULL ? lstrlenA(lpsz) : 0;
	if (nSrcLen != 0)
	{
		AllocBuffer(nSrcLen);
		_mbstowcsz(m_pchData, lpsz, nSrcLen+1);
		ReleaseBuffer();
	}
}

//////////////////////////////////////////////////////////////////////////////
// Diagnostic support

//////////////////////////////////////////////////////////////////////////////
// Assignment operators
//  All assign a new value to the string
//      (a) first see if the buffer is big enough
//      (b) if enough room, copy on top of old buffer, set size and type
//      (c) otherwise free old string data, and create a new one
//
//  All routines return the new string (but as a 'const CStringW&' so that
//      assigning it again will cause a copy, eg: s1 = s2 = "hi there".
//

void CStringW::AssignCopy(int nSrcLen, LPCWSTR lpszSrcData)
{
	AllocBeforeWrite(nSrcLen);
	memcpy(m_pchData, lpszSrcData, nSrcLen*sizeof(WCHAR));
	GetData()->nDataLength = nSrcLen;
	m_pchData[nSrcLen] = '\0';
}

const CStringW& CStringW::operator=(const CStringW& stringSrc)
{
	if (m_pchData != stringSrc.m_pchData)
	{
		if ((GetData()->nRefs < 0 && GetData() != _afxDataNilW) ||
			stringSrc.GetData()->nRefs < 0)
		{
			// actual copy necessary since one of the strings is locked
			AssignCopy(stringSrc.GetData()->nDataLength, stringSrc.m_pchData);
		}
		else
		{
			// can just copy references around
			Release();
			ASSERT(stringSrc.GetData() != _afxDataNilW);
			m_pchData = stringSrc.m_pchData;
			InterlockedIncrement(&GetData()->nRefs);
		}
	}
	return *this;
}

const CStringW& CStringW::operator=(LPCWSTR lpsz)
{
	AssignCopy(SafeStrlen(lpsz), lpsz);
	return *this;
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion assignment


const CStringW& CStringW::operator=(LPCSTR lpsz)
{
	int nSrcLen = lpsz != NULL ? lstrlenA(lpsz) : 0;
	AllocBeforeWrite(nSrcLen);
	mbstowcs(m_pchData, lpsz, nSrcLen+1);
	ReleaseBuffer();
	return *this;
}

//////////////////////////////////////////////////////////////////////////////
// concatenation

// NOTE: "operator+" is done as friend functions for simplicity
//      There are three variants:
//          CStringW + CStringW
// and for ? = WCHAR, LPCWSTR
//          CStringW + ?
//          ? + CStringW

void CStringW::ConcatCopy(int nSrc1Len, LPCWSTR lpszSrc1Data, int nSrc2Len, LPCWSTR lpszSrc2Data)
{
  // -- master concatenation routine
  // Concatenate two sources
  // -- assume that 'this' is a new CStringW object

	int nNewLen = nSrc1Len + nSrc2Len;
	if (nNewLen != 0)
	{
		AllocBuffer(nNewLen);
		memcpy(m_pchData, lpszSrc1Data, nSrc1Len*sizeof(WCHAR));
		memcpy(m_pchData+nSrc1Len, lpszSrc2Data, nSrc2Len*sizeof(WCHAR));
	}
}

CStringW operator+(const CStringW& string1, const CStringW& string2)
{
	CStringW s;
	s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData,
		string2.GetData()->nDataLength, string2.m_pchData);
	return s;
}

CStringW operator+(const CStringW& string, LPCWSTR lpsz)
{
	CStringW s;
	s.ConcatCopy(string.GetData()->nDataLength, string.m_pchData,
		CStringW::SafeStrlen(lpsz), lpsz);
	return s;
}

CStringW operator+(LPCWSTR lpsz, const CStringW& string)
{
	CStringW s;
	s.ConcatCopy(CStringW::SafeStrlen(lpsz), lpsz, string.GetData()->nDataLength,
		string.m_pchData);
	return s;
}

//////////////////////////////////////////////////////////////////////////////
// concatenate in place

void CStringW::ConcatInPlace(int nSrcLen, LPCWSTR lpszSrcData)
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
		CStringDataW* pOldData = GetData();
		ConcatCopy(GetData()->nDataLength, m_pchData, nSrcLen, lpszSrcData);
		ASSERT(pOldData != NULL);
		CStringW::Release(pOldData);
	}
	else
	{
		// fast concatenation when buffer big enough
		memcpy(m_pchData+GetData()->nDataLength, lpszSrcData, nSrcLen*sizeof(WCHAR));
		GetData()->nDataLength += nSrcLen;
		ASSERT(GetData()->nDataLength <= GetData()->nAllocLength);
		m_pchData[GetData()->nDataLength] = '\0';
	}
}

const CStringW& CStringW::operator+=(LPCWSTR lpsz)
{
	ConcatInPlace(SafeStrlen(lpsz), lpsz);
	return *this;
}

const CStringW& CStringW::operator+=(WCHAR ch)
{
	ConcatInPlace(1, &ch);
	return *this;
}

const CStringW& CStringW::operator+=(const CStringW& string)
{
	ConcatInPlace(string.GetData()->nDataLength, string.m_pchData);
	return *this;
}

///////////////////////////////////////////////////////////////////////////////
// Advanced direct buffer access

LPWSTR CStringW::GetBuffer(int nMinBufLength)
{
	ASSERT(nMinBufLength >= 0);

	if (GetData()->nRefs > 1 || nMinBufLength > GetData()->nAllocLength)
	{
//#ifdef _DEBUG
//		// give a warning in case locked string becomes unlocked
//		if (GetData() != _afxDataNilW && GetData()->nRefs < 0)
//			TRACE0("Warning: GetBuffer on locked CStringW creates unlocked CStringW!\n");
//#endif
		// we have to grow the buffer
		CStringDataW* pOldData = GetData();
		int nOldLen = GetData()->nDataLength;   // AllocBuffer will tromp it
		if (nMinBufLength < nOldLen)
			nMinBufLength = nOldLen;
		AllocBuffer(nMinBufLength);
		memcpy(m_pchData, pOldData->data(), (nOldLen+1)*sizeof(WCHAR));
		GetData()->nDataLength = nOldLen;
		CStringW::Release(pOldData);
	}
	ASSERT(GetData()->nRefs <= 1);

	// return a pointer to the character storage for this string
	ASSERT(m_pchData != NULL);
	return m_pchData;
}

void CStringW::ReleaseBuffer(int nNewLength)
{
	CopyBeforeWrite();  // just in case GetBuffer was not called

	if (nNewLength == -1)
		nNewLength = lstrlenW(m_pchData); // zero terminated

	ASSERT(nNewLength <= GetData()->nAllocLength);
	GetData()->nDataLength = nNewLength;
	m_pchData[nNewLength] = '\0';
}

LPWSTR CStringW::GetBufferSetLength(int nNewLength)
{
	ASSERT(nNewLength >= 0);

	GetBuffer(nNewLength);
	GetData()->nDataLength = nNewLength;
	m_pchData[nNewLength] = '\0';
	return m_pchData;
}

void CStringW::FreeExtra()
{
	ASSERT(GetData()->nDataLength <= GetData()->nAllocLength);
	if (GetData()->nDataLength != GetData()->nAllocLength)
	{
		CStringDataW* pOldData = GetData();
		AllocBuffer(GetData()->nDataLength);
		memcpy(m_pchData, pOldData->data(), pOldData->nDataLength*sizeof(WCHAR));
		ASSERT(m_pchData[GetData()->nDataLength] == '\0');
		CStringW::Release(pOldData);
	}
	ASSERT(GetData() != NULL);
}

LPWSTR CStringW::LockBuffer()
{
	LPWSTR lpsz = GetBuffer(0);
	GetData()->nRefs = -1;
	return lpsz;
}

void CStringW::UnlockBuffer()
{
	ASSERT(GetData()->nRefs == -1);
	if (GetData() != _afxDataNilW)
		GetData()->nRefs = 1;
}

///////////////////////////////////////////////////////////////////////////////
// Commonly used routines (rarely used routines in STREX.CPP)

int CStringW::Find(WCHAR ch) const
{
	return Find(ch, 0);
}

int CStringW::Find(WCHAR ch, int nStart) const
{
	int nLength = GetData()->nDataLength;
	if (nStart >= nLength)
		return -1;

	// find first single character
	LPWSTR lpsz = wcschr(m_pchData + nStart, (WCHAR)ch);

	// return -1 if not found and index otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

int CStringW::FindOneOf(LPCWSTR lpszCharSet) const
{
	LPWSTR lpsz = wcspbrk(m_pchData, lpszCharSet);
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

void CStringW::MakeUpper()
{
	CopyBeforeWrite();
	wcsupr(m_pchData);
}

void CStringW::MakeLower()
{
	CopyBeforeWrite();
	wcslwr(m_pchData);
}

void CStringW::MakeReverse()
{
	CopyBeforeWrite();
	wcsrev(m_pchData);
}

void CStringW::SetAt(int nIndex, WCHAR ch)
{
	ASSERT(nIndex >= 0);
	ASSERT(nIndex < GetData()->nDataLength);

	CopyBeforeWrite();
	m_pchData[nIndex] = ch;
}

/*
#ifndef _UNICODE
void CStringW::AnsiToOem()
{
	CopyBeforeWrite();
	::AnsiToOem(m_pchData, m_pchData);
}
void CStringW::OemToAnsi()
{
	CopyBeforeWrite();
	::OemToAnsi(m_pchData, m_pchData);
}
#endif
*/

///////////////////////////////////////////////////////////////////////////////
// CStringW conversion helpers (these use the current system locale)

/*
int _wcstombsz(char* mbstr, const wchar_t* wcstr, size_t count)
{
	if (count == 0 && mbstr != NULL)
		return 0;

	int result = ::WideCharToMultiByte(CP_ACP, 0, wcstr, -1,
		mbstr, count, NULL, NULL);
	ASSERT(mbstr == NULL || result <= (int)count);
	if (result > 0)
		mbstr[result-1] = 0;
	return result;
}

int _mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count)
{
	if (count == 0 && wcstr != NULL)
		return 0;

	int result = ::MultiByteToWideChar(CP_ACP, 0, mbstr, -1,
		wcstr, count);
	ASSERT(wcstr == NULL || result <= (int)count);
	if (result > 0)
		wcstr[result-1] = 0;
	return result;
}
*/


///////////////////////////////////////////////////////////////////////////////


CStringDataW* CStringW::GetData() const
	{ ASSERT(m_pchData != NULL); return ((CStringDataW*)m_pchData)-1; }

void CStringW::Init()
	{ m_pchData = ((CStringW*)&_afxPchNilW)->m_pchData; }


CStringW::CStringW(const unsigned char* lpsz)
	{ Init(); *this = (LPCSTR)lpsz; }

const CStringW& CStringW::operator=(const unsigned char* lpsz)
	{ *this = (LPCSTR)lpsz; return *this; }


const CStringW& CStringW::operator+=(char ch)
	{ *this += (WCHAR)ch; return *this; }

const CStringW& CStringW::operator=(char ch)
	{ *this = (WCHAR)ch; return *this; }

CStringW operator+(const CStringW& string, char ch)
	{ return string + (WCHAR)ch; }

CStringW operator+(char ch, const CStringW& string)
	{ return (WCHAR)ch + string; }

int CStringW::GetLength() const
	{ return GetData()->nDataLength; }

int CStringW::GetAllocLength() const
	{ return GetData()->nAllocLength; }

BOOL CStringW::IsEmpty() const
{ 
	return GetData()->nDataLength == 0;
}

CStringW::operator LPCWSTR() const
	{ return m_pchData; }

int PASCAL CStringW::SafeStrlen(LPCWSTR lpsz)
	{ return (lpsz == NULL) ? 0 : lstrlenW(lpsz); }

// CStringW support (windows specific)
int CStringW::Compare(LPCWSTR lpsz) const
	{ return wcscmp(m_pchData, lpsz); }    // MBCS/Unicode aware

int CStringW::CompareNoCase(LPCWSTR lpsz) const
	{ return wcsicmp(m_pchData, lpsz); }   // MBCS/Unicode aware

// CStringW::Collate is often slower than Compare but is MBSC/Unicode
//  aware as well as locale-sensitive with respect to sort order.
int CStringW::Collate(LPCWSTR lpsz) const
	{ return wcscoll(m_pchData, lpsz); }   // locale sensitive

int CStringW::CollateNoCase(LPCWSTR lpsz) const
	{ return wcsicoll(m_pchData, lpsz); }   // locale sensitive

WCHAR CStringW::GetAt(int nIndex) const
{
	ASSERT(nIndex >= 0);
	ASSERT(nIndex < GetData()->nDataLength);
	return m_pchData[nIndex];
}

WCHAR CStringW::operator[](int nIndex) const
{
	// same as GetAt
	ASSERT(nIndex >= 0);
	ASSERT(nIndex <= GetData()->nDataLength); // allow because it's Terminated char
	return m_pchData[nIndex];
}

bool operator==(const CStringW& s1, const CStringW& s2)
{ 
	return s1.Compare(s2) == 0; 
}

bool operator==(const CStringW& s1, LPCWSTR s2)
{ 
	return s1.Compare(s2) == 0; 
}

bool operator==(LPCWSTR s1, const CStringW& s2)
{ 
	return s2.Compare(s1) == 0; 
}

bool operator!=(const CStringW& s1, const CStringW& s2)
{ 
	return s1.Compare(s2) != 0; 
}

bool operator!=(const CStringW& s1, LPCWSTR s2)
{ 
	return s1.Compare(s2) != 0; 
}

bool operator!=(LPCWSTR s1, const CStringW& s2)
{ 
	return s2.Compare(s1) != 0; 
}

bool operator<(const CStringW& s1, const CStringW& s2)
{ 
	return s1.Compare(s2) < 0; 
}

bool operator<(const CStringW& s1, LPCWSTR s2)
{ 
	return s1.Compare(s2) < 0; 
}

bool operator<(LPCWSTR s1, const CStringW& s2)
{ 
	return s2.Compare(s1) > 0; 
}

bool operator>(const CStringW& s1, const CStringW& s2)
{ 
	return s1.Compare(s2) > 0; 
}

bool operator>(const CStringW& s1, LPCWSTR s2)
{ 
	return s1.Compare(s2) > 0; 
}

bool operator>(LPCWSTR s1, const CStringW& s2)
{ 
	return s2.Compare(s1) < 0; 
}

bool operator<=(const CStringW& s1, const CStringW& s2)
{ 
	return s1.Compare(s2) <= 0; 
}

bool operator<=(const CStringW& s1, LPCWSTR s2)
{ 
	return s1.Compare(s2) <= 0; 
}

bool operator<=(LPCWSTR s1, const CStringW& s2)
{ 
	return s2.Compare(s1) >= 0; 
}

bool operator>=(const CStringW& s1, const CStringW& s2)
{ 
	return s1.Compare(s2) >= 0; 
}

bool operator>=(const CStringW& s1, LPCWSTR s2)
{ 
	return s1.Compare(s2) >= 0; 
}

bool operator>=(LPCWSTR s1, const CStringW& s2)
{ 
	return s2.Compare(s1) <= 0; 
}


//////////////////////////////////////////////////////////////////////////////
// More sophisticated construction

CStringW::CStringW(WCHAR ch, int nLength)
{
	Init();
	if (nLength >= 1)
	{
		AllocBuffer(nLength);

		for (int i = 0; i < nLength; i++)
			m_pchData[i] = ch;
	}
}

CStringW::CStringW(LPCWSTR lpch, int nLength)
{
	Init();
	if (nLength != 0)
	{
		AllocBuffer(nLength);
		memcpy(m_pchData, lpch, nLength*sizeof(WCHAR));
	}
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion constructors

CStringW::CStringW(LPCSTR lpsz, int nLength)
{
	Init();
	if (nLength != 0)
	{
		AllocBuffer(nLength);
		int n = ::MultiByteToWideChar(CP_ACP, 0, lpsz, nLength, m_pchData, nLength+1);
		ReleaseBuffer(n >= 0 ? n : -1);
	}
}

//////////////////////////////////////////////////////////////////////////////
// Assignment operators

const CStringW& CStringW::operator=(WCHAR ch)
{
	AssignCopy(1, &ch);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////
// less common string expressions

CStringW operator+(const CStringW& string1, WCHAR ch)
{
	CStringW s;
	s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData, 1, &ch);
	return s;
}

CStringW operator+(WCHAR ch, const CStringW& string)
{
	CStringW s;
	s.ConcatCopy(1, &ch, string.GetData()->nDataLength, string.m_pchData);
	return s;
}

//////////////////////////////////////////////////////////////////////////////
// Advanced manipulation

int CStringW::Delete(int nIndex, int nCount /* = 1 */)
{
	if (nIndex < 0)
		nIndex = 0;
	int nNewLength = GetData()->nDataLength;
	if (nCount > 0 && nIndex < nNewLength)
	{
		CopyBeforeWrite();
		int nBytesToCopy = nNewLength - (nIndex + nCount) + 1;

		memcpy(m_pchData + nIndex,
			m_pchData + nIndex + nCount, nBytesToCopy * sizeof(WCHAR));
		GetData()->nDataLength = nNewLength - nCount;
	}

	return nNewLength;
}

int CStringW::Insert(int nIndex, WCHAR ch)
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
		CStringDataW* pOldData = GetData();
		LPWSTR pstr = m_pchData;
		AllocBuffer(nNewLength);
		memcpy(m_pchData, pstr, (pOldData->nDataLength+1)*sizeof(WCHAR));
		CStringW::Release(pOldData);
	}

	// move existing bytes down
	memcpy(m_pchData + nIndex + 1,
		m_pchData + nIndex, (nNewLength-nIndex)*sizeof(WCHAR));
	m_pchData[nIndex] = ch;
	GetData()->nDataLength = nNewLength;

	return nNewLength;
}

int CStringW::Insert(int nIndex, LPCWSTR pstr)
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
			CStringDataW* pOldData = GetData();
			LPWSTR pstr = m_pchData;
			AllocBuffer(nNewLength);
			memcpy(m_pchData, pstr, (pOldData->nDataLength+1)*sizeof(WCHAR));
			CStringW::Release(pOldData);
		}

		// move existing bytes down
		memcpy(m_pchData + nIndex + nInsertLength,
			m_pchData + nIndex,
			(nNewLength-nIndex-nInsertLength+1)*sizeof(WCHAR));
		memcpy(m_pchData + nIndex,
			pstr, nInsertLength*sizeof(WCHAR));
		GetData()->nDataLength = nNewLength;
	}

	return nNewLength;
}

int CStringW::Replace(WCHAR chOld, WCHAR chNew)
{
	int nCount = 0;

	// short-circuit the nop case
	if (chOld != chNew)
	{
		// otherwise modify each character that matches in the string
		CopyBeforeWrite();
		LPWSTR psz = m_pchData;
		LPWSTR pszEnd = psz + GetData()->nDataLength;
		while (psz < pszEnd)
		{
			// replace instances of the specified character only
			if (*psz == chOld)
			{
				*psz = chNew;
				nCount++;
			}
			psz++; //psz = _wcsinc(psz);
		}
	}
	return nCount;
}

int CStringW::Replace(LPCWSTR lpszOld, LPCWSTR lpszNew)
{
	// can't have empty or NULL lpszOld

	int nSourceLen = SafeStrlen(lpszOld);
	if (nSourceLen == 0)
		return 0;
	int nReplacementLen = SafeStrlen(lpszNew);

	// loop once to figure out the size of the result string
	int nCount = 0;
	LPWSTR lpszStart = m_pchData;
	LPWSTR lpszEnd = m_pchData + GetData()->nDataLength;
	LPWSTR lpszTarget;
	while (lpszStart < lpszEnd)
	{
		while ((lpszTarget = wcsstr(lpszStart, lpszOld)) != NULL)
		{
			nCount++;
			lpszStart = lpszTarget + nSourceLen;
		}
		lpszStart += lstrlenW(lpszStart) + 1;
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
			CStringDataW* pOldData = GetData();
			LPWSTR pstr = m_pchData;
			AllocBuffer(nNewLength);
			memcpy(m_pchData, pstr, pOldData->nDataLength*sizeof(WCHAR));
			CStringW::Release(pOldData);
		}
		// else, we just do it in-place
		lpszStart = m_pchData;
		lpszEnd = m_pchData + GetData()->nDataLength;

		// loop again to actually do the work
		while (lpszStart < lpszEnd)
		{
			while ( (lpszTarget = wcsstr(lpszStart, lpszOld)) != NULL)
			{
				int nBalance = nOldLength - (lpszTarget - m_pchData + nSourceLen);
				memmove(lpszTarget + nReplacementLen, lpszTarget + nSourceLen,
					nBalance * sizeof(WCHAR));
				memcpy(lpszTarget, lpszNew, nReplacementLen*sizeof(WCHAR));
				lpszStart = lpszTarget + nReplacementLen;
				lpszStart[nBalance] = '\0';
				nOldLength += (nReplacementLen - nSourceLen);
			}
			lpszStart += lstrlenW(lpszStart) + 1;
		}
		ASSERT(m_pchData[nNewLength] == '\0');
		GetData()->nDataLength = nNewLength;
	}

	return nCount;
}

/*
int CStringW::Remove(WCHAR chRemove)
{
	CopyBeforeWrite();

	LPWSTR pstrSource = m_pchData;
	LPWSTR pstrDest = m_pchData;
	LPWSTR pstrEnd = m_pchData + GetData()->nDataLength;

	while (pstrSource < pstrEnd)
	{
		if (*pstrSource != chRemove)
		{
			*pstrDest = *pstrSource;
			pstrDest = _tcsinc(pstrDest);
		}
		pstrSource = _tcsinc(pstrSource);
	}
	*pstrDest = '\0';
	int nCount = pstrSource - pstrDest;
	GetData()->nDataLength -= nCount;

	return nCount;
}
*/

//////////////////////////////////////////////////////////////////////////////
// Very simple sub-string extraction

CStringW CStringW::Mid(int nFirst) const
{
	return Mid(nFirst, GetData()->nDataLength - nFirst);
}

CStringW CStringW::Mid(int nFirst, int nCount) const
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

	CStringW dest;
	AllocCopy(dest, nCount, nFirst, 0);
	return dest;
}

/*
CStringW CStringW::Right(int nCount) const
{
	if (nCount < 0)
		nCount = 0;
	if (nCount >= GetData()->nDataLength)
		return *this;

	CStringW dest;
	AllocCopy(dest, nCount, GetData()->nDataLength-nCount, 0);
	return dest;
}
*/

CStringW CStringW::Left(int nCount) const
{
	if (nCount < 0)
		nCount = 0;
	if (nCount >= GetData()->nDataLength)
		return *this;

	CStringW dest;
	AllocCopy(dest, nCount, 0, 0);
	return dest;
}

/*
// strspn equivalent
CStringW CStringW::SpanIncluding(LPCWSTR lpszCharSet) const
{
	return Left(_tcsspn(m_pchData, lpszCharSet));
}

// strcspn equivalent
CStringW CStringW::SpanExcluding(LPCWSTR lpszCharSet) const
{
	return Left(_tcscspn(m_pchData, lpszCharSet));
}
*/

//////////////////////////////////////////////////////////////////////////////
// Finding

int CStringW::ReverseFind(WCHAR ch) const
{
	// find last single character
	LPWSTR lpsz = wcsrchr(m_pchData, (WCHAR) ch);

	// return -1 if not found, distance from beginning otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

// find a sub-string (like strstr)
int CStringW::Find(LPCWSTR lpszSub) const
{
	return Find(lpszSub, 0);
}

int CStringW::Find(LPCWSTR lpszSub, int nStart) const
{
	int nLength = GetData()->nDataLength;
	if (nStart > nLength)
		return -1;

	// find first matching substring
	LPWSTR lpsz = wcsstr(m_pchData + nStart, lpszSub);

	// return -1 for not found, distance from beginning otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}


/////////////////////////////////////////////////////////////////////////////
// CStringW formatting

#define TCHAR_ARG   WCHAR
#define WCHAR_ARG   WCHAR
#define CHAR_ARG    char


#define FORCE_ANSI      0x10000
#define FORCE_UNICODE   0x20000
#define FORCE_INT64     0x40000
#define _wcsinc(_pc)    ((_pc)+1)


void CStringW::FormatV(LPCWSTR lpszFormat, va_list argList)
{
	va_list argListSave = argList;

	// make a guess at the maximum length of the resulting string
	int nMaxLen = 0;
	for (LPCWSTR lpsz = lpszFormat; *lpsz != '\0'; lpsz = _wcsinc(lpsz))
	{
		// handle '%' character, but watch out for '%%'
		if (*lpsz != '%' || *(lpsz = _wcsinc(lpsz)) == '%')
		{
			nMaxLen += lstrlenW(lpsz);
			continue;
		}

		int nItemLen = 0;

		// handle '%' character with format
		int nWidth = 0;
		for (; *lpsz != '\0'; lpsz = _wcsinc(lpsz))
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
			nWidth = _wtoi(lpsz);
			for (; *lpsz != '\0' && iswdigit(*lpsz); lpsz = _wcsinc(lpsz))
				;
		}
		ASSERT(nWidth >= 0);

		int nPrecision = 0;
		if (*lpsz == '.')
		{
			// skip past '.' separator (width.precision)
			lpsz = _wcsinc(lpsz);

			// get precision and skip it
			if (*lpsz == '*')
			{
				nPrecision = va_arg(argList, int);
				lpsz = _wcsinc(lpsz);
			}
			else
			{
				nPrecision = _wtoi(lpsz);
				for (; *lpsz != '\0' && iswdigit(*lpsz); lpsz = _wcsinc(lpsz))
					;
			}
			ASSERT(nPrecision >= 0);
		}

		// should be on type modifier or specifier
		int nModifier = 0;
		if (wcsncmp(lpsz, L"I64", 3) == 0)
		{
			lpsz += 3;
			nModifier = FORCE_INT64;
#if !defined(_X86_) && !defined(_ALPHA_)
			// __int64 is only available on X86 and ALPHA platforms
			ASSERT(FALSE);
#endif
		}
		else
		{
			switch (*lpsz)
			{
			// modifiers that affect size
			case 'h':
				nModifier = FORCE_ANSI;
				lpsz = _wcsinc(lpsz);
				break;
			case 'l':
				nModifier = FORCE_UNICODE;
				lpsz = _wcsinc(lpsz);
				break;

			// modifiers that do not affect size
			case 'F':
			case 'N':
			case 'L':
				lpsz = _wcsinc(lpsz);
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
			va_arg(argList, TCHAR_ARG);
			break;
		case 'c'|FORCE_ANSI:
		case 'C'|FORCE_ANSI:
			nItemLen = 2;
			va_arg(argList, CHAR_ARG);
			break;
		case 'c'|FORCE_UNICODE:
		case 'C'|FORCE_UNICODE:
			nItemLen = 2;
			va_arg(argList, WCHAR_ARG);
			break;

		// strings
		case 's':
			{
				LPCWSTR pstrNextArg = va_arg(argList, LPCWSTR);
				if (pstrNextArg == NULL)
				   nItemLen = 6;  // "(null)"
				else
				{
				   nItemLen = lstrlenW(pstrNextArg);
				   nItemLen = max(1, nItemLen);
				}
			}
			break;

		case 'S':
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
				   nItemLen = lstrlenW(pstrNextArg);
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
					LPWSTR pszTemp;

					// 312 == strlen("-1+(309 zeroes).")
					// 309 zeroes == max precision of a double
					// 6 == adjustment in case precision is not specified,
					//   which means that the precision defaults to 6
					pszTemp = (LPWSTR)_alloca(max(nWidth, 312+nPrecision+6));

					f = va_arg(argList, double);
					swprintf( pszTemp, L"%*.*f", nWidth, nPrecision+6, f );
					nItemLen = lstrlenW(pszTemp);
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
	VERIFY(vswprintf(m_pchData, lpszFormat, argListSave) <= GetAllocLength());
	ReleaseBuffer();

	va_end(argListSave);
}


// formatting (using wsprintf style formatting)
void CStringW::Format(LPCWSTR lpszFormat, ...)
{
	va_list argList;
	va_start(argList, lpszFormat);
	FormatV(lpszFormat, argList);
	va_end(argList);
}

/*
void CStringW::Format(UINT nFormatID, ...)
{
	CStringW strFormat;
	VERIFY(strFormat.LoadString(nFormatID) != 0);

	va_list argList;
	va_start(argList, nFormatID);
	FormatV(strFormat, argList);
	va_end(argList);
}


// formatting (using FormatMessage style formatting)
void CStringW::FormatMessage(LPCWSTR lpszFormat, ...)
{
	// format message into temporary buffer lpszTemp
	va_list argList;
	va_start(argList, lpszFormat);
	LPWSTR lpszTemp;

	if (::FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
		lpszFormat, 0, 0, (LPWSTR)&lpszTemp, 0, &argList) == 0 ||
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
void CStringW::FormatMessage(UINT nFormatID, ...)
{
	// get format string from string table
	CStringW strFormat;
	VERIFY(strFormat.LoadString(nFormatID) != 0);

	// format message into temporary buffer lpszTemp
	va_list argList;
	va_start(argList, nFormatID);
	LPWSTR lpszTemp;
	if (::FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
		strFormat, 0, 0, (LPWSTR)&lpszTemp, 0, &argList) == 0 ||
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
void CStringW::TrimRight(LPCWSTR lpszTargetList)
{
	// find beginning of trailing matches
	// by starting at beginning (DBCS aware)

	CopyBeforeWrite();
	LPWSTR lpsz = m_pchData;
	LPWSTR lpszLast = NULL;

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

void CStringW::TrimRight(WCHAR chTarget)
{
	// find beginning of trailing matches
	// by starting at beginning (DBCS aware)

	CopyBeforeWrite();
	LPWSTR lpsz = m_pchData;
	LPWSTR lpszLast = NULL;

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

void CStringW::TrimRight()
{
	// find beginning of trailing spaces by starting at beginning (DBCS aware)

	CopyBeforeWrite();
	LPWSTR lpsz = m_pchData;
	LPWSTR lpszLast = NULL;

	while (*lpsz != '\0')
	{
		if (iswspace(*lpsz))
		{
			if (lpszLast == NULL)
				lpszLast = lpsz;
		}
		else
			lpszLast = NULL;
		lpsz = _wcsinc(lpsz);
	}

	if (lpszLast != NULL)
	{
		// truncate at trailing space start
		*lpszLast = '\0';
		GetData()->nDataLength = lpszLast - m_pchData;
	}
}

/*
void CStringW::TrimLeft(LPCWSTR lpszTargets)
{
	// if we're not trimming anything, we're not doing any work
	if (SafeStrlen(lpszTargets) == 0)
		return;

	CopyBeforeWrite();
	LPCWSTR lpsz = m_pchData;

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
		memmove(m_pchData, lpsz, (nDataLength+1)*sizeof(WCHAR));
		GetData()->nDataLength = nDataLength;
	}
}

void CStringW::TrimLeft(WCHAR chTarget)
{
	// find first non-matching character

	CopyBeforeWrite();
	LPCWSTR lpsz = m_pchData;

	while (chTarget == *lpsz)
		lpsz = _tcsinc(lpsz);

	if (lpsz != m_pchData)
	{
		// fix up data and length
		int nDataLength = GetData()->nDataLength - (lpsz - m_pchData);
		memmove(m_pchData, lpsz, (nDataLength+1)*sizeof(WCHAR));
		GetData()->nDataLength = nDataLength;
	}
}
*/


void CStringW::TrimLeft()
{
	// find first non-space character

	CopyBeforeWrite();
	LPCWSTR lpsz = m_pchData;

	while (iswspace(*lpsz)) lpsz = _wcsinc(lpsz);

	if (lpsz != m_pchData)
	{
		// fix up data and length
		int nDataLength = GetData()->nDataLength - (lpsz - m_pchData);
		memmove(m_pchData, lpsz, (nDataLength+1)*sizeof(WCHAR));
		GetData()->nDataLength = nDataLength;
	}
}

/*

int AfxLoadString(UINT nID, LPWSTR lpszBuf, UINT nMaxBuf)
{	
#ifdef _DEBUG
	// LoadString without annoying warning from the Debug kernel if the
	//  segment containing the string is not present
	if (::FindResource(NULL, MAKEINTRESOURCE((nID>>4)+1), RT_STRING) == NULL)
	{
		lpszBuf[0] = '\0';
		return 0; // not found
	}
#endif //_DEBUG
	int nLen = ::LoadString(NULL, nID, lpszBuf, nMaxBuf);
	if (nLen == 0)
		lpszBuf[0] = '\0';
	return nLen;
}


BOOL CStringW::LoadString(UINT nID)
{
	// try fixed buffer first (to avoid wasting space in the heap)
	WCHAR szTemp[256];
	int nLen = AfxLoadString(nID, szTemp, 256);
	if (256 - nLen > 1)
	{
		*this = szTemp;
		return nLen > 0;
	}

	// try buffer size of 512, then larger size until entire string is retrieved
	int nSize = 256;
	do
	{
		nSize += 256;
		nLen = AfxLoadString(nID, GetBuffer(nSize-1), nSize);
	} while (nSize - nLen <= 1);
	ReleaseBuffer();

	return nLen > 0;
}
*/