#if !defined(_STDAFX_H__ROUTER_0FFA3BFAD75__INCLUDED_)
#define _STDAFX_H__ROUTER_0FFA3BFAD75__INCLUDED_

#define AMMYY_ROUTER

#ifdef _WIN32

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define _WIN32_WINNT 0x0500
//#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <Winsock2.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <malloc.h>
#include <crtdbg.h>
//#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#include "../RL/StringW.h" // for ServiceManager.cpp & Common.cpp

void DEBUG(LPCSTR errMsg, ...);

#else //_________________________________________________________________________________________________________

typedef const char *LPCSTR;
typedef const wchar_t *LPCWSTR;
typedef char INT8, *LPSTR;
typedef wchar_t *LPWSTR;
typedef int BOOL;
typedef unsigned int UINT;
typedef long LONG;
typedef unsigned char BYTE;
typedef unsigned short      WORD;
typedef unsigned long	DWORD_PTR, *PDWORD_PTR;
typedef unsigned long   DWORD;
typedef unsigned char   UINT8, *PUINT8;
typedef unsigned short  UINT16, *PUINT16;
typedef unsigned int    UINT32, *PUINT32;
typedef wchar_t WCHAR;
typedef int HMODULE;
typedef int SOCKET;
typedef long long INT64;
typedef unsigned long long UINT64;
typedef int INT32;
#define __stdcall
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef int INT;

#include <inttypes.h>
#define __int64 int64_t
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1 
#define PASCAL __stdcall
#define TRUE 1
#include <cstddef>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include <../../config.h>
#endif

#define __forceinline inline

//#define RL_ERROR(x) printf("ERROR:%s\n", (LPCSTR)x);

#define HIWORD(l)           ((WORD)((DWORD_PTR)(l) >> 16))

template <class T> inline const T& max ( const T& a, const T& b ) { return (b<a)?a:b; }
template <class T> inline const T& min ( const T& a, const T& b ) { return (a<b)?a:b; }

struct SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
};

void GetLocalTime (SYSTEMTIME* lpTime);
void GetSystemTime(SYSTEMTIME* lpTime);
void Sleep(DWORD dwMilliseconds);
DWORD GetTickCount();

#ifdef _FreeBSD
#include <stdarg.h>
#endif


/* just for knowing
enum EPOLL_EVENTS
{
    EPOLLIN = 0x001,
    EPOLLPRI = 0x002,
    EPOLLOUT = 0x004,
    EPOLLRDNORM = 0x040,
    EPOLLRDBAND = 0x080,
    EPOLLWRNORM = 0x100,
    EPOLLWRBAND = 0x200,
    EPOLLMSG = 0x400,
    EPOLLERR = 0x008,
    EPOLLHUP = 0x010,
    EPOLLONESHOT = (1 << 30),
    EPOLLET = (1 << 31)
};
*/

#endif// _____________________________________________________________________________________________________

#include "../RL/StringA.h"
#include "../RL/RLLog.h"
#include "../RL/RLStream.h"
#include "../RL/RLException.h"
#include "../RL/RLTimer.h"
#include "../RL/RLEH.h"

#define CString CStringA
#define ASSERT RL_ASSERT
#define VERIFY RL_ASSERT

extern RLLogEx _log;
//extern RLLogEx _logWeb;


#ifdef _WIN32
#define RLEH_SOURCE_BASE_FOLDER "C:\\my\\projects\\AMMYY\\sources"
#include "../main/Common.h"
#else
#define RLEH_SOURCE_BASE_FOLDER "/home/maxim/ammyy_router/src/"
#endif

#include "RLHttp2.h"
#include "../common/Common2.h"

#endif // !defined(_STDAFX_H__ROUTER_0FFA3BFAD75__INCLUDED_)
