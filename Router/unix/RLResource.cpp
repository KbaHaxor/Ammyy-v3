#include "stdafx.h"
#include "RLResource.h"

#include "res/main.html.h"
#include "res/login.html.h"

DWORD RLResource::Load(HMODULE hModule, LPCSTR lpType, LPCSTR lpName, WORD wLanguage, LPCVOID* lpData)
{
	ASSERT(lpType==RT_HTML);
	
	if (lpName==(LPCSTR)IDR_HTML_MAIN) {
		*lpData = main_html;
		return sizeof(main_html)-1;
	}
	else if (lpName==(LPCSTR)IDR_HTML_LOGIN) {
		*lpData = login_html;
		return sizeof(login_html)-1;
	}
	else
		throw RLException("ERROR in RLResource::LoadResource()");
}


