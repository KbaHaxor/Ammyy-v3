#include "stdafx.h"
#include "RLStream.h"

#ifdef _DEBUG
//#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#include "RLFile.h"

RLStream::RLStream()
{	
	m_dwCapacity = 0;
	m_posWrite   = 0;
	m_posRead	 = 0;
	m_pData		 = NULL;
	m_needToFree = false;
}


RLStream::RLStream(DWORD dwCapasity)
{	
	m_dwCapacity = 0;
	m_posWrite   = 0;
	m_posRead	 = 0;
	m_pData		 = NULL;
	m_needToFree = false;

	SetMinCapasity(dwCapasity);
}


RLStream::RLStream(void* buffer, DWORD dwCapasity)
{
	m_dwCapacity = dwCapasity;
	m_posWrite   = 0;
	m_posRead	 = 0;
	m_pData		 = (char*)buffer;
	m_needToFree = false;
}

const RLStream& RLStream::operator=(const RLStream& src)
{
	if (&src != this) {
		Free();
		SetMinCapasity(src.m_dwCapacity);
		m_posWrite = src.m_posWrite;
		m_posRead  = src.m_posRead;

		memcpy(m_pData, src.m_pData, m_posWrite);
	}

	return *this;
}


RLStream::~RLStream()
{
	if (m_needToFree && m_pData != NULL) ::free(m_pData);
}

void RLStream::Reset()
{
	m_posWrite = 0;
	m_posRead  = 0;
}

void RLStream::Free()
{
	if (m_needToFree && m_pData != NULL) ::free(m_pData);
	
	m_dwCapacity = 0;
	m_posWrite   = 0;
	m_posRead	 = 0;
	m_pData		 = NULL;
}

void RLStream::SetMinCapasity(DWORD dwMinCapasity)
{
	if (m_dwCapacity >= dwMinCapasity)
		return;

	m_dwCapacity = (m_dwCapacity==0) ? 32 : m_dwCapacity*2;

	if (m_dwCapacity < dwMinCapasity) m_dwCapacity = dwMinCapasity;

	void* pOldData = m_pData;
	m_pData = (char*)::malloc(m_dwCapacity);
	if (m_pData==NULL)
		throw RLException("Memory allocation %u bytes failed", m_dwCapacity);

	if (pOldData!=NULL) {
		if (m_posWrite>0) memcpy(m_pData, pOldData, m_posWrite);
		if (m_needToFree) ::free(pOldData);
	}

	m_needToFree = true;
}


void RLStream::AddRaw(const void* pData, DWORD dwLen)
{
	SetMinCapasity(m_posWrite+dwLen);
	if (pData) memcpy(m_pData+m_posWrite, pData, dwLen);
	m_posWrite+=dwLen;
}

void RLStream::AddUINT8(UINT8 data)   { AddRaw(&data, 1); }
void RLStream::AddUINT16(UINT16 data) { AddRaw(&data, 2); }
void RLStream::AddUINT24(UINT32 data) { AddRaw(&data, 3); }
void RLStream::AddUINT32(UINT32 data) { AddRaw(&data, 4); }
void RLStream::AddUINT64(UINT64 data) { AddRaw(&data, 8); }


/*
void RLStream::AddDateTime(SYSTEMTIME t)
{
	this->AddUINT16(t.wYear);
	this->AddUINT8 ((UINT8)t.wMonth);
	this->AddUINT8 ((UINT8)t.wDay);
	this->AddUINT8 ((UINT8)t.wHour);
	this->AddUINT8 ((UINT8)t.wMinute);
	this->AddUINT8 ((UINT8)t.wSecond);
	this->AddUINT16(t.wMilliseconds);
}
*/

void RLStream::AddBool(bool b)
{
	char c = (b) ? 1 : 0;
	AddRaw(&c, 1);
}

void RLStream::AddStream(const RLStream *pStream, bool len)
{
	DWORD length = pStream->GetLen();
	if (len) AddUINT32(length);
	AddRaw(pStream->GetBuffer(), length);
}



// AddStringX
// X=0, data
// X=1, data+0
// X=2, len+data
//

#ifdef __CSTRING_A_H__INCLUDED__

void RLStream::AddString0A(const CStringA& str)
{
	DWORD strLen = str.GetLength();
	AddRaw((LPCSTR)str, strLen);
}

void RLStream::AddString1A(const CStringA& str)
{
	DWORD strLen = str.GetLength()+1;
	AddRaw((LPCSTR)str, strLen);
}

