#include "stdafx.h"

RLLogEx _log;
//RLLogEx _logWeb;

#ifdef _WIN32

void DEBUG(LPCSTR errMsg, ...)
{
  #ifdef _DEBUG
  char buffer[256];
 
  va_list arg;
  va_start( arg, errMsg );
  vsprintf( buffer, errMsg, arg );
  va_end( arg );
 
  OutputDebugString(buffer);
  OutputDebugString("\n");
 
  #endif
}

#else

#include <sys/timeb.h>
#include <unistd.h>
#include "../RL/RLTimer.h"

RLTimer getTickCount;


void GetLocalTime(SYSTEMTIME* lpTime)
{
	struct timeb t1;
	ftime(&t1);

	struct tm* tmLocalTime = localtime(&t1.time);
	lpTime->wYear   = tmLocalTime->tm_year + 1900;
	lpTime->wMonth  = tmLocalTime->tm_mon + 1;
	lpTime->wDay    = tmLocalTime->tm_mday;
	lpTime->wHour   = tmLocalTime->tm_hour;
	lpTime->wMinute = tmLocalTime->tm_min;
	lpTime->wSecond = tmLocalTime->tm_sec;
	lpTime->wMilliseconds = t1.millitm;
}

void GetSystemTime(SYSTEMTIME* lpTime)
{
	struct timeb t1;
	ftime(&t1);

	struct tm* tmLocalTime = gmtime(&t1.time);
	lpTime->wYear   = tmLocalTime->tm_year + 1900;
	lpTime->wMonth  = tmLocalTime->tm_mon + 1;
	lpTime->wDay    = tmLocalTime->tm_mday;
	lpTime->wHour   = tmLocalTime->tm_hour;
	lpTime->wMinute = tmLocalTime->tm_min;
	lpTime->wSecond = tmLocalTime->tm_sec;
	lpTime->wMilliseconds = t1.millitm;
}

void Sleep(DWORD dwMilliseconds)
{
	::usleep(dwMilliseconds*1000);
	//VERIFY(::usleep(dwMilliseconds*1000)==0); // cause error if user press Ctrl+C
}

DWORD GetTickCount()
{
	return (DWORD)(getTickCount.GetElapsedSeconds()*1000);
}

#endif
