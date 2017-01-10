#ifndef _VR_MAIN_H__INCLUDED_
#define _VR_MAIN_H__INCLUDED_

#include "res/resource.h"
#include "vrHelp.h"
#include "vrHotKeys.h"
#include "../main/InteropViewer.h"

#define WM_REGIONUPDATED WM_USER+2

class VrMain
{
public:	
	VrMain();
	~VrMain();

	static void CentreWindow(HWND hwnd);
	static void OnExitProcess();

	HINSTANCE	m_hInstance;
	VrHelp*		m_pHelp;
	VrHotKeys*  m_pHotkeys;	// Global logger - may be used by anything

private:	
	static DWORD WINAPI ConnectThreadProc(LPVOID lpParameter);
};

extern VrMain vrMain;

#endif // _VR_MAIN_H__INCLUDED_

