// CursorMono and CursorRich encodings
//
// Support for cursor shape updates for VrClient class.

#include "stdafx.h"
#include "VrMain.h"
#include "VrClient.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

void VrClient::ReadCursorShape(aaFramebufferUpdateRectHeader *pfburh) 
{
	_log2.Print(6, VTCLOG("Receiving cursor shape update, cursor %dx%d"), (int)pfburh->r.w, (int)pfburh->r.h);

	SoftCursorFree();

	bool isColor = (pfburh->encoder == aaEncoderCursorRich);
	int pixels = pfburh->r.h * pfburh->r.w;
	int bytesMaskData = (pixels + 7) /8;
	if (pixels == 0) return; // Empty cursor

	BYTE* buffer1 = (BYTE*)m_netBuffer.GetBuffer1(bytesMaskData);

	// Read and decode mask data.
	ReadExact(buffer1, bytesMaskData);

	int pixel_colors = BitArray::GetCountBits((BYTE*)buffer1, bytesMaskData);
	int color_size = (isColor) ? pixel_colors * m_bytesppR : (pixel_colors+7)/8;

	RLStream color(color_size);
	BYTE* pColor = (BYTE*)color.GetBuffer();

	ReadExact(pColor, color_size);

	// Ignore cursor shape updates if requested by user
	if ((m_opts.m_cursorRemoteRequest & aaSetPointerShape)==0) return;

	m_cursor.mask1 .SetMinCapasity(pixels * sizeof(bool));
	m_cursor.source.SetMinCapasity(pixels * m_bytesppL);

	bool* pMask = (bool*)m_cursor.mask1.GetBuffer();

	// read mask
	{
		BitArray bitMR(buffer1);
		for (int i=0; i<pixels; i++) pMask[i] = bitMR.Read();
	}

	if (!isColor)
	{
		BitArray bitCR(pColor);

		for (int i=0; i<pixels; i++) {
			if (!pMask[i]) continue;
			bool c = bitCR.Read();
			void* dst = (BYTE*)m_cursor.source.GetBuffer() + i*m_bytesppL;
			CopyPixelDDB(dst, &m_ddbColors[(c) ? 1 : 0]);
		}

	} else {		
		BYTE* buf = (BYTE*)m_netBuffer.GetBuffer1(pixel_colors*m_bytesppL);
		m_translator->Translate(buf, pColor, pixel_colors);

		for (int i=0; i<pixels; i++) {
			if (!pMask[i]) continue;
			void* dst = (BYTE*)m_cursor.source.GetBuffer() + i*m_bytesppL;
			CopyPixelDDB(dst, buf);
			buf += m_bytesppL;
		}
	}

	// Set remaining data associated with cursor.

	omni_mutex_lock l(m_cursor.mutex);

	_log2.Print(6, VTCLOG("Receiving cursor shape update, cursor %dx%d"), (int)pfburh->r.w, (int)pfburh->r.h);

	m_cursor.cx = pfburh->r.w;
	m_cursor.cy = pfburh->r.h;
	m_cursor.hotX = min(pfburh->r.x, m_cursor.cx-1);
	m_cursor.hotY = min(pfburh->r.y, m_cursor.cy-1);

	SoftCursorSaveArea();
	SoftCursorDraw();

	m_cursor.hidden  = false;
	m_cursor.lockset = false;
	m_cursor.set = true;
}

void VrClient::OnAaPointerMove()
{
	aaPointerMoveMsg msg;
	ReadExact(((char*)&msg)+1, sizeof(msg)-1);

	if ((m_opts.m_cursorRemoteRequest & aaSetPointerPos)==0) return;

	// check if we have message after last sent of aaPointerEvent
	if (((UINT8)m_counterAaPointerEvent)!=msg.counter) return;

	int x = (int)msg.x;
	int y = (int)msg.y;
	if (x >= m_desktop_cx) x = m_desktop_cx - 1;
	if (y >= m_desktop_cy) y = m_desktop_cy - 1;

	SoftCursorMove(x, y);

	_log2.Print(4, VTCLOG("OnAaPointerMove() x=%d y=%d"), x, y);
}

//
// SoftCursorLockArea(). This method should be used to prevent
// collisions between simultaneous framebuffer update operations and
// cursor drawing operations caused by movements of pointing device.
// The parameters denote a rectangle where mouse cursor should not
// be drawn. Every next call to this function expands locked area so previous locks remain active.
//

void VrClient::SoftCursorLockArea(int x, int y, int w, int h) 
{
	omni_mutex_lock l(m_cursor.mutex);

	if (!m_cursor.set)
		return;

	if (!m_cursor.lockset) {
		m_cursor.lockX  = x;
		m_cursor.lockY  = y;
		m_cursor.lockCX = w;
		m_cursor.lockCY = h;
		m_cursor.lockset = true;
	} else {
		int newX1 = min(x, m_cursor.lockX);
		int newY1 = min(y, m_cursor.lockY);
		int newX2 = max(x+w, m_cursor.lockX + m_cursor.lockCX);
		int newY2 = max(y+h, m_cursor.lockY + m_cursor.lockCY);

		m_cursor.lockCX = newX2 - newX1;
		m_cursor.lockCY = newY2 - newY1;
		m_cursor.lockX  = newX1;
		m_cursor.lockY  = newY1;
	}

	if (!m_cursor.hidden && m_cursor.IsInLockedArea()) {
		SoftCursorRestoreArea();
		m_cursor.hidden = true;
	}
}

//
// SoftCursorUnlockScreen(). This function discards all locks
// performed since previous SoftCursorUnlockScreen() call.
//

