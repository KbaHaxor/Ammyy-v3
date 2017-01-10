#include "stdafx.h"
#include "Downloader.h"
#include "../RL/RLHttp.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

CDownloader::CDownloader()
{
	m_hThread = NULL;
	m_speed = 0;
	m_done  = 0;
}

CDownloader::~CDownloader()
{
	Stop();
}

void CDownloader::Start()
{
	if (m_hThread!=NULL) return;	// if working now

	m_stopping = false;
	m_error = "";
	DWORD dwThreaId;
	m_hThread = ::CreateThread(NULL, 0, StartThreadProc, this, 0, &dwThreaId);
}

void CDownloader::Stop()
{
	if (m_hThread==NULL) return;	// if not working now

	m_stopping = true;
	VERIFY(::WaitForSingleObject(m_hThread, INFINITE) == WAIT_OBJECT_0);
	ASSERT(m_hThread==NULL);
}


DWORD WINAPI CDownloader::StartThreadProc(LPVOID lpParameter)
{
	CDownloader* _this = (CDownloader*)lpParameter;
	try {
		_this->StartInternal();
	}
	catch(RLException& ex) {
		_this->m_error = ex.GetDescription();
	}
	::CloseHandle(_this->m_hThread);
	_this->m_hThread = NULL;
	return 0;
}

void CDownloader::StartInternal()
{
	CStringW tmpFileName = m_fileName+".tmp";
	HANDLE hFile = ::CreateFileW(tmpFileName, FILE_WRITE_DATA, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile==INVALID_HANDLE_VALUE) {
		throw RLException("Can't create file '%s' error %d", (LPCSTR)(CStringA)tmpFileName, ::GetLastError());
	}

	// for continue downloading need to change flag to OPEN_ALWAYS
	m_dwFilePosBegin = m_dwFilePosCurr = ::GetFileSize(hFile, NULL);
	ASSERT(m_dwFilePosBegin!=INVALID_FILE_SIZE);

	VERIFY(::SetFilePointer(hFile, 0, NULL, FILE_END) == m_dwFilePosBegin);

	RLHttp http;
	http.m_szHeader.Format("Content-Type: application/x-www-form-urlencoded\r\nRange: bytes=%d-\r\nAccept-Encoding:gzip, deflate", m_dwFilePosBegin);
	http.Get(m_url);

	if (!http.IsConnected()) {
		::CloseHandle(hFile);
		throw RLException("Can't connect to server");
	}
	DWORD dwStatusCode = http.GetStatusCode();

	if (dwStatusCode<200 || dwStatusCode>=300) {
		::CloseHandle(hFile);
		throw RLException("Incorrect status code %d from '%s'", dwStatusCode, m_url);
	}

	DWORD dwContentSize = http.GetContentLength();

	m_dwFilePosEnd = m_dwFilePosBegin + dwContentSize;
	m_dwTickCountBegin = ::GetTickCount();

	UpdateStatus();

	const DWORD dwBufferSize = 16*1024;
	RLStream stream;
	stream.SetMinCapasity(dwBufferSize);

	LPVOID pBuffer = (char*)stream.GetBuffer();

	while(!m_stopping) {
		DWORD dwNumberOfBytesRead;
		bool b = http.ReadDataOneTime(pBuffer, dwBufferSize, &dwNumberOfBytesRead);

		if (!b) {
			::CloseHandle(hFile);
			throw RLException("Error of getting file '%s'", m_url);
		}

		if (dwNumberOfBytesRead==0) break;

		//save to file
		DWORD dwNumberOfBytesWritten;
		VERIFY(::WriteFile(hFile, pBuffer, dwNumberOfBytesRead, &dwNumberOfBytesWritten, NULL)!=FALSE);
		ASSERT(dwNumberOfBytesWritten==dwNumberOfBytesRead);

		m_dwFilePosCurr += dwNumberOfBytesRead;

		UpdateStatus();

		if (m_dwFilePosCurr>=m_dwFilePosEnd) break;
	}

	::CloseHandle(hFile);


	// if ok, rename the file
	if (m_dwFilePosCurr==m_dwFilePosEnd) {
		VERIFY(::MoveFileW(tmpFileName, m_fileName) != FALSE);
	}
}


void CDownloader::UpdateStatus()
{
	DWORD dwTicksElapsed = ::GetTickCount() - m_dwTickCountBegin;	
	m_done	= (m_dwFilePosEnd==0) ? 0 :  (double)m_dwFilePosCurr/m_dwFilePosEnd;
	m_speed = (dwTicksElapsed==0) ? 0 : (((double)(m_dwFilePosCurr-m_dwFilePosBegin)) / dwTicksElapsed)*1000;
}

bool CDownloader::IsWorking()
{ 
	if (m_error.GetLength()>0) throw RLException(m_error);

	return m_hThread!=NULL; 
}