void RLStream::AddString2A(const CStringA& str)
{
	DWORD strLen = str.GetLength();
	AddUINT32(strLen);
	AddRaw((LPCSTR)str, strLen);
}

#endif

#ifdef __CSTRING_W_H__INCLUDED__

void RLStream::AddString0W(const CStringW& str)
{
	DWORD strLen = str.GetLength();
	AddRaw((LPCWSTR)str, strLen*2);
}

void RLStream::AddString1W(const CStringW& str)
{
	DWORD strLen = str.GetLength()+1;
	AddRaw((LPCWSTR)str, strLen*sizeof(WCHAR));
}

#endif

// __________________________________________________________________________________________

void RLStream::GetRaw(void* pData, DWORD dwLen)
{
	if (m_posWrite < m_posRead + dwLen)
		throw RLStreamOutException("RLStream::GetRaw out of data %u %u %u", m_posWrite,  m_posRead, dwLen);

	if (pData) memcpy(pData, &m_pData[m_posRead], dwLen);
	m_posRead+=dwLen;
}

UINT8  RLStream::GetUINT8()  { UINT8  d;   GetRaw(&d, sizeof(d)); return d; }
UINT16 RLStream::GetUINT16() { UINT16 d;   GetRaw(&d, sizeof(d)); return d; }
UINT32 RLStream::GetUINT24() { UINT32 d=0; GetRaw(&d, 3); return d; }
UINT32 RLStream::GetUINT32() { UINT32 d;   GetRaw(&d, sizeof(d)); return d; }
UINT64 RLStream::GetUINT64() { UINT64 d;   GetRaw(&d, sizeof(d)); return d; }

/*
void RLStream::GetDateTime(SYSTEMTIME& t)
{
	t.wYear  = this->GetUINT16();
	t.wMonth = this->GetUINT8 ();
	t.wDay = this->GetUINT8 ();
	t.wHour = this->GetUINT8 ();
	t.wMinute = this->GetUINT8 ();
	t.wSecond = this->GetUINT8 ();
	t.wMilliseconds = this->GetUINT16();
}
*/


bool  RLStream::GetBool()
{
	char c;
	GetRaw(&c, 1);
	return (c!=0);
}



// GetStringX
// X=0, data
// X=1, data+0
// X=2, len+data
//

#ifdef __CSTRING_A_H__INCLUDED__

CStringA RLStream::GetString0A()
{
	DWORD strLen = m_posWrite-m_posRead;
	CStringA str((LPCSTR)GetBufferRd(), strLen);
	m_posRead = m_posWrite;
	return str;
}

CStringA RLStream::GetString2A()
{
	DWORD len = GetUINT32();
	if (len>GetAvailForReading())
			throw RLStreamOutException("RLStream::GetString2A out of data %u %u", m_posWrite,  m_posRead);
	CStringA str;
	GetRaw(str.GetBuffer(len), len);
	str.ReleaseBuffer(len);
	return str;
}

CStringA RLStream::GetString1A()
{		
	LPCSTR pBufferBeg = m_pData + m_posRead;
	LPCSTR pBufferEnd = m_pData + m_posWrite;
	LPCSTR pBuffer    = pBufferBeg;

	while(true) {
		if (pBuffer >= pBufferEnd)
			throw RLStreamOutException("RLStream::GetString1A out of data %u %u", m_posWrite,  m_posRead);
	
		if (*pBuffer++ == 0) break;
	}

	DWORD strLen = pBuffer - pBufferBeg;
	m_posRead += strLen;
	return CStringA(pBufferBeg, strLen-1);
}

#endif

#ifdef __CSTRING_W_H__INCLUDED__

CStringW RLStream::GetString0W()
{
	DWORD strLen = m_posWrite-m_posRead;
	CStringW str((LPCWSTR)GetBufferRd(), strLen/2);
	m_posRead = m_posWrite;
	return str;
}

CStringW RLStream::GetString1W()
{		
	LPCWSTR pBufferBeg = (LPWSTR)(m_pData + m_posRead);
	LPCWSTR pBufferEnd = (LPWSTR)(m_pData + m_posWrite);
	LPCWSTR pBuffer    = pBufferBeg;

	while(true) {
		if (pBuffer >= pBufferEnd)
			throw RLStreamOutException("RLStream::GetString1W out of data %u %u", m_posWrite,  m_posRead);
	
		if (*pBuffer++ == 0) break;
	}

	DWORD strLen = pBuffer - pBufferBeg;
	m_posRead += strLen*sizeof(WCHAR);
	return CStringW(pBufferBeg, strLen-1);
}
#endif


