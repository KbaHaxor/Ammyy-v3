#include "stdafx.h"
#include "vrMain.h"
#include "vrClient.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

VrMain vrMain; // global object!


VrMain::VrMain() 
{
	m_hInstance = (HINSTANCE)0x400000; // exe module
	m_pHotkeys  = NULL;
	m_pHelp     = NULL;
}

VrMain::~VrMain()
{
	if (m_pHotkeys) delete m_pHotkeys;
	if (m_pHelp)    delete m_pHelp;
}

// _______________________________________________


void VrMain::OnExitProcess()
{
	{
		omni_mutex_lock l(VrClient::m_objectsLock);

		int count = VrClient::m_objects.size();
		for (int i=0; i<count; i++) {
			((VrClient*)VrClient::m_objects[i])->OnExitProcess();
		}
	}

	while(true) // wait until all viewers will be terminated
	{
		{
			omni_mutex_lock l(VrClient::m_objectsLock);
			if (VrClient::m_objects.size()==0) break;
		}
		::Sleep(2);
	}
}


// Move the given window to the centre of the screen and bring it to the top
void VrMain::CentreWindow(HWND hwnd)
{
	RECT winrect, workrect;
	
	// Find how large the desktop work area is
	RLWnd::GetWorkArea(&workrect);
	int workCX = workrect.right  - workrect.left;
	int workCY = workrect.bottom - workrect.top;
	
	// And how big the window is
	::GetWindowRect(hwnd, &winrect);
	int winCX = winrect.right  - winrect.left;
	int winCY = winrect.bottom - winrect.top;
	// Make sure it's not bigger than the work area
	winCX = min(winCX, workCX);
	winCY = min(winCY, workCY);

	int x = workrect.left + (workCX-winCX) / 2;
	int y = workrect.top  + (workCY-winCY) / 2;

	// Now centre it
	::SetWindowPos(hwnd, HWND_TOP, x, y, winCX, winCY, SWP_SHOWWINDOW);
	::SetForegroundWindow(hwnd);
}
