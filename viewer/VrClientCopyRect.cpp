#include "stdafx.h"
#include "vrMain.h"
#include "vrClient.h"

void VrClient::ReadCopyRect() 
{
	aaCopyRect msg;	
	ReadExact(((char *)&msg)+1, sizeof(msg)-1);

	
	// If *Cursor encoding is used, we should extend our "cursor lock area"
	// (previously set to destination rectangle) to the source rect as well.
	SoftCursorLockArea(msg.r.x,  msg.r.y,  msg.r.w, msg.r.h);
	SoftCursorLockArea(msg.srcX, msg.srcY, msg.r.w, msg.r.h);

	RLMutexLock l(m_bitmapdcMutex);

	if (!::BitBlt(m_hBitmapDC, msg.r.x, msg.r.y, msg.r.w, msg.r.h, m_hBitmapDC, msg.srcX, msg.srcY, SRCCOPY))
	{
		//_log2.Print(0, VTCLOG("Error in blit in VrClient::CopyRect"));
		throw RLException("Error in blit ReadCopyRect()!");
	}

	InvalidateScreenRect(&msg.r);
}
