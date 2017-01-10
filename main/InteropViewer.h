#if !defined(_VIEWER_INTEROP_H__INCLUDED_)
#define _VIEWER_INTEROP_H__INCLUDED_

#include "InteropCommon.h"

class EncryptorRSA;

class InteropViewer: public InteropCommon
{
public:
	InteropViewer();
	~InteropViewer();

	void FreeRSA();

	virtual bool  IsTarget() { return false; }

	virtual void RunRDPclient(HANDLE& hRDPClient, DWORD& dwProcessId, HWND hwnd, UINT msg);

protected:
	virtual void AddToInitMsg(RLStream& buffer);

private:
	static SOCKET AcceptSocket(SOCKET s, DWORD timeout);

	static HANDLE RunMstsc(WORD port, DWORD& dwProcessId);

public:
	EncryptorRSA* m_pRSA;

public:
	INT8	 m_encryption;
	DWORD	 m_profile;
	CStringW m_caption;

	enum VrProfileType
	{
		VrDesktop = 100,
		VrAudioChat = 0,
		VrFileManagerOnly = 1,
		VrSpeedTestOnly = 2,
		VrRDP = 3, // Only
	};
};

#endif // !defined(_VIEWER_INTEROP_H__INCLUDED_)
