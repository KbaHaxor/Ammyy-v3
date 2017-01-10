#if !defined(AFX_RLRESOURCE_H__INCLUDED_)
#define AFX_RLRESOURCE_H__INCLUDED_

class RLResource  
{
public:
	static DWORD Load(HMODULE hModule, LPCSTR lpType, LPCSTR lpName, WORD wLanguage, LPCVOID* lpData);
};

#ifndef _WIN32

#define IDR_HTML_MAIN                   131
#define IDR_HTML_LOGIN                  132
#define MAKEINTRESOURCE(x) (LPCSTR)x
#define RT_HTML         MAKEINTRESOURCE(23)

#endif

#endif // !defined(AFX_RLRESOURCE_H__INCLUDED_)
