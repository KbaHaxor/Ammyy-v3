#if !defined(_COMMON_INTEROP_H__INCLUDED_)
#define _COMMON_INTEROP_H__INCLUDED_

#include "Transport.h"

class InteropCommon
{
public:
	InteropCommon();
	virtual ~InteropCommon();

	virtual DWORD GetRemoteId();
	virtual bool  IsTarget() = 0;

	static void SendInfo (LPCSTR templ, ...);
	static void ShowError(LPCSTR error);

	virtual void SetStoppingEvent(BOOL* pEvent);

	virtual void SetState(DWORD state) {}
	virtual void AddToInitMsg(RLStream& buffer) {}

	// 0-off, 1-started, 2-TCP connect, 3-connected to router, 4-operator is on
	enum STATE
	{
		OFF=0,
		STARTED=1,
		TCP_CONNECTED=2,
		ROUTER_CONNECTED=3,
		OPERATOR_CONNECTED=4,
		OPERATOR_AUTHORIZED=5,
	};	

public:
	        void         Thread01_start();
private:
	static  DWORD WINAPI Thread01_static(LPVOID lpParameter);
protected:
	virtual void		 Thread01() {}

protected:	
	void   ConnectToRouter();
	void   SendInitMsg(SOCKET s);
public:
	void   ReadInitMsg(bool lan, UINT32& myId, UINT32& remoteId, UINT32& version);
	void   ReadRouterInfo(Transport* transport);
	BYTE   ReadByteWithPinging(bool& waitingPingReply);

private:
	CStringA GetRouters();
	static void ParseRouter(LPCSTR pRouter, CStringA& ip, CStringA& ports);
	SOCKET  ConnectToRouterOneAttempt(LPCSTR routerIP, int routerPort, const CStringA& proxyAddress, UINT16 proxyPort);
	void    ReadNonBlock(SOCKET s, UINT timeout, char* buffer, int len);	

	inline BOOL IsStopping() { return (m_bStopping!=NULL) ? *m_bStopping : FALSE; }

public:	
	inline void  SendByte(BYTE b, BOOL block) {m_transport->SendExact(&b, 1, block);}
	inline UINT8 ReadByte() { UINT8 b; m_transport->ReadExact(&b, 1); return b; }
	inline void  ReadStream16(RLStream& buffer) { m_transport->ReadStream16(buffer); }

public:
	static HWND	  m_hErrorWnd;
	UINT32	m_remoteId;	

protected:
	Transport* m_transport;
	int		m_type;	// aaTypeTarget || aaTypeViewer
	int     m_counterAaPointerEvent;

private:
	BOOL*	m_bStopping;
	UINT16  m_external_port; // external port, returned by router
protected:
	UINT32  m_external_ip;	// external ip,   returned by router
	UINT64  m_router_time;  // begin of session timestamp UTC, returned by router
};

class CStoppingException
{
public:
	CStoppingException::CStoppingException();
};

class CWarningException: public RLException	// not critical exeption
{
public:
	CWarningException(LPCSTR templ, ...);
};


#endif // !defined(_COMMON_INTEROP_H__INCLUDED_)
