#include "stdafx.h"
#include "vrMain.h"
#include "vrHotKeys.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

VrHotKeys::VrHotKeys()
{
	m_hwnd = 0;

	const int MAX_ACCELS = 16;
	ACCEL accel[MAX_ACCELS];
	int i = 0;

	accel[i].fVirt = FVIRTKEY | FALT | FCONTROL | FSHIFT | FNOINVERT;
	accel[i].key = 0x46;	// "F"
	accel[i++].cmd = ID_FULLSCREEN;

	accel[i].fVirt = FVIRTKEY | FALT | FCONTROL | FSHIFT | FNOINVERT;
	accel[i].key = 0x52;	// "R"
	accel[i++].cmd = ID_REQUEST_REFRESH;

	ASSERT(i <= MAX_ACCELS);

	m_hAccel = ::CreateAcceleratorTable((LPACCEL)accel, i);
}

bool VrHotKeys::TranslateAccel(MSG *pmsg)
{
	return (TranslateAccelerator(m_hwnd, m_hAccel, pmsg) != 0);
}

VrHotKeys::~VrHotKeys()
{
	DestroyAcceleratorTable(m_hAccel);
}
