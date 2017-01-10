#include "stdafx.h"
#include "RLLog.h"
#include "RLTimer.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

RLTimer globalTime;


#ifdef _WIN32
	#define INVALID_FILE INVALID_HANDLE_VALUE
#else
	#define INVALID_FILE NULL

	inline DWORD GetCurrentThreadId()
	{
		return (DWORD)::pthread_self();
	}
#endif


RLLog::RLLog()
{
	m_hFile = INVALID_FILE;
	m_initialized = false;
	m_silent = false;
	m_exactTime = false;
	m_console = false;
}

RLLog::~RLLog() 
{
	CloseFile();
}

void RLLog::OnError(LPCSTR error)
{
#ifdef _WIN32
	if (!m_silent) 
		::MessageBoxA(NULL, error, "ERROR", MB_OK | MB_ICONWARNING);
#else
	printf("ERROR: %s\n", error);
#endif
}

void RLLog::OpenFile()
{
	if (m_hFile!=INVALID_FILE) return; // already opened

#ifdef _WIN32
	m_hFile = ::CreateFileW(m_fileName, FILE_WRITE_DATA, FILE_SHARE_READ+FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (m_hFile!=INVALID_FILE) {
		::SetFilePointer(m_hFile,0, NULL, FILE_END);
		return;
	}
#else
	m_hFile = ::fopen(m_fileName, "ab");

	if (m_hFile!=INVALID_FILE) {
		::fseek(m_hFile, 0, SEEK_END);
		return;
	}
#endif
	
	m_initialized = false; // error occur
	OnError("Can't create log file. Check the permissions.");
}

void RLLog::CloseFile()
{
	if (m_hFile!=INVALID_FILE) {
	#ifdef _WIN32
		::CloseHandle(m_hFile);
	#else
		::fclose(m_hFile);
	#endif
		m_hFile =INVALID_FILE;
	}
}


#ifdef _WIN32
LPCWSTR RLLog::GetFileName()
{
	return m_fileName;
}

void RLLog::InitW(const WCHAR* fileName)
{
	if (fileName!=NULL) wcsncpy(m_fileName, fileName, MAX_PATH);
	m_initialized = true;
}


void RLLog::InitA(const char* fileName)
{
	::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, fileName, -1, m_fileName, MAX_PATH);
	InitW(NULL);
}

void RLLog::Init(HMODULE hModule)
{
	::GetModuleFileNameW(hModule, m_fileName, MAX_PATH);
	
	WCHAR* p = wcsrchr(m_fileName, '.');
	if (p==NULL) {
		OnError("ERROR in initalize log file name");
		return;
	}
	wcscpy(p+1, L"log");
	InitW(NULL);
}
#else

LPCSTR RLLog::GetFileName()
{
	return m_fileName;
}

void RLLog::InitA(const char* fileName)
{
	if ( fileName ) strncpy(m_fileName, fileName, sizeof(m_fileName));
	m_initialized = true;
}

#endif // _WIN32


void RLLog::WriteRawData(const void* pData, DWORD len)
{
	RLMutexLock ml(m_csWrite);

	bool needToOpen = (m_hFile==INVALID_FILE);

	if (needToOpen) OpenFile();

	
	if (m_hFile!=INVALID_FILE) {
	#ifdef _WIN32
		DWORD dwNumberOfBytesWritten;
		::WriteFile(m_hFile, pData, len, &dwNumberOfBytesWritten, NULL);
	#else
		::fwrite(pData, len, 1, m_hFile);
		::fflush(m_hFile);
	#endif
	}
	
	if (needToOpen) CloseFile();

	#ifndef _WIN32
	if (m_console) {
		::fwrite(pData, len, 1, stdout);
	}
	#endif
}


void RLLog::WriteMsgV(const char* pTempl, va_list arg)
{
	if (!m_initialized) return;

	const int MAX_SIZE = 2048;
	char msg[MAX_SIZE];

	int c;

	if (m_exactTime) {
		c = sprintf(msg, "%.6f %.8X - ", globalTime.GetElapsedSeconds(), ::GetCurrentThreadId());
	}
	else {
		SYSTEMTIME t;
		::GetLocalTime(&t);

		c = sprintf(msg, "%.4hd%.2hd%.2hd-%.2hd:%.2hd:%.2hd.%.3hd %.8X - ",
					t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds,
					::GetCurrentThreadId());
	}

	c += vsprintf(msg+c, pTempl, arg);

	msg[c++] = '\r';
	msg[c++] = '\n';
	
	if (c>MAX_SIZE) {
		OnError("CRLLog::WriteMsgA() buffer overrun");
		return;
	}
	
	WriteRawData(msg, c*sizeof(char));
}


#ifdef _WIN32
void RLLog::WriteMsgV(const WCHAR* pTempl, va_list arg)
{
	if (!m_initialized) return;

	WCHAR msg[1024];
	
	/*
	SYSTEMTIME t;
	GetLocalTime(&t);

	int c = swprintf(msg, L"%.4hd%.2hd%.2hd-%.2hd:%.2hd:%.2hd.%.3hd %.8X - ", 
				t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds,
				::GetCurrentThreadId());
	*/

	int c = swprintf(msg, L"%.6Lf %.8X - ", globalTime.GetElapsedSeconds(), ::GetCurrentThreadId());


	c += vswprintf(msg+c, pTempl, arg);

	msg[c++] = '\r';
	msg[c++] = '\n';
	
	if (c>1024) {
		OnError("CRLLog::WriteMsgW() buffer overrun");
		return;
	}

	WriteRawData(msg, c*sizeof(WCHAR));
}
#endif


void RLLog::WriteMsg(const char* templ, ...)
{
	va_list arg;
	va_start(arg, templ);
	WriteMsgV(templ, arg);
	va_end(arg);
}


//_______________________________________________________________________________________________________



RLLogEx::RLLogEx()
{
	m_level = 0;	// 0-error, 1-warning, 2-info; only errors by default
}


void RLLogEx::SetLogLevel(DWORD level)
{
	m_level = level;
}


void RLLogEx::WriteMsgV_withPrefix(const char* pPrefix, const char* pTempl, va_list arg)
{
	char msgTemp[1024];

	if (pPrefix==NULL) {
		WriteMsgV(pTempl, arg);
	}
	else {
		if (strlen(pPrefix) + strlen(pTempl) > 1024-1) {
			OnError("CLog::WriteMsgA() buffer overrun");
			return;
		}

		strcpy(msgTemp, pPrefix);
		strcat(msgTemp, pTempl);

		WriteMsgV(msgTemp, arg);
	}
}


void RLLogEx::WriteError(const char* templ, ...)
{
	va_list arg;
	va_start(arg, templ);
	WriteMsgV_withPrefix("ERROR: ", templ, arg);
	va_end(arg);	
}


void RLLogEx::WriteInfo(const char* templ, ...)
{
	if (m_level<2)
		return;

	va_list arg;
	va_start(arg, templ);
	WriteMsgV_withPrefix(NULL, templ, arg);
	va_end(arg);
}

