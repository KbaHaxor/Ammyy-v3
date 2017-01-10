#if !defined(_AMMYY_CUSTOMIZER_H__B2B583EC__INCLUDED_)
#define _AMMYY_CUSTOMIZER_H__B2B583EC__INCLUDED_

#include "resource.h"		// main symbols

class CCustomizerApp //: public CWinApp
{
public:
	CCustomizerApp();
	void WinMain();

public:
	HINSTANCE	m_hInstance;
	LPCSTR		m_lpCmdLine;
};

class Updater
{
public:
	Updater();
	~Updater();
	static void RemoveSignature(LPCSTR lpExePath);
	void DoUpdate();

	CStringA m_exePath;
	CStringA m_iconPath;
	CStringA m_privateRouters;
	CStringA m_url;

private:
	void DoUpdateIcon();
	void CloseHandle();

	WORD	m_wLanguage;
	HANDLE  m_hExe;
};

#endif // !defined(_AMMYY_CUSTOMIZER_H__B2B583EC__INCLUDED_)
