#include "stdafx.h"
#include "RLFile.h"
#include "RLException.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

RLFile::RLFile()
{
	m_handle = NULL;
}

RLFile::~RLFile()
{
	// desctuctor shouldn't throw any exeption, because it can be called in exception catcher
	try {
		Close();
	}
	catch(RLException& ) {}
}

void RLFile::Create(LPCSTR fileName)
{
	m_handle = ::fopen(fileName, "wb");

	if (m_handle==NULL) throw RLException("RLFile::Create('%s') error=%d", fileName, errno);
}

void RLFile::Open(LPCSTR fileName)
{
	m_handle = ::fopen(fileName, "rb");

	if (m_handle==NULL) throw RLException("RLFile::Open('%s') error=%d", fileName, errno);
}

void RLFile::OpenOrCreate(LPCSTR fileName)
{
	m_handle = ::fopen(fileName, "ab");

	if (m_handle==NULL) throw RLException("RLFile::Open('%s') error=%d", fileName, errno);
}


void RLFile::Read(LPVOID buffer, UINT len)
{
	size_t dwRead = ::fread(buffer, 1, len, m_handle);
	if (dwRead != len) // && !feof(m_handle) )
		throw RLException("RLFile::Read() %d %d error=%d", dwRead, len ,errno);
}

void RLFile::Write(LPCVOID buffer, UINT len)
{
	DWORD dwWritten = ::fwrite(buffer, 1, len, m_handle);
	//	TODO: feof - do we need it ???
	if (dwWritten != len) // && !feof(m_handle) )
		throw RLException("RLFile::Write() %d %d error=%d", dwWritten, len, errno);
}

void RLFile::Close()
{
	if (m_handle!=NULL) {
		if (::fclose(m_handle) == EOF)
			throw RLException("RLFile::Close() error=%d", errno);
		m_handle = NULL;
	}
}

UINT32 RLFile::GetSize()
{
	::fseek(m_handle, 0L, SEEK_END);
	long dwSize = ::ftell(m_handle);
	if ( dwSize == -1 )
		throw RLException("RLFile::GetSize() error=%d", errno);

	return dwSize;
}

void RLFile::SetSeek(UINT32 offset)
{
	::fseek(m_handle, offset, SEEK_SET);
}


//______________________________________________________________________________________________________

#ifdef _WIN32

RLHandle::RLHandle(HANDLE handle)
{
	m_handle = handle;
}

RLHandle::~RLHandle()
{
	// desctuctor shouldn't throw any exeption, because it can be called in exception catcher
	try {
		Close();
	}
	catch(RLException& ) {}
}

void RLHandle::Close()
{
	if (m_handle!=NULL) {
		if (::CloseHandle(m_handle)==FALSE)
			throw RLException("RLHandle::Close() error=%d", ::GetLastError());
		m_handle = NULL;
	}
}


//______________________________________________________________________________________________________

RLFileWin::RLFileWin()
{
	m_handle = NULL;
}


void RLFileWin::Attach(HANDLE handle)
{
	this->Close();
	m_handle = handle;
}

UINT32 RLFileWin::GetSize()
{
	DWORD dwSize = ::GetFileSize(m_handle, NULL);
	if (dwSize==INVALID_FILE_SIZE)
		throw RLException("RLFileWin::GetSize() error=%d", ::GetLastError());
	
	return dwSize;
}


void RLFileWin::Read(LPVOID buffer, UINT len)
{
	DWORD dwRead;
	BOOL bResult = ::ReadFile(m_handle, buffer, len, &dwRead, NULL);
	if (bResult==FALSE || dwRead!=len)
		throw RLException("RLFileWin::Read() error=%d", ::GetLastError());	
}

void RLFileWin::Write(LPCVOID buffer, UINT len)
{
	DWORD dwWritten;
	BOOL bResult = ::WriteFile(m_handle, buffer, len, &dwWritten, NULL);
	if (bResult==FALSE || dwWritten!=len)
		throw RLException("RLFileWin::Write() error=%d", ::GetLastError());
}

void RLFileWin::GetLastWriteTime(HANDLE hFile, FILETIME* pTime)
{
	if (::GetFileTime(hFile, NULL, NULL, pTime)==0)
		pTime->dwHighDateTime = pTime->dwLowDateTime = 0xFFFFFFFF; // means error
}

void RLFileWin::SetLastWriteTime(HANDLE hFile, FILETIME* pTime)
{
	if (pTime->dwHighDateTime != 0xFFFFFFFF || pTime->dwLowDateTime != 0xFFFFFFFF)
		::SetFileTime(hFile, NULL, NULL, pTime);
}

#endif // _WIN32
