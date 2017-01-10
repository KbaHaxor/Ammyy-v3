#if !defined(AFX_STDAFX_H__772377D5_F622_4C59_B85F_33E65FEB6128__INCLUDED_)
#define AFX_STDAFX_H__772377D5_F622_4C59_B85F_33E65FEB6128__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#define _WIN32_WINNT 0x0500

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
//#ifdef _CRTDBG_MAP_ALLOC
//#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#include "../RL/StringA.h"
#define CString CStringA
#include "../RL/RLLog.h"
extern RLLogEx _log;

//#pragma warning(disable : 4786)

#define ASSERT RL_ASSERT
#define VERIFY RL_ASSERT


#include "../RL/RLStream.h"
#include "../RL/RLEH.h"
#include "../RL/RLException.h"
#include "../RL/CmdBase.h"
#include "../RL/RLResource.h"
#include "../RL/RLTimer.h"



#define SAFE_RELEASE(p)		if (p != NULL) { p->Release(); p = NULL; }

#if _MSC_VER < 1300
typedef unsigned int	uintptr_t;
#endif


#endif // !defined(AFX_STDAFX_H__772377D5_F622_4C59_B85F_33E65FEB6128__INCLUDED_)
