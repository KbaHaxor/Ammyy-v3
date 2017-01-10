#include "stdafx.h"
#include "TCP.h"
#include "InteropCommon.h"
#include <MSTcpIP.h>
#include <Ws2tcpip.h>

TCP::TCP()
{
	m_socket = INVALID_SOCKET;

}

TCP::~TCP()
{
	this->Close();
}

void TCP::Create()
{
	if (m_socket!=INVALID_SOCKET)
		throw RLException("CreateInternal()#1");

	m_socket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (m_socket==INVALID_SOCKET)
		throw RLException("Error %d socket()", ::GetLastError());
}

void TCP::Close()
{
	if (m_socket!=INVALID_SOCKET) {
		if (::closesocket(m_socket)!=0)
			throw RLException("Error %d closesocket()", ::GetLastError());
		
		m_socket = INVALID_SOCKET;
	}
}

void TCP::CloseSafe()
{
	if (m_socket!=INVALID_SOCKET) {
		::closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}
}

void TCP::Attach(SOCKET socket)
{
	this->Close();
	m_socket = socket;
}

SOCKET TCP::Detach()
{
	SOCKET s = m_socket;
	m_socket = INVALID_SOCKET;
	return s;
}




void TCP::Listen(UINT16 port)
{
	this->Create();

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = 0; //::inet_addr("0.0.0.0");
	addr.sin_port = htons(port);

//#ifdef _WIN32
//	AsyncSelect(true);
//#endif

//#ifndef _WIN32
//	if (TheApp.m_CmdArgs.reuseport) { // doesn't work in Linux
//		int optval = 1;		
//		if(::setsockopt(m_socket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0)
//			_log.WriteError("ERROR %d while setsockopt(SO_REUSEPORT)", errno);
//	}
//#endif

	if (::bind(m_socket, (sockaddr*) &addr, sizeof(addr)) == SOCKET_ERROR) {
		throw RLException("couldn't bind port %d, error = %d",  (int)port, ::WSAGetLastError());
	}

	if (::listen(m_socket,20)!=0) {
		throw RLException("couldn't listen port %d, error = %d", (int)port, ::WSAGetLastError());
	}
}

SOCKET TCP::Accept(UINT timeout_ms)
{
	struct timeval tm;
	tm.tv_sec  = timeout_ms/1000;
	tm.tv_usec = (timeout_ms%1000)*1000;

	struct fd_set fd;
	FD_ZERO(&fd);
	FD_SET(m_socket, &fd);

	int	res = ::select(0, &fd, 0, NULL, &tm);

	if (res==0) return INVALID_SOCKET; // timeout

	if (res>0 && FD_ISSET(m_socket, &fd))
	{
		SOCKET s1 = ::accept(m_socket, NULL, 0);
		if (s1 == INVALID_SOCKET)
			throw RLException("accept() error=%d", ::GetLastError());

		return s1;
	}	
	
	throw RLException("select() error=%d", ::GetLastError());
}

DWORD TCP::Connect(UINT32 ip_v4, UINT16 port)
{ 
	struct sockaddr_in thataddr;
	thataddr.sin_addr.s_addr = ip_v4;
	thataddr.sin_family = AF_INET;
	thataddr.sin_port = htons(port);

	if (::connect(m_socket, (LPSOCKADDR) &thataddr, sizeof(thataddr)) == 0) return 0; // no errors

	return ::WSAGetLastError();
}

DWORD TCP::Connect(LPCSTR host, UINT16 port, bool exception)
{ 
	UINT32 ip_v4 = ResolveHostname_IPv4(host);
	DWORD error = Connect(ip_v4, port);

	if (exception && error!=0)
		throw RLException("Error %d while connecting to '%s', port=%u", error, host, port);
	
	return error;
}


// TODO: with blocking connect(), connection time = ping time, 
// but with Debugging non-blocking connect() + select() usually takes 50ms additional time, but sometimes the same time as ping, I don't know why.
//
SOCKET TCP::Connect(UINT32 ip_v4, UINT16 port, int timeout_ms)
{ 
	TCP tcp;
	tcp.Create();

	TCP::SetBlockingMode(tcp, false);	

	struct sockaddr_in thataddr;
	thataddr.sin_addr.s_addr = ip_v4;
	thataddr.sin_family = AF_INET;
	thataddr.sin_port = htons(port);

	SOCKET s = tcp;

	if (::connect(s, (LPSOCKADDR) &thataddr, sizeof(thataddr)) == 0) 
		goto exit; // no errors, it shouldn't happen

	if (::WSAGetLastError()!=WSAEWOULDBLOCK)
		return 0; // error, it shouldn't happen	
	
	{
		struct timeval tm;
		tm.tv_sec  =  timeout_ms/1000;
		tm.tv_usec = (timeout_ms%1000)*1000;

		struct fd_set fd2, fd3;
		FD_ZERO(&fd2);
		FD_ZERO(&fd3);
		FD_SET(s, &fd2);
		FD_SET(s, &fd3);

		int	res = ::select(0, NULL, &fd2, &fd3, &tm);
		
		if (res<0 || res>1)
			throw RLException("select() error=%d", ::GetLastError());

		// timeout occur OR error while connection
		if (res==0 || (res==1 && FD_ISSET(s, &fd3))) {
			tcp.Close();
			return 0;
		}

		if (!FD_ISSET(s, &fd2))
			throw RLException("select() error=%d", ::GetLastError());
	}

exit:
	TCP::SetBlockingMode(tcp, true); // switch-back to blocking mode
	return tcp.Detach();
}

