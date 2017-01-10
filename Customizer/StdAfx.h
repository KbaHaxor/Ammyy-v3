#if !defined(AFX_STDAFX_H__01501A10_4076_4389_9740_B617F02392B5__INCLUDED_)
#define AFX_STDAFX_H__01501A10_4076_4389_9740_B617F02392B5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#define _WIN32_WINNT 0x0500

#include <windows.h>
#include <commctrl.h>

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <malloc.h>
#include <crtdbg.h>
//#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


#include "../RL/StringA.h"
#include "../RL/StringW.h"
#include "../RL/RLStream.h"


#define CString CStringA

#include "../main/Settings.h"

extern Settings settings;

#include <assert.h>

#define ASSERT assert
#define VERIFY(x) if (!(x)) assert(false)

#include "../RL/RLLog.h"

extern RLLogEx	_log;

#define _log2 _log

#define CUSTOMIZER

#include "../main/RLLanguages.h"

#define COUNTOF(x)	(sizeof(x) / sizeof(x[0]))


#endif // !defined(AFX_STDAFX_H__01501A10_4076_4389_9740_B617F02392B5__INCLUDED_)
