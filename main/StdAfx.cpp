// stdafx.cpp : source file that includes just the standard includes
//	Installer.pch will be the pre-compiled header
//	stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

RLResource  rlResource;
Settings	settings;

//RLLogEx	_log;
RLLogEx2 _log( __FILE__);