void TCP::SetSocketOptions(SOCKET s)
{
	TCP::DisableNagle(s);
	TCP::SetKeepAlive(s);
}


void TCP::DisableNagle(SOCKET s)
{
	BOOL nodelay = TRUE;
	if (::setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char *) &nodelay, sizeof(nodelay))!=0)
		throw RLException("ERROR %d disabling Nagle's algorithm", ::WSAGetLastError());	
}

void TCP::SetKeepAlive(SOCKET s)
{
	/* // print old keep alive value
	{
		BOOL b = -1;
		int  len = sizeof(b);
		if (::getsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char*)&b, &len)!=0) {
			_log.WriteError("ERROR %d getsockopt()", ::GetLastError());
		}
		_log.WriteInfo("InteropCommon::Connect()#1 %d %d", (int)b, len);
	}
	*/

	// set old keep alive value
	{
		BOOL b = 1;
		if (::setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char*)&b, sizeof(b))!=0)
			_log.WriteError("ERROR %d setsockopt(SO_KEEPALIVE)", ::WSAGetLastError());
	}

	// set new keep alive value
	{
		tcp_keepalive alive;
		DWORD dwSize;

		alive.onoff = 1;
		alive.keepalivetime = 60*1000;
		alive.keepaliveinterval = 2*1000;

		if (::WSAIoctl(s, SIO_KEEPALIVE_VALS, &alive, sizeof(alive), NULL, 0, &dwSize, NULL, NULL)!=0) {
			throw RLException("ERROR %d while setting keepalive", ::WSAGetLastError());
		}
	}
}

SOCKET TCP::Connect2(LPCSTR host, int port, DWORD error)
{
	TCP socket;
	socket.Create();
	error = socket.Connect(host, port, false);

	if (error > 0)
		return INVALID_SOCKET; // failed connecting to server not throw exception!		

	TCP::SetSocketOptions(socket);
	
	return socket.Detach();
}


UINT32 TCP::ResolveHostname_IPv4(LPCSTR hostname)
{
	// The host may be specified as a dotted address "a.b.c.d", try that first
	UINT32 ip = ::inet_addr(hostname);
	
	if (ip==INADDR_NONE)
	{
		struct addrinfo* result = NULL;
 
		/* resolve the domain name into a list of addresses */
		int error = ::getaddrinfo(hostname, NULL, NULL, &result);

		if (error || !result) {
			if (result) ::freeaddrinfo(result);
			throw RLException("getaddrinfo('%s') error=%d, result=%X", hostname, error, result);
		}

		if (result->ai_family==AF_INET) {
			ip = *((UINT32*)(&result->ai_addr->sa_data[2]));
		}
 
		freeaddrinfo(result);

		if (ip == INADDR_NONE) 
			throw RLException("getaddrinfo('%s') couldn't be resolved");
	}
 
    return ip;
}

SOCKET TCP::ListenFreePort(WORD portFrom, WORD portTill, WORD& port)
{
	TCP socket;

	socket.Create();

	SOCKADDR_IN InternetAddr;
	InternetAddr.sin_family = AF_INET;
	InternetAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	for (WORD curPort=portFrom; curPort<=portTill; curPort++)
	{
		//_log.WriteInfo("ListenFreePort#1 port=%d", (DWORD)curPort);

		InternetAddr.sin_port = htons(curPort);
		if (::bind(socket, (PSOCKADDR) &InternetAddr, sizeof(InternetAddr)) != 0)
			continue; // error was occur
	
		port = curPort;		// got available port

		if (::listen(socket, 3) == SOCKET_ERROR)
			throw RLException("ERROR %d while listen() in ListenFreePort()", ::WSAGetLastError());

		return socket.Detach();
	}

	throw RLException("ERROR while search free TCP ports %u-%u", (UINT)portFrom, (UINT)portTill);
}