void RLStream::GetStream(RLStream& stream, bool len)
{
	DWORD length;

	if (len) {
		length = GetUINT32();

		if (length>GetAvailForReading())
			throw RLStreamOutException("RLStream::GetStream out of data %u %u", m_posWrite,  m_posRead);
	}
	else {
		length = GetAvailForReading();
	}

	stream.Reset();
	stream.SetMinCapasity(length);

	GetRaw(stream.GetBuffer(), length);
	stream.SetLen(length);
}


// __________________________________________________________________________________________


#ifdef _WIN32
void RLStream::ReadFromFileWin32(HANDLE hFile, LPCSTR fileName)
{
	if (hFile==INVALID_HANDLE_VALUE)
		throw RLException("RLStream::ReadFromFile() couldn't open file '%s' error=%d", fileName, ::GetLastError());

	try {
		RLFileWin file;
		file.Attach(hFile);
		UINT32 size = file.GetSize();
		SetMinCapasity(size);
		file.Read(m_pData, size);
		file.Close();
		SetLen(size);
	}
	catch(RLException& ex) {
		throw RLException("RLStream::ReadFromFile('%s') %s", fileName, (LPCSTR)ex.GetDescription());
	}
}

void RLStream::ReadFromFileW(LPCWSTR fileName)
{
	HANDLE hFile = ::CreateFileW(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	ReadFromFileWin32(hFile, (CStringA)fileName);
}

void RLStream::ReadFromFile (LPCSTR fileName)
{
	HANDLE hFile = ::CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	ReadFromFileWin32(hFile, fileName);
}


#else
void RLStream::ReadFromFile(LPCSTR fileName)
{
	try {
		RLFile file;
		file.Open(fileName);
		UINT32 size = file.GetSize();
		file.SetSeek(0); // set to begin
		SetMinCapasity(size);
		file.Read(m_pData, size);
		file.Close();
		SetLen(size);
	}
	catch(RLException& ex) {
		throw RLException("RLStream::ReadFromFile('%s') %s", fileName, (LPCSTR)ex.GetDescription());
	}
}
#endif // _WIN32

#ifdef _WIN32
void RLStream::WriteToFileWin32(HANDLE hFile, LPCSTR fileName)
{
	if (hFile==INVALID_HANDLE_VALUE)
		throw RLException("RLStream::WriteToFile() couldn't create file '%s' error=%d", fileName, ::GetLastError());

	try {
		RLFileWin file;
		file.Attach(hFile);
		file.Write(m_pData, m_posWrite);
		file.Close();
	}
	catch(RLException& ex) {
		throw RLException("RLStream::WriteToFile('%s') %s", fileName, (LPCSTR)ex.GetDescription());
	}

}

void RLStream::WriteToFile(LPCSTR fileName)
{
	HANDLE hFile = ::CreateFileA(fileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	WriteToFileWin32(hFile, fileName);
}

void RLStream::WriteToFileW(LPCWSTR fileName)
{
	HANDLE hFile = ::CreateFileW(fileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	WriteToFileWin32(hFile, (CStringA)fileName);
}

#else
void RLStream::WriteToFile(LPCSTR fileName)
{
	try {
		RLFile file;
		file.Create(fileName);	
		file.Write(m_pData, m_posWrite);
		file.Close();
	}
	catch(RLException& ex) {
		throw RLException("RLStream::WriteToFile('%s') %s", fileName, (LPCSTR)ex.GetDescription());
	}
}
#endif // _WIN32


void* RLStream::GetBuffer1(int len)
{ 
	m_posWrite = 0; // to avoid copying old data
	SetMinCapasity(len);
	return m_pData; 
}


void* RLStream::GetBuffer() const
{ 
	return m_pData; 
}

void RLStream::SetLen(DWORD dwLen)
{ 
	m_posWrite = dwLen; 
}


char* RLStream::GetBufferWr() const
{
	return m_pData + m_posWrite;
}


char* RLStream::GetBufferRd() const
{
	return m_pData + m_posRead;
}

void RLStream::CutTillRead()
{
	if (m_posRead==0) return; // no need to cut

	int size = m_posWrite - m_posRead;

	if (size>0) {
		memcpy(m_pData, m_pData + m_posRead, size);
		m_posWrite -= m_posRead;
		m_posRead = 0;
	}
}




