#if !defined(_TCP_H__D42C12F9__INCLUDED_)
#define _TCP_H__D42C12F9__INCLUDED_

class TCP  
{
public:
	TCP();
	~TCP();
	void   Attach(SOCKET socket);
	SOCKET Detach();
	void   Close();
	void   CloseSafe();
	void   Listen(UINT16 port);
	SOCKET Accept(UINT timeout_ms);

	void   Create();
	DWORD  Connect(UINT32 ip_v4, UINT16 port);
	DWORD  Connect(LPCSTR host,  UINT16 port, bool exception);

	int    TryRead(void* buffer, int len, UINT32 timeout_ms);

	bool IsOpened() { return (m_socket != INVALID_SOCKET); } 

	static void SetBlockingMode(SOCKET s, bool blocking);
	static void SetBufferSizes(SOCKET s);
	static void SetLinger(SOCKET s, bool on, int time_seconds);
	static UINT32 ResolveHostname_IPv4(LPCSTR hostname);
	static SOCKET   Connect2(LPCSTR host, int port, DWORD error);
	static SOCKET   Connect(UINT32 ip_v4, UINT16 port, int timeout_ms);
	static SOCKET   ListenFreePort(WORD portFrom, WORD portTill, WORD& port);
	static CStringA GetPeerIPv4(SOCKET s);
	static void     GetIPaddresses(RLStream& ips);
	static CStringA GetIPaddresses();


	operator SOCKET () { return m_socket; }

	static void   SetSocketOptions(SOCKET s);


private:
	static void   SetKeepAlive(SOCKET s);
	static void   DisableNagle(SOCKET s);

private:
	SOCKET m_socket;

};

#endif // !defined(_TCP_H__D42C12F9__INCLUDED_)