// Put the socket into non-blocking mode
//
void TCP::SetBlockingMode(SOCKET s, bool blocking)
{
	u_long arg = blocking ? 0 : 1; // non-blocking mode
	if (ioctlsocket(s, FIONBIO, &arg) != 0)
		throw RLException("Error=%d in SetBlockingMode(%u)", ::GetLastError(), (int)arg);
}

void TCP::SetLinger(SOCKET s, bool on, int time_seconds)
{
	// if also changes SO_DONTLINGER
	linger ll;
	ll.l_onoff = (on) ? 1 : 0;
	ll.l_linger = time_seconds;
	if (::setsockopt(s, SOL_SOCKET, SO_LINGER, (char*)&ll, sizeof(ll)))
		throw RLException("setsockopt(SO_LINGER) %u", ::GetLastError());	

	//linger l = {0};
	//int size = sizeof(l);
	//int h2 = ::getsockopt(m_socket, SOL_SOCKET, SO_LINGER, (char*)&l, &size);
}

void TCP::SetBufferSizes(SOCKET s)
{
	//TODO: in release build it's better to write to log instead of throw exception

	int buffsize1 = 64*1024;
	if (::setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char*)&buffsize1, sizeof(buffsize1))!=0)
		throw RLException("Error=%d setsockopt()", ::GetLastError());
	

	//int buffsize2 = 64*1024;
	//if (::setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&buffsize2, sizeof(buffsize2))!=0)
	//	throw RLException("Error=%d setsockopt()", ::GetLastError());
}

int TCP::TryRead(void* buffer, int len, UINT32 timeout_ms)
{	
	struct fd_set read_fds;

	// Wait until some data can be read
	{
		timeval t;
		t.tv_sec =  timeout_ms / 1000;
		t.tv_usec= (timeout_ms % 1000) * 1000;


		FD_ZERO(&read_fds);
		FD_SET(m_socket, &read_fds);
		int count = select(0, &read_fds, NULL, NULL, &t);
		if (count < 0 || count > 1)
			throw TransportException("ERROR %d in TCP::TryRead()#1 %d", ::GetLastError(), count);
	}

	if (FD_ISSET(m_socket, &read_fds)) {
		// Try to read some data in
		int bytes = ::recv(m_socket, (char*)buffer, len, 0);
		if (bytes > 0)
			return bytes;
		else
			return -1;
	}

	return 0;
}

CStringA TCP::GetPeerIPv4(SOCKET s)
{	
	struct sockaddr_in addr;
	int len = sizeof(addr);
	if (::getpeername(s,(struct sockaddr*)&addr,&len)!=0)
		throw RLException("getpeername() error=%d", ::WSAGetLastError());

	CStringA str;
	LPCSTR ip_v4 = ::inet_ntoa(addr.sin_addr);
	str.Format("%s:%u", ip_v4, (int)htons(addr.sin_port));

	return str;
}


#include <iostream>
#include <windows.h>
#include <iphlpapi.h>


void TCP::GetIPaddresses(RLStream& ips)
{
	ULONG dwBufLen = 0;
	DWORD ret = ::GetAdaptersInfo(NULL, &dwBufLen);

	if (ret==ERROR_NO_DATA) {
		//ASSERT(dwBufLen==0);
		return; // no netword cards on this computer
	}
	else {
		if (ret!=ERROR_BUFFER_OVERFLOW || dwBufLen==0)
			throw RLException("GetIPs#%d", 1);

		RLStream buffer(dwBufLen);

		IP_ADAPTER_INFO* pAdaptersInfo = (IP_ADAPTER_INFO*)buffer.GetBuffer();

		if (ERROR_SUCCESS != ::GetAdaptersInfo(pAdaptersInfo, &dwBufLen))
			throw RLException("GetIPs#%d", 2);

		DWORD dwCountMac = dwBufLen/sizeof(IP_ADAPTER_INFO);

		for (DWORD i=0; i<dwCountMac; i++) 
		{			
            PIP_ADDR_STRING pAddr = &pAdaptersInfo[i].IpAddressList;
            while (pAddr)
			{
				DWORD ip = ::inet_addr(pAddr->IpAddress.String);

				if (ip!=INADDR_NONE && ip!=INADDR_ANY) {
					ips.AddUINT32(ip);
				}                
                pAddr = pAddr->Next;
            }
		}
	}
}


CStringA TCP::GetIPaddresses()
{
	RLStream ips;
	TCP::GetIPaddresses(ips);

	int c = ips.GetLen()/4;

	CStringA str;

	if (c==0) {
		str = "OFF";
	}
	else {
		for (int i=0; i<c; i++) {
			in_addr ip = ((in_addr*)ips.GetBuffer())[i];
			LPCSTR ip_str = ::inet_ntoa((in_addr)ip);
			if (i>0) str += ", ";
			str += ip_str;
		}
	}
	return str;
}