void VrClient::SoftCursorUnlockScreen() 
{
	omni_mutex_lock l(m_cursor.mutex);

	if (!m_cursor.set)
		return;

	if (m_cursor.hidden) {
		SoftCursorSaveArea();
		SoftCursorDraw();
		m_cursor.hidden = false;
	}
	m_cursor.lockset = false;
}

//
// SoftCursorMove(). Moves soft cursor in particular location. This
// function respects locking of screen areas so when the cursor is
// moved in the locked area, it becomes invisible until
// SoftCursorUnlockScreen() method is called.
//

void VrClient::SoftCursorMove(int x, int y) 
{
	omni_mutex_lock l(m_cursor.mutex);

	//_log.WriteInfo("SoftCursorMove() %d %d", x, y);

	if (m_cursor.set && !m_cursor.hidden) {
		SoftCursorRestoreArea();
		m_cursor.hidden = true;
	}

	m_cursor.x = x;
	m_cursor.y = y;

	if (m_cursor.set && !(m_cursor.lockset && m_cursor.IsInLockedArea())) {
		SoftCursorSaveArea();
		SoftCursorDraw();
		m_cursor.hidden = false;
	}
}

 //
 // Free all data associated with cursor.
 //

void VrClient::SoftCursorFree() 
{
	omni_mutex_lock l(m_cursor.mutex);

	if (m_cursor.set) {
		if (!m_cursor.hidden)
			SoftCursorRestoreArea();
		m_cursor.source.Free();
		m_cursor.mask1 .Free();
		m_cursor.screen.Free();
		m_cursor.set = false;
	}
}

//////////////////////////////////////////////////////////////////
//
// Low-level methods implementing software cursor functionality.
//

//
// Check if cursor is within locked part of screen.
//

bool VrClient::Cursor::IsInLockedArea() {

    return (lockX < x - hotX + cx &&
			lockY < y - hotY + cy &&
			lockX + lockCX > x - hotX &&
			lockY + lockCY > y - hotY);
}

//
// Save screen data in memory buffer.
//
void VrClient::SoftCursorSaveArea()
{
	RECT r;
	SoftCursorToScreen(&r);
	int x = r.left;
	int y = r.top;
	int w = r.right - r.left;
	int h = r.bottom - r.top;

	RLMutexLock l(m_bitmapdcMutex);
	
	int bytesPerRow = w * m_bytesppL;
	int bytesPerRowScreen = m_bytesppL*m_desktop_cx;
	BYTE* src = (BYTE*)this->GetScreenPtr(x, y, m_bytesppL);
	BYTE* dst = (BYTE*)m_cursor.screen.GetBuffer1(bytesPerRow*h);

	for (;h>0;h--) {
		memcpy(dst, src, bytesPerRow);
		dst += bytesPerRow;
		src -= bytesPerRowScreen;
	}
}


//
// Restore screen data saved in memory buffer.
//
void VrClient::SoftCursorRestoreArea()
{
	RECT r;
	SoftCursorToScreen(&r);
	int x = r.left;
	int y = r.top;
	int w = r.right  - r.left;
	int h = r.bottom - r.top;

	RLMutexLock   l(m_bitmapdcMutex);

	int bytesPerRow = w * m_bytesppL;
	int bytesPerRowScreen = m_bytesppL*m_desktop_cx;
	BYTE* dst = (BYTE*)this->GetScreenPtr(x, y, m_bytesppL);
	BYTE* src = (BYTE*)m_cursor.screen.GetBuffer();

	for (;h>0;h--) {
		memcpy(dst, src, bytesPerRow);
		dst -= bytesPerRowScreen;
		src += bytesPerRow;
	}

	InvalidateScreenRect(&r);
}

//
// Draw cursor.
//

void VrClient::SoftCursorDraw() 
{	
	RLMutexLock l(m_bitmapdcMutex);

	//_log.WriteInfo("SoftCursorDraw() %d %d", m_cursor.x, m_cursor.y);

	BYTE* source_ptr = (BYTE*)m_cursor.source.GetBuffer();
	bool* mask_ptr   = (bool*)m_cursor.mask1 .GetBuffer();

	for (int y=0; y<m_cursor.cy; y++) {
		int y0 = m_cursor.y - m_cursor.hotY + y;
		if (y0<0 || y0 >= m_desktop_cy) continue;
		for (int x = 0; x < m_cursor.cx; x++) {
			int x0 = m_cursor.x - m_cursor.hotX + x;
			if (x0<0 || x0>=m_desktop_cx) continue;			
			int offset = y * m_cursor.cx + x;
			if (mask_ptr[offset]) {
				SetPixelDDB(x0, y0, source_ptr+offset*m_bytesppL);
			}
		}	
	}	

	RECT r;
	SoftCursorToScreen(&r);
	InvalidateScreenRect(&r);
}





//
// Calculate position, size and offset for the part of cursor located inside framebuffer bounds.

void VrClient::SoftCursorToScreen(RECT *screenArea)
{
	int cx = 0, cy = 0;

	int x = m_cursor.x - m_cursor.hotX;
	int y = m_cursor.y - m_cursor.hotY;
	int w = m_cursor.cx;
	int h = m_cursor.cy;

	if (x < 0) {
		cx = -x;
		w -= cx;
		x = 0;
	} else if (x + w > m_desktop_cx) {
		w = m_desktop_cx - x;
	}
	if (y < 0) {
		cy = -y;
		h -= cy;
		y = 0;
	} else if (y + h > m_desktop_cy) {
		h = m_desktop_cy - y;
	}

	if (w < 0) {
		cx = 0; x = 0; w = 0;
	}
	if (h < 0) {
		cy = 0; y = 0; h = 0;
	}

	::SetRect(screenArea, x, y, x + w, y + h);	
}

