#if !defined(AFX_STDAFX_H__772377D5_F622_4C59_B85F_33E65FEB6128__INCLUDED_)
#define AFX_STDAFX_H__772377D5_F622_4C59_B85F_33E65FEB6128__INCLUDED_


#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#define _WIN32_WINNT 0x0500

#pragma warning (disable : 4786)

// to avoid linking with msvcp60.dll for functions _lockit@std@@
#include <../common/YVALS_my.H>

#include <Winsock2.h>
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <stdio.h>
#include <process.h>


#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <malloc.h>
#include <crtdbg.h>
//#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


#include "../RL/StringA.h"
#include "../RL/StringW.h"
#define CString CStringA
#include "../RL/RLLog.h"
#include "../common/vtcLog.h"
extern RLLogEx2 _log;


#define RLEH_SOURCE_BASE_FOLDER "C:\\projects\\AMMYY\\sources"
#define ASSERT RL_ASSERT
#define VERIFY RL_ASSERT

#include "../RL/RLStream.h"
#include "../RL/RLEH.h"
#include "../RL/RLException.h"
#include "../RL/RLResource.h"
#include "../RL/RLTimer.h"
#include "Settings.h"
#include "../RL/RLHttp.h"

extern Settings settings;
extern RLResource rlResource;

#include "RLLanguages.h"


#define SAFE_RELEASE(p)		if (p != NULL) { p->Release(); p = NULL; }

#if _MSC_VER < 1300
typedef unsigned int	uintptr_t;
#endif

#ifndef LVS_EX_DOUBLEBUFFER
#define LVS_EX_DOUBLEBUFFER     0x00010000
#endif

#define COUNTOF(x)	(sizeof(x) / sizeof(x[0]))

#define AMMYY_ADMIN

//_______________________________________ TARGET & VIEWER ________________________________________

// for omnithread
#define __WIN32__
#define __NT__
#define __x86__
#define _WINSTATIC
#define NCORBA
//#define WINVER 0x0500

#define INVALID_PROTOCOL RLException("Invalid protocol %u", __LINE__)

#include "..\common\Debug2.h"
#include "..\main\AmmyyApp.h"

#endif // !defined(AFX_STDAFX_H__772377D5_F622_4C59_B85F_33E65FEB6128__INCLUDED_)
