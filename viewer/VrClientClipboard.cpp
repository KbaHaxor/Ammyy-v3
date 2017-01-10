#include "stdafx.h"
#include "vrMain.h"
#include "vrClient.h"
#include "../main/Common.h"
#include "../target/TrDesktop.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

// This file contains the code for getting text from, and putting text into
// the Windows clipboard.


// We've been informed that the local clipboard has been updated.
// If it's text we want to send it to the server.
//
void VrClient::OnLocalClipboardChange()
{
	if (!m_desktop_on) return; // desktop is off - ignore
	if (!m_running) return;
	if (!m_prm.Get(Permission::ClipboardIn)) return; // target don't allow ClipboardIn

	HWND hOwner = ::GetClipboardOwner();
	if (hOwner == m_hwnd2) return; //We changed it - ignore!
		

	if (m_opts.m_allowClipboardOut)
	{
		// The clipboard should not be modified by more than one thread at once
		omni_mutex_lock l(m_clipMutex);
		
		if (::OpenClipboard(m_hwnd2)!=0)
		{
			HGLOBAL hglb2 = ::GetClipboardData(CF_UNICODETEXT); 

			if (hglb2 == NULL) {
				::CloseClipboard();
			} else {
				LPCWSTR lpstr = (LPCWSTR)::GlobalLock(hglb2);

				RLStream utf8;
				CCommon::ConvToUTF8(lpstr, utf8);
				
				::GlobalUnlock(hglb2);
				::CloseClipboard();
				
				// Translate to Unix-format lines before sending
				char* pContent     = (char*)utf8.GetBuffer();
				char* pContentWin  = pContent;
				char* pContentUnix = pContent;
				while(true) {
					char c = *pContentWin++;
					if (c != '\x0d') {
						*pContentUnix++ = c;
						if (c==0) break;
					}
				}

				int contentLen = pContentUnix-pContent-1;

				SendClientCutText(pContent, contentLen);
			}
		}
	}
}

// We've read some text from the remote server, and we need to copy it into the local clipboard.
// Called by VrClient::OnAaCutText()

void VrClient::UpdateLocalClipboard(char *buf, int len) 
{	
	if (m_opts.m_allowClipboardIn) {
		// The clipboard should not be modified by more than one thread at once
		omni_mutex_lock l(m_clipMutex);
		TrDesktop::SetLocalClipboard(m_hwnd2, buf, len);
	}
}
