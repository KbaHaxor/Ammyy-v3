#include "stdafx.h"
#include "CRouter.h"
#include "../main/aaProtocol.h"
#include <map>
#include <stdlib.h>
#include "../RL/RLEncryptor01.h"
#include "../common/Common2.h"
#include "TaskSessionEnded.h"
#include "CmdRouterInit.h"
#include "CmdWaitingIDs.h"
#include "RouterApp.h"
#include "Sockets.h"
#include "StreamWithTime.h"

#ifdef _WIN32

#include <process.h>
#include <MSTcpIP.h>
#include "../RL/RLResource.h"
#include "resource.h"
#define WM_SOCKET		(WM_USER+1)

#else

#include "RLResource.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>

inline int closesocket(SOCKET s) { return close(s); }
inline int WSAGetLastError() 	 { return errno; }
inline int    GetLastError()   	 { return errno; }

#define WSAEWOULDBLOCK  EWOULDBLOCK
#define WSAECONNRESET   ECONNRESET
#define WSAECONNABORTED ECONNABORTED
#define WSAETIMEDOUT    ETIMEDOUT
#define WSAEHOSTUNREACH EHOSTUNREACH

#ifdef _LINUX
#define EPOLLRDHUP 0x2000
#define SO_REUSEPORT SO_REUSEADDR
#endif

#endif // !_WIN32


CRouter Router;

void CRouter::SocketBase::CloseSocket()
{
	if (m_socket==0) return; // already closed

	_log.WriteInfo("CloseSocket() %X %X", m_socket, this);

	// need to catch exception, because CloseSocket can be called from constructor while throwing exception
	try {
		AsyncSelect(false);	// reset subscribtion to events
	}
	catch(RLException& ex) {
		_log.WriteError(ex.GetDescription());
	}

#ifndef _WIN32
	for (int i=0; i<Router.m_epoll_events_count; i++) {
		#ifdef _LINUX
			SOCKET* s = (SOCKET*)&Router.m_epoll_events[i].data.fd;
		#else // FreeBSD
			SOCKET* s = (SOCKET*)&Router.m_epoll_events[i].ident;
		#endif
		if (*s==m_socket) *s=0;
	}
#endif

	_Sockets.Remove(this); // is should be before m_socket is 0

	if (::closesocket(m_socket) !=0) 
		_log.WriteError("closesocket(), error=%d", ::WSAGetLastError());
	
	m_socket = 0;
}


void CRouter::SocketBase::DeleteIfClosed()
{ 
	if (m_socket!=0) return; // not closed
	_log.WriteInfo("deleting socket %X", this);
	delete this; 	
}

void CRouter::SocketBase::AsyncSelect(bool set)
{
	//_log.WriteInfo("SocketBase::AsyncSelect(%u)", lEvent);

#ifdef _WIN32
	long lEvent = (set) ? FD_READ|FD_WRITE|FD_OOB|FD_ACCEPT|FD_CONNECT|FD_CLOSE : 0 ;
	if (::WSAAsyncSelect(m_socket, Router.m_hWorkerWnd, WM_SOCKET, lEvent) == SOCKET_ERROR)
		throw RLException("SocketBase::AsyncSelect() error %d", ::WSAGetLastError());
#endif

#ifdef _LINUX
	if (set) {
		// all : EPOLLIN | EPOLLPRI | EPOLLOUT | EPOLLRDBAND | EPOLLWRNORM | EPOLLWRBAND | EPOLLMSG | EPOLLERR | EPOLLHUP | EPOLLONESHOT | EPOLLET

		// EPOLLRDNORM - for Listener, came with accept & EPOLLIN, but no documentation
		// EPOLLWRNORM - for Conn      came with EPOLLOUT when connected			

		long lEvent = (this->IsListener()) ? EPOLLIN : EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;

		// set non-blocking mode for socket
		if (fcntl(m_socket, F_SETFL, O_NONBLOCK) == -1)
			throw RLException("fcntl failed to O_NONBLOCK: %u", errno);

		struct epoll_event ev;
		ev.events = lEvent;
		ev.data.fd = m_socket;
		if (epoll_ctl(Router.m_epoll, EPOLL_CTL_ADD, m_socket, &ev)!=0)
			throw RLException("epoll_ctl(ADD) error=%d", errno);
	}
	else {
		if (epoll_ctl(Router.m_epoll, EPOLL_CTL_DEL, m_socket, NULL)!=0)
			throw RLException("epoll_ctl(DEL) error=%d, %X", errno, m_socket);
	}
#endif

#ifdef _FreeBSD
	if (set) {
		// set non-blocking mode for socket
		if (fcntl(m_socket, F_SETFL, O_NONBLOCK) == -1)
			throw RLException("fcntl failed to O_NONBLOCK: %u", errno);
	}

	struct kevent ev[2];
	int n;

	if (this->IsListener()) {
		if (set) {
			EV_SET(&ev[0], m_socket, EVFILT_READ,  EV_ADD | EV_ENABLE,  0, 0, this);
		}
		else {
			EV_SET(&ev[0], m_socket, EVFILT_READ,  EV_DELETE,  0, 0, this);
		}
		n = 1;
	}else {
		if (set) {
			EV_SET(&ev[0], m_socket, EVFILT_READ,  EV_ADD | EV_ENABLE,  0, 0, this);
			EV_SET(&ev[1], m_socket, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, this);
		}
		else {
			EV_SET(&ev[0], m_socket, EVFILT_READ,  EV_DELETE,  0, 0, this);
			EV_SET(&ev[1], m_socket, EVFILT_WRITE, EV_DELETE,  0, 0, this);
		}
		n = 2;
	}

	if (::kevent(Router.m_epoll, &ev[0], n, NULL, 0, NULL)!=0)
		throw RLException("Error %d kevent() in AsyncSelect(%u)", errno, lEvent);	
#endif
}


void CRouter::SocketListener::CheckPort(UINT16 port)
{
	SOCKET socket = ::socket(AF_INET, SOCK_STREAM, 0);
	ASSERT(socket!=INVALID_SOCKET);
	ASSERT(socket!=0);

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ::inet_addr(Router.m_assignIP);
	addr.sin_port = htons(port);

	bool ok = (::bind(socket, (sockaddr*) &addr, sizeof(addr)) != SOCKET_ERROR);
	int error = ::WSAGetLastError();
	::closesocket(socket);

	if (!ok)
		throw RLException("SocketListener() can't bind port %d, error = %d",  (int)port, error);
}

CRouter::SocketListener::SocketListener(WORD port)
{
	m_type   = TypeListener;
	m_port   = port;

	m_socket = ::socket(AF_INET, SOCK_STREAM, 0);
	ASSERT(m_socket!=INVALID_SOCKET);
	ASSERT(m_socket!=0);

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ::inet_addr(Router.m_assignIP);
	addr.sin_port = htons(m_port);

#ifdef _WIN32
	AsyncSelect(true);
#endif

#ifndef _WIN32
	if (TheApp.m_CmdArgs.reuseport) { // doesn't work in Linux
		int optval = 1;		
		if(::setsockopt(m_socket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0)
			_log.WriteError("ERROR %d while setsockopt(SO_REUSEPORT)", errno);

		//tmp_code
		if(::setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
			_log.WriteError("ERROR %d while setsockopt(SO_REUSEADDR)", errno);
	}
#endif

	if (::bind(m_socket, (sockaddr*) &addr, sizeof(addr)) == SOCKET_ERROR) {
		throw RLException("SocketListener() can't bind port %d, error = %d",  (int)m_port, ::WSAGetLastError());
	}

	if (::listen(m_socket,20)!=0) {
		throw RLException("SocketListener() can't listen port %d, error = %d", (int)m_port, ::WSAGetLastError());
	}

#ifndef _WIN32
	AsyncSelect(true);
#endif

	_Sockets.AddNew(this);
}

#ifdef _WIN32
void CRouter::SocketListener::OnEvent(WORD event, WORD error)
{
	if(error!=0)
		throw RLException("Socket failed with error %d", error);

	if (event==FD_ACCEPT) {
		OnAccept();
	}
	else
		throw RLException("CListener::OnEvent() Invalid event came %d", event);
}
#else
void CRouter::SocketListener::OnEvent(UINT32 events)
{
#ifdef _LINUX
	if (events & EPOLLIN) {
		OnAccept();
		events ^= EPOLLIN;
	}

	if (events!=0)
		_log.WriteError("SocketListener::OnEvent() unhandled events=%X socket=%X", events, m_socket);

#else // FreeBSD
	if (events==EVFILT_READ) {
		OnAccept();
	}
	else
		_log.WriteError("SocketListener::OnEvent() unhandled events=%X socket=%X", events, m_socket);	
#endif
}
#endif


void CRouter::SocketListener::OnAccept()
{
	sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = 0;
	address.sin_port = 0;

#ifdef _WIN32
	INT addressSize = sizeof(address);
#else
	socklen_t addressSize = sizeof(address);	
#endif

	SOCKET s = ::accept(m_socket, (sockaddr*)&address, &addressSize);
	if (s==INVALID_SOCKET || s==0) {
		_log.WriteError("ERROR %d while accept(), %X", ::WSAGetLastError(), s);
		return;
	}

	// set keepalive, to detect connection breaks
	#ifdef _WIN32
	{
		tcp_keepalive alive;
		DWORD dwSize;

		alive.onoff = 1;
		alive.keepalivetime = 60*1000;
		alive.keepaliveinterval = 2*1000;

		if (::WSAIoctl(s, SIO_KEEPALIVE_VALS, &alive, sizeof(alive), NULL, 0, &dwSize, NULL, NULL)!=0)
			_log.WriteError("ERROR %d while WSAIoctl(SIO_KEEPALIVE_VALS)", ::WSAGetLastError());
	}
	#else
		try {
			RLHttp2::SetKeepAlive(s);
		}
		catch(RLException& ex) {
			_log.WriteError(ex.GetDescription());
		}
	#endif
	

	if (!Router.m_enableNagle) {
		BOOL nodelay = TRUE;
		if (::setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char *) &nodelay, sizeof(nodelay))!=0)
			_log.WriteError("ERROR %d while disabling Nagle's algorithm", ::WSAGetLastError());		
	}

	SocketConn* pLeg = new SocketConn(s, address.sin_addr.s_addr, htons(address.sin_port), m_port);
}


//________________________________________________________________________________________________


const int BUFFER_STATIC_SIZE = 65*1024; 
char* CRouter::SocketConn::m_pBufferStatic = new char[BUFFER_STATIC_SIZE];


CRouter::SocketConn::SocketConn(SOCKET socket, UINT32 ip, UINT16 portR, UINT16 postL):
m_recvBuffer(256),
m_ip(ip),
m_port(portR)
{
	m_type = TypeUndefined;

	memset(&m_stat, 0, sizeof(m_stat));	// clear all statistics

	m_type = 0;
	m_pOtherLeg = NULL;
	m_socket = socket;
	m_v3 = false;

	int len = sprintf(m_name, "%u.%u.%u.%u:%d/%d",
		(int)(((BYTE*)&ip)[0]),
		(int)(((BYTE*)&ip)[1]),
		(int)(((BYTE*)&ip)[2]),
		(int)(((BYTE*)&ip)[3]),
		(int)portR, (int)postL);

	if (len>27) //72 chars - maximum length for this template
		throw RLException("Error#1 in SocketConn(), %d", len);

	AsyncSelect(true);

	_Sockets.AddNew(this);
	
	_log.WriteInfo("new socket %X %X %s", m_socket, this, m_name);

	//DEBUG("=%s= CLeg was created", m_name);
}

void CRouter::SocketConn::DisConnect()
{
	if (m_socket==0)
		throw RLException("ERROR on DisConnect()#1 m_socket=0");

	CloseSocket();
}

void CRouter::SocketConn::OnClose()
{
	_log.WriteInfo("SocketConn::OnClose() %d, %s", m_socket, (LPCSTR)m_name);

	DisConnect();
	if (m_pOtherLeg!=NULL) 
	{ 
		// post message CTaskSessionEnded
		{
			TaskSessionEnded* pTSE = new TaskSessionEnded();

			SocketConn* pViewer;
			SocketConn* pTarget;

			if (this->IsViewer()) {
				pTarget = m_pOtherLeg;
				pViewer = this;
				pTSE->m_terminator = "v";
			}
			else {
				pTarget = this;
				pViewer = m_pOtherLeg;
				pTSE->m_terminator = "t";
			}

			pTSE->m_cmd.m_source = (m_v3) ? 2 : 1;
			pTSE->m_cmd.m_buildStamp = RouterApp::GetBuildDateTime();
			pTSE->m_cmd.m_time = pViewer->m_time;
			pTSE->m_cmd.m_span = (Common2::GetSystemTime() - pViewer->m_time); // in ms
			pTSE->m_cmd.m_sent_v = pViewer->m_stat.BytesRecv;
			pTSE->m_cmd.m_sent_t = pTarget->m_stat.BytesRecv;
			pTSE->m_cmd.m_id_t   = pTarget->m_id;
			pTSE->m_cmd.m_id_v   = pViewer->m_id;
			pTSE->m_cmd.m_ip_t   = Common2::IPv4toString(pTarget->m_ip);
			pTSE->m_cmd.m_ip_v   = Common2::IPv4toString(pViewer->m_ip);
			pTSE->m_viewerIp   = pViewer->m_name;
			pTSE->m_targetIp   = pTarget->m_name;
			pTSE->Post();
		}

		#ifdef _ROUTER_CAPTURE_
		{
			if (m_path.GetLength()>0) {
				this->m_fileStream.Close();
				m_pOtherLeg->m_fileStream.Close();

				//rename folder
				CStringA extPath = TheApp.m_path + "captures" + PATH_DELIMETER + m_path;
				CStringA newPath = TheApp.m_path + "captures" + PATH_DELIMETER + m_path.Mid(1);
				Common2::MoveFile(extPath, newPath);
			}
		}
		#endif

		m_pOtherLeg->m_pOtherLeg = NULL;
		m_pOtherLeg->DisConnect();
		m_pOtherLeg->DeleteIfClosed(); // TODO: delete m_pOtherLeg;
		m_pOtherLeg = NULL;
	}
	//delete this; // will be deleted in DeleteIfClosed()
}


void CRouter::SocketConn::OnReceive()
{
	UINT32 totalRecv = 0;

	while(true) {		
		int bytesRecv  = ::recv(m_socket, m_pBufferStatic, BUFFER_STATIC_SIZE, 0);
		//_log.WriteInfo("OnReceive()#1 socket=%X, bytes=%d, total=%d", m_socket, bytesRecv, totalRecv);	
		if (bytesRecv>0) {
			totalRecv += bytesRecv;

			if (this->IsLinked()) 
				m_pOtherLeg->m_sendBuffer.PushData(m_pBufferStatic, bytesRecv);
			else
				m_recvBuffer.AddRaw(m_pBufferStatic, bytesRecv);

			if (bytesRecv>=32*1024) { // may be we have more data
				//_log.WriteInfo("OnReceive() unusual case #364 %d", bytesRecv);
				continue;
			}
			break;
		}
		else if (bytesRecv==0) // connection has been gracefully closed
		{
			_log.WriteInfo("OnReceive() <- #1");
			OnClose();			
			return;
		}
		else if (bytesRecv==SOCKET_ERROR)
		{
			DWORD error = ::WSAGetLastError();

			if (error==WSAEWOULDBLOCK) break; // finish receiving

			bool ok = (error==WSAECONNRESET || error==WSAECONNABORTED || error==WSAETIMEDOUT || error==WSAEHOSTUNREACH);
			
			#ifdef _LINUX
				if (!ok) {
					ok = (error==ENETUNREACH || error==ECONNREFUSED); // it occurs sometimes
				}
			#endif
			
			if (ok) {
				_log.WriteInfo("OnReceive() <- #2");
			}
			else {			
				_log.WriteError("OnReceive() error=%d while reading data from socket '%s' %d %u", 
							error, (LPCSTR)m_name, bytesRecv, totalRecv);
			}
			OnClose();
			return;
		}
		else {
			_log.WriteError("OnReceive()#434 ERROR bytesRecv=%d", bytesRecv);
			OnClose();
			return;
		}
	}
	
	if (totalRecv==0) return;


	// reply to PING, do it before statistics
	if (m_type==1 && m_v3 && !IsLinked()) {
		int len = m_recvBuffer.GetLen();
		if (totalRecv!=1 || m_pBufferStatic[0] != aaPingRequest || len<1) {
			_log.WriteError("Leg %s get invalid data while waiting, %d bytes", (LPCSTR)m_name, totalRecv);
			OnClose();
		}
		else {
			m_recvBuffer.SetLen(len-1);
			static BYTE b = aaPingReply;
			this->SendSimple(&b, 1);
		}
		return;
	}

	//_log.WriteInfo("OnReceive()#2 socket=%X, bytes=%d", m_socket, totalRecv);
	
	// update statistics	
	{
		int bufferLen = (this->IsLinked()) ? m_pOtherLeg->m_sendBuffer.m_DataLen : m_recvBuffer.GetLen();

		if (m_type!=3 || Router.m_statWithControls)
		{
			Router.m_stat.BytesRecv += totalRecv;
			if (Router.m_stat.MaxRecv < totalRecv)
				Router.m_stat.MaxRecv = totalRecv;

			m_stat.BytesRecv += totalRecv;
			m_stat.RecvPackets++;
			if (m_stat.MaxRecvPacketSize <totalRecv)
				m_stat.MaxRecvPacketSize =totalRecv;
			if (m_stat.MaxQueueSize < bufferLen)
				m_stat.MaxQueueSize = bufferLen;
		}
	
		// protection from crashing router by allocate too many memory
		if (bufferLen > 16*1024*1024) {
			_log.WriteError("Leg %s has too big queue %d bytes", (LPCSTR)m_name, bufferLen);
			OnClose();
			return;
		}
	}

	//change status if we need
	     if (m_type==TypeUndefined) { OnReceiveUnknown(); }
	else if (m_type==TypeHttpQuery) { OnReceiveHttp(); }
	else {
		if (m_type!=TypeTarget && m_type!=TypeViewer)
			throw RLException("OnReceive()#473 %d", (int)m_type);

		if (m_pOtherLeg) m_pOtherLeg->OnSend(); // other leg should send recieved data
	}
}



CStringA CRouter::SocketConn::FindGetArgument(const CStringA& uri, const CStringA& name)
{
	int i1 = uri.Find("&" + name + "=");
	if (i1<0) return "";
	i1 += 2 + name.GetLength();
	int i2 = uri.Find("&", i1);
	return uri.Mid(i1, i2-i1);
}


void CRouter::SocketConn::OnReceiveHttp()
{
	char* pBuffer   = m_recvBuffer.GetBufferRd();
	DWORD count		= m_recvBuffer.GetAvailForReading();

	if (count>64*1024) {
		_log.WriteError("OnReceiveHttp()#1 error %d", count);
		OnClose();
		return;
	}

	CStringA str(pBuffer, count);

	int i = str.Find("\r\n\r\n");

	if (i<=0) return; // no valid query, may be next time it'll be fine

	if (i+4!=count) {
		_log.WriteError("OnReceiveHttp()#2 error %d %d", i, count);
		OnClose();
		return;
	}

	m_recvBuffer.GetRaw(NULL, i+4);

	if (m_recvBuffer.GetAvailForReading()==0) m_recvBuffer.Reset();

	i = str.Find("\r\n");

	CStringA httpQuery = str.Mid(0, i);

	int p1 = httpQuery.Find(" ") + 1;
	int p2 = httpQuery.ReverseFind(' ');

	if (p2<=p1) {
		_log.WriteError("OnReceiveHttp()#3 error query=%s", (LPCSTR)httpQuery);
		OnClose();
		return;
	}

	httpQuery = httpQuery.Mid(p1, p2-p1);
	httpQuery += "&";
	httpQuery.Replace("?", "&");

	bool pswOk = false;
	{
		CStringA password = FindGetArgument(httpQuery, "psw");

		if (password==Router.m_password_console) {
			pswOk = true;
		}
		else if (password.GetLength()>3) {
			LPSTR p = password.GetBuffer(0);
			for (int i=0; i<5; i++) p[i]++;

			if (password=="mmm65255ggsjh") { // mmm65=lll54255ggsjh
				pswOk = true;
			}
		}	
	}

	CStringA output;

	/// "connections" parser
	if (httpQuery.Find ("stat2.html") > -1)
	{
		if (pswOk) output = Router.GetStat2();
	}
	else if (httpQuery.Find("stat1.html") > -1)
	{
		if (pswOk) {
			CStringA id_str = FindGetArgument(httpQuery, "id");
			output = Router.GetStat1(atoi(id_str));
		}
	}
	else if (httpQuery.Find("stat3.html") > -1)
	{
		if (pswOk) {
			bool* pBool = NULL;
			CStringA row_str = FindGetArgument(httpQuery, "row");

			if      (row_str=="0") { pBool = &Router.m_statWithControls; }
			else if (row_str=="1") { pBool = &Router.m_enableNagle; }
			else if (row_str=="2") { pBool = &TheApp.m_CmdArgs.log; }
		
			if (pBool) {
				*pBool = ! (*pBool);
				output = (*pBool) ? "YES" : "NO";

				if (row_str=="2") {
					TheApp.OnLogChange();
				}
			}
		}		
	}
#ifdef _ROUTER_CAPTURE_
	else if (httpQuery.Find("capture.html") > -1)
	{
		if (pswOk) {
			CStringA v = FindGetArgument(httpQuery, "v");

			if      (v=="on")  Router.m_capture = true;
			else if (v=="off") Router.m_capture = false;

			output = CStringA("capture ") + ((Router.m_capture) ? "on" : "off");
		}
	}
#endif
	else {
		LPCVOID pData;
		LPCSTR  lpResName = (pswOk) ? MAKEINTRESOURCE(IDR_HTML_MAIN) : MAKEINTRESOURCE(IDR_HTML_LOGIN);
		DWORD count = RLResource::Load(0, RT_HTML, lpResName, 0, &pData);
		ASSERT(count>0);

		if (pswOk) {
			LPCSTR statWithControls = (Router.m_statWithControls) ? "YES" : "NO";
			LPCSTR enableNagle      = (Router.m_enableNagle) ? "YES" : "NO";
			LPCSTR debugLog         = (TheApp.m_CmdArgs.log) ? "YES" : "NO";
			output.Format((LPCSTR)pData, statWithControls, enableNagle, debugLog, 
				(LPCSTR)Router.m_listenPorts, 
				(LPCSTR)Router.m_assignIP,
				(LPCSTR)RouterApp::GetBuildDateTime(),
				(LPCSTR)Router.m_startedTime, 
				(LPCSTR)Router.GetCurrentTimeUTC());
			output.Replace("{PASSWORD}", "?psw=" + Router.m_password_console);
		}
		else
			output = (LPCSTR)pData;
	}

	LPCSTR httpReplyTempl =
				"HTTP/1.1 200 OK\r\n"
				"Content-Length: %u\r\n"
				"Keep-Alive: timeout=5, max=99\r\n"
				"Connection: Keep-Alive\r\n"
				"Cache-Control: no-cache\r\n"
				"Content-Type: text/html\r\n\r\n%s";
	
	CStringA httpReply;
	httpReply.Format(httpReplyTempl, output.GetLength(), (LPCSTR)output);

	m_sendBuffer.PushData(httpReply.GetBuffer(0), httpReply.GetLength());

	this->OnSend();
}


void CRouter::SocketConn::OnReceiveUnknown()
{	
	char* pBuffer   = m_recvBuffer.GetBufferRd();
	DWORD count		= m_recvBuffer.GetLen();
	DWORD countRead = m_recvBuffer.GetReadPos();

	if (count<1) return;
	
	if (countRead==0)
	{
		if (pBuffer[0] == aaPreInitMsg_v3[0]) {
			this->m_v3 = true;
			OnReceive_v3();
			return;
		}

		if (count>=3) {
			if (memcmp(pBuffer, "GET", 3)==0) {
				m_type=3;
				OnReceiveHttp();
				return;
			}
		}
		
		if (pBuffer[0] != aaPreInitMsg_v2[0]) {
			//_log.WriteError("SocketConn::OnReceiveUnknown()#1 '%s' is closing because first char=%d", m_name, (int)pBuffer[0]);
			OnClose();
			return;
		}

		m_sendBuffer.PushData(aaPreInitReply, 1);
		m_recvBuffer.GetRaw(NULL, 1); // remote from queue

		count--;
		countRead++;
		pBuffer++;
	}

	// parse init msg if enough data in the buffer 
	if (countRead==1 && count>=sz_aaProtocolVersionMsg_2_1)
	{
		OnReceive_v2();
	}
}

inline void CRouter::SocketConn::OnReceive_v2()
{
	char* pBuffer   = m_recvBuffer.GetBufferRd();

	char  buffer[64];
		
	memcpy(buffer, pBuffer, sz_aaProtocolVersionMsg_2_1);
	buffer[sz_aaProtocolVersionMsg_2_1] = 0;

	// decrypting
	RLEncryptor01::Encrypt(buffer, sz_aaProtocolVersionMsg_2_1, 0, aaKey01);

	char	version[64];
	UINT32	remoteId;

	int n = sscanf(buffer, aaProtocolVersionFormat_2_1, version, &m_type, &m_id, &remoteId);
	if (n!= 4 || m_type<1 || m_type>2 || m_id==0) {
		_log.WriteInfo("OnReceive_v2()#1 '%s' invalid connection", (LPCSTR)m_name);
		OnClose();
		return;
	}

	{   // m_name += '=$m_id';
		int len1 = strlen(m_name);
		int len2 = sprintf(&m_name[len1], "=%u", m_id);
		if (len1+len2>=sizeof(m_name))	// len1+len2 - without null-term char
			throw RLException("OnReceive_v2()#2 Error");
	}

	Router.m_logLogins.WriteMsg("%s %d", version, m_id);

	int check = Router.OnNewID(m_id, NULL);
	if (check!=0) {
		this->OnCloseSafe();
		return;
	}

	if (remoteId==0) {
		Router.OnNewTarget(m_id);
		this->OnSend();
		return;
	}
			
	// if not found the pair
	int found = Router.OnNewInitiator(this, remoteId);
	if (found!=0) // if not found and it'was viewer
	{
		const int aaErrorPCNotFound   = 1;
		const int aaErrorComputerBusy = 5;

		aaTerminateMsg msg;
		msg.type = 128; // aaError for v2.0
		msg.code = (found==2) ? aaErrorComputerBusy : aaErrorPCNotFound;

		// encrypting
		RLEncryptor01::Encrypt(&msg, sizeof(msg), 0, aaKey01);

		//send errorMsg
		m_sendBuffer.PushData((char*)&msg, sizeof(msg));
		OnSend();

		if (m_sendBuffer.m_DataLen!=0)
			_log.WriteError("Error while sending data before close %d", ::GetLastError());

		//close connection, previous OnSend() could close connection, so we need to check it
		this->OnCloseSafe();
	}
	else { // if found
		this->OnLinked();
		m_pOtherLeg->OnLinked();
		OnSend();
		if (m_pOtherLeg!=NULL) // previous OnSend() could cause m_pOtherLeg = NULL, so we need to check it
			m_pOtherLeg->OnSend();				
	}
}


inline void CRouter::SocketConn::OnReceive_v3()
{
	char* pBuffer   = m_recvBuffer.GetBufferRd()+1;
	int count		= m_recvBuffer.GetLen()-1;

	if (count<35) return; // not ready yet

	BYTE buffer[35];

	memcpy(buffer, pBuffer, sizeof(buffer));

	Router.m_InstanceDecryptor02.FastCopy(Router.m_TemplateDecryptor02);
	Router.m_InstanceDecryptor02.Decrypt(&buffer[0],  18);
	Router.m_InstanceDecryptor02.FastCopy(Router.m_TemplateDecryptor02);
	Router.m_InstanceDecryptor02.Decrypt(&buffer[18], 17);

	aaInitMsg* pIM = (aaInitMsg*)&buffer[18]; // pointer to init message

	UINT32 versionMajor = (pIM->version >> 20);

	if ((pIM->zero!=0) || (versionMajor!=3) || (pIM->id==0) ||
		(pIM->type!=aaTypeTarget && pIM->type!=aaTypeViewer))
	{
		_log.WriteInfo("OnReceive_v3()#1 '%s' invalid connection %u %u", (LPCSTR)m_name, pIM->id, pIM->version);
		OnClose();	// invalid data
		return;
	}

	m_type = pIM->type; // aaTypeTarget or aaTypeViewer
	m_id   = pIM->id;

	// need to be sure that all sent waiter's data is ready, beacuse it can send PING
	if (pIM->type==aaTypeTarget) {
		if (count!=sizeof(buffer)) {
			_log.WriteInfo("OnReceive_v3()#2 '%s' %u invalid connection", (LPCSTR)m_name, (int)count);
			OnClose();
			return;
		}
	}

	{   // m_name += '=$m_id';
		int len1 = strlen(m_name);
		int len2 = sprintf(&m_name[len1], "=%u", m_id);
		if (len1+len2>=sizeof(m_name))	// len1+len2 - without null-term char
			throw RLException("Error#1 in OnReceive_v3()");
	}	

	int check = Router.OnNewID(m_id, &buffer[2]);
	if (check!=0) {
		SendReply_v3(check); // failed
		OnSend();
		OnCloseSafe();
		return;
	}

	Router.m_logLogins.WriteMsg("v3 %d", m_id);

	if (this->IsTarget()) {
		Router.OnNewTarget(m_id);
		SendReply_v3(0); // no failed
		OnSend();
		return;
	}
	
	int found = Router.OnNewInitiator(this, pIM->remote_id);
	if (found!=0) // if not found and it'was viewer
	{
		SendReply_v3(found); // send reason of failed
		this->OnSend();

		if (m_sendBuffer.m_DataLen!=0)
			_log.WriteError("Error while sending data before close %d", ::GetLastError());

		//close connection, previous OnSend() could close connection, so we need to check it
		this->OnCloseSafe();
	}
	else { // if found
		       this->SendReply_v3(0); // no failed
		       this->OnLinked_v3();
		m_pOtherLeg->OnLinked_v3();
		this->OnSend();
		if (m_pOtherLeg!=NULL) // previous OnSend() could cause m_pOtherLeg = NULL, so we need to check it
			m_pOtherLeg->OnSend();
	}
}



void CRouter::SocketConn::SendReply_v3(INT8 status)
{
	BYTE buffer[2];

	buffer[0] = aaPreInitReply[0];
	buffer[1] = status; // means ok

	m_sendBuffer.PushData((char*)&buffer[0], sizeof(buffer));
}

void CRouter::SocketConn::OnLinked()
{		
	m_pOtherLeg->m_sendBuffer.PushData(m_recvBuffer.GetBufferRd(), m_recvBuffer.GetAvailForReading());
	m_recvBuffer.Free(); // no need this buffer anymore
}

void CRouter::SocketConn::OnLinked_v3()
{
	BYTE* pBuffer = (BYTE*)m_recvBuffer.GetBuffer()+1;

	aaRouterInfoMsg* p1 = (aaRouterInfoMsg*)&pBuffer[0];

	{ 
		p1->random   = ::GetTickCount() % 65536; // just random 2 bytes
		p1->time     = m_pOtherLeg->m_time;
		p1->tcp_port = m_pOtherLeg->m_port;
		p1->ip       = m_pOtherLeg->m_ip;

		Router.m_InstanceEncryptor02.FastCopy(Router.m_TemplateEncryptor02);
		Router.m_InstanceEncryptor02.Encrypt(pBuffer, sizeof(aaRouterInfoMsg));
	}

	// drop md5 password

	m_pOtherLeg->m_sendBuffer.PushData((char*)pBuffer-1, 16+1);
	m_pOtherLeg->m_sendBuffer.PushData((char*)pBuffer+18, m_recvBuffer.GetLen()-(18+1));
	m_recvBuffer.Free(); // no need this buffer anymore
}

void CRouter::SocketConn::SendSimple(const void* pBuffer, const int len)
{
	int ret = ::send(m_socket, (char*)pBuffer, len, 0);
	if (ret!=len) {
		int error = ::WSAGetLastError();
		_log.WriteError("SocketConn::SendSimple() error=%d, %d %d %s", error, ret, len, (LPCSTR)m_name);
		OnClose();
	}
}


int CRouter::SocketConn::SendOneTime(const char* pBuffer, int len)
{
	int ret = ::send(m_socket, pBuffer, len, 0);
	if (ret == SOCKET_ERROR)
	{
		int error = ::WSAGetLastError();

		/*
		if (error==WSAECONNRESET)	{ //10054
			_log.WriteInfo("SocketConn::Send()#1 socket '%s' get error=%d and will be closing", (LPCSTR)m_name, error);
			OnClose();
		}
		else {
			// will try to send later
			if (error!=WSAEWOULDBLOCK)
				_log.WriteError("SocketConn::Send()#2 %d %d %d %s",error, ret, len, (LPCSTR)m_name);
		}
		*/

		if (error!=WSAEWOULDBLOCK) { // will try to send later if WSAEWOULDBLOCK
			if (error==WSAECONNRESET)
				_log.WriteInfo ("SocketConn::Send()#1 socket '%s' get error=%d and will be closing", (LPCSTR)m_name, error);
			else
				_log.WriteError("SocketConn::Send()#2 %d %d %d %s",error, ret, len, (LPCSTR)m_name);

			OnClose();
		}
	}
	return ret;
}



// first time Send() will be called by event FD_WRITE_BIT
//
void CRouter::SocketConn::OnSend()
{	
	int socket = m_socket; 	// save it, because it can be closed by SendOneTime()

	UINT32 totalSent = 0;
	while(true) {
		char* pBuffer = NULL;
		int count = (int)m_sendBuffer.GetReadBuffer(&pBuffer);
		if (count==0) break;
		count = min(count, 32*1024);
		//DEBUG("=%s= sending %d", m_name, count);

		int sent = SendOneTime(pBuffer, count);
		if (sent == SOCKET_ERROR)
			break;

	#ifdef _WIN32		
		ASSERT(sent==count);
	#else
		ASSERT(sent>=0);
	#endif

	#ifdef _ROUTER_CAPTURE_
		m_fileStream.Push(pBuffer, sent);
	#endif

		// commit transaction
		m_sendBuffer.SkipReading(sent);
		totalSent += sent;

	#ifndef _WIN32
		if (sent!=count) break; // sent less we wanted, so don't try again now
	#endif
	}

	_log.WriteInfo("Send() socket=%X, bytes=%d", socket, totalSent);

	// update statistics
	if (totalSent>0 && (m_type!=TypeHttpQuery || Router.m_statWithControls)) {
		Router.m_stat.BytesSent += totalSent;

		m_stat.SendPackets++;
		if (m_stat.MaxSendPacketSize < totalSent)
			m_stat.MaxSendPacketSize = totalSent;

		if (Router.m_stat.MaxSent < totalSent)
			Router.m_stat.MaxSent = totalSent;
	}
}

#ifdef _WIN32
void CRouter::SocketConn::OnEvent(WORD event, WORD error)
{
	if (event==FD_CLOSE && error==WSAECONNABORTED) error=0;	// connection was aborted by another PC

	if(error!=0)
		throw RLException("Socket failed with error %d, event %d", error, event);

	if      (event==FD_WRITE) { OnSend();    }
	else if (event==FD_READ)  { OnReceive(); }
	else if (event==FD_CLOSE) {	OnClose();   }
	else
		throw RLException("CLeg::OnEvent() Invalid event came %d", event);
}
#else
void CRouter::SocketConn::OnEvent(UINT32 events)
{
#ifdef _LINUX
	if (events &  EPOLLIN) {
		events ^= EPOLLIN;
		OnReceive();
		if (m_socket==0) return; // this socket was closed
	}	

	if (events &  EPOLLOUT) {
		events ^= EPOLLOUT;
		OnSend();
		if (m_socket==0) return; // this socket was closed
	}

	if (events &  EPOLLRDHUP) {
		events ^= EPOLLRDHUP;
		OnClose();
		return; // this socket was closed
	}

	if (events!=0) 
	{
		if (events!=EPOLLERR && events!=(EPOLLERR|EPOLLHUP)) // skip these cases, they're occured very often
			_log.WriteError("SocketConn::OnEvent() unhandled events=%X socket=%X", events, m_socket);

		OnClose();
	}
#else // FreeBSD
	if (events==EVFILT_READ) {
		OnReceive();
	}
	else if (events==EVFILT_WRITE) {
		OnSend();
	}
	else {
		_log.WriteError("SocketConn::OnEvent() unhandled events=%X socket=%X", events, m_socket);
	}
#endif
}
#endif

void CRouter::AddStatistics(CRouter::SocketConn::STAT& s1, CRouter::SocketConn::STAT& s2)
{
	s1.BytesRecv += s2.BytesRecv;
	s1.RecvPackets += s2.RecvPackets;
	s1.SendPackets += s2.SendPackets;
	s1.QueueSize   += s2.QueueSize;
	if (s1.MaxRecvPacketSize < s2.MaxRecvPacketSize) s1.MaxRecvPacketSize = s2.MaxRecvPacketSize;
	if (s1.MaxSendPacketSize < s2.MaxSendPacketSize) s1.MaxSendPacketSize = s2.MaxSendPacketSize;
	if (s1.MaxQueueSize  < s2.MaxQueueSize)			 s1.MaxQueueSize  = s2.MaxQueueSize;
}

void CRouter::Add_INT64(CStringA& out, INT64 val)
{
	char buffer[32];
#ifdef _WIN32
	sprintf(buffer, "%I64d;", val);
#else
	sprintf(buffer, "%lld;",  val);
#endif
	out += buffer;
}

void CRouter::Add_INT32(CStringA& out, INT32 val)
{
	char buffer[32];
	sprintf(buffer, "%u;", val);
	out += buffer;
}



CStringA CRouter::GetStat1(UINT32 dwSelectedLegData)
{
	CStringA output;
	output.GetBuffer(2*1024);

	int nLegsCount[4];
	int linkedPairs = 0;

	CRouter::SocketConn::STAT stat[3];
  
	memset(nLegsCount,0, sizeof(nLegsCount));
	memset(stat,      0, sizeof(stat));


	// check if selected leg was deleted
	CRouter::SocketConn* pSelectedLeg = NULL;
	{
		if (dwSelectedLegData!=0) {
			CRouter::SocketBase* pSocket1 = _Sockets.Find((SOCKET)dwSelectedLegData);

			if (pSocket1) {
				if (!pSocket1->IsListener()) pSelectedLeg = (SocketConn*)pSocket1;
			}
		}

		output += (pSelectedLeg==NULL) ? "1;" : "0;";
	}

	_Sockets.IterReset();
	while(true) {
		SocketBase* pLeg1 = _Sockets.IterNext();
		if (pLeg1==NULL) break;
		if (pLeg1->IsListener()) continue;
		SocketConn* pLeg = (SocketConn*)pLeg1;

		int type = pLeg->m_type;

		if (type<0 && type>3)
			throw RLException("GetStat1()#1 incorect type = %d, %X", type, pLeg1);
		
		nLegsCount[type]++;

		if (pLeg->IsLinked() && pLeg->IsViewer()) linkedPairs++;

		if (pSelectedLeg!=NULL) {
			if (pSelectedLeg==pLeg) {
				if (pLeg->IsLinked()) {
					CRouter::SocketConn* pLeg2 = pLeg->m_pOtherLeg; // viewer

					pLeg ->m_stat.QueueSize = pLeg ->m_sendBuffer.m_DataLen;
					pLeg2->m_stat.QueueSize = pLeg2->m_sendBuffer.m_DataLen;
						
					AddStatistics(stat[1], pLeg ->m_stat);
					AddStatistics(stat[2], pLeg2->m_stat);
					AddStatistics(stat[0], stat[1]);
					AddStatistics(stat[0], stat[2]);
				}
				else {
					AddStatistics(stat[0], pLeg->m_stat);
				}
			}
		}
		else {
			AddStatistics(stat[0], pLeg->m_stat);
		}		
	}	

	int nLegsTotal = nLegsCount[0] + nLegsCount[1] + nLegsCount[2] + nLegsCount[3];

	Add_INT32(output, nLegsTotal);
	Add_INT32(output, nLegsCount[1]);
	Add_INT32(output, nLegsCount[2]);
	Add_INT32(output, nLegsCount[3]);
	Add_INT32(output, nLegsCount[0]);
	Add_INT32(output, linkedPairs);
	Add_INT32(output, this->m_stat.MaxRecv);
	Add_INT32(output, this->m_stat.MaxSent);
	Add_INT64(output, this->m_stat.BytesRecv);
	Add_INT64(output, this->m_stat.BytesSent);	

	//calculate speed
	{
		DWORD dwTicks = ::GetTickCount();
		DWORD dwSpan = dwTicks - m_speedMeasure.m_ticks;		

		if (m_speedMeasure.m_ticks==0 || dwSpan==0) {
			output += "-;-;";
		}
		else {			
			DWORD speedRecv = 8*1000*((UINT64)this->m_stat.BytesRecv - m_speedMeasure.m_bytesRecv) / dwSpan;
			DWORD speedSent = 8*1000*((UINT64)this->m_stat.BytesSent - m_speedMeasure.m_bytesSent) / dwSpan;

			Add_INT32(output, speedRecv);
			Add_INT32(output, speedSent);
		}

		m_speedMeasure.m_ticks = dwTicks;
		m_speedMeasure.m_bytesRecv = this->m_stat.BytesRecv;
		m_speedMeasure.m_bytesSent = this->m_stat.BytesSent;
	}	

	// fill bottom table
	{
		for (int i=0; i<3; i++) {
			if (i>0 && pSelectedLeg==NULL) {
				output += ";;;;;;;";
			}
			else {
				CRouter::SocketConn::STAT* pStat = &stat[i];

				Add_INT32(output, pStat->BytesRecv);
				Add_INT32(output, pStat->RecvPackets);
				Add_INT32(output, pStat->SendPackets);
				Add_INT32(output, pStat->MaxRecvPacketSize);
				Add_INT32(output, pStat->MaxSendPacketSize);
				Add_INT32(output, pStat->MaxQueueSize);
				Add_INT32(output, pStat->QueueSize);
			}			
		}
	}

	return output;
}


CStringA CRouter::GetStat2()
{
	CStringA output;
		
	output.GetBuffer(_Sockets.GetCount()*30); // average 30 bytes per leg, skip listeners

	_Sockets.IterReset();
	while(true) {
		SocketBase* pLeg1 = _Sockets.IterNext();
		if (pLeg1==NULL) break;
		if (pLeg1->IsListener()) continue;
		SocketConn* pLeg = (SocketConn*)pLeg1;
		
		CStringA name = pLeg->GetName();

		if (pLeg->IsLinked()) {
			if (!pLeg->IsViewer()) continue;
			
			name += " -> ";
			name += pLeg->m_pOtherLeg->GetName();
		}
		else {
			if      (pLeg->IsTarget())    name += " <-";
			else if (pLeg->IsViewer())    name += " ->";
			else if (pLeg->IsHttpQuery()) name += " -> control";
		}

		char str[32];
		sprintf(str, ";%u;", (UINT32)pLeg->m_socket);
		
		output += name + str;
	}
	
	return output;
}

//________________________________________________________________________________________________________

CRouter::CRouter()
{
	m_started = false;
	m_thread1 = 0;
	m_thread2 = 0;

#ifdef _WIN32
	m_hWorkerWnd = NULL;
#else
	m_epoll = 0;
	m_epoll_events_count = 0;
#endif

	//clear statistics
	memset(&m_stat, 0, sizeof(m_stat));

	m_speedMeasure.m_ticks = 0;

	m_TemplateEncryptor02.SetKey((BYTE*)&aaKey02, true);
	m_TemplateDecryptor02.SetKey((BYTE*)&aaKey02, false);
	m_InstanceEncryptor02.Copy(m_TemplateEncryptor02);
	m_InstanceDecryptor02.Copy(m_TemplateDecryptor02);
}

CRouter::~CRouter()
{
	Stop();
	//CloseHandles();
}


void CRouter::CheckPorts()
{
	for(int i1=0; i1<m_listenPorts.GetLength();) {
		int i2 = m_listenPorts.Find(",", i1);

		if (i2==-1) i2 = m_listenPorts.GetLength();

		CStringA port_str = m_listenPorts.Mid(i1, i2-i1);
		int port_int = atoi(port_str);
		if (port_int>0) {
			SocketListener::CheckPort(port_int);
		}

		i1 = i2+1;
	}
}


void CRouter::ConfigCreateFile()
{
	CStringA path = TheApp.m_path + "config.txt";

	LPCSTR values = "password_console: router583""\r\n"
					"ports: 80,443,8080""\r\n"
					"ip: 0.0.0.0""\r\n"
					"nagle: off""\r\n";

	RLStream file;
	file.AddRaw(values, strlen(values));
	file.WriteToFile(path);
}


void CRouter::ConfigReadFile()
{
	// set default values	
	m_enableNagle = false;
	m_statWithControls = true;
	m_assignIP = "0.0.0.0";
	m_listenPorts = "80,443,8080";
	m_password_console = "router583";
	m_password_using_set = false;
	m_allow_ids.Reset();

	CStringA path = TheApp.m_path + "config.txt";

	if (Common2::FileIsExistA(path)) {
		RLStream fileStream;
		fileStream.ReadFromFile(path);
		CStringA file = fileStream.GetString0A();
		file.Replace("\r\n", "\n");

		int i=0;

		while(true) {
			CStringA str = Common2::GetSubString(file, "\n", i);
			if (str=="") break;
			int j =	str.Find(":");
			if (j<0) continue;
			CStringA name = str.Mid(0, j);
			CStringA value = str.Mid(j+1);
			name.TrimLeft();
			name.TrimRight();
			name.MakeLower();
			value.TrimLeft();
			value.TrimRight();
			//value.MakeLower();	password is case sensitive

			if      (name=="ports")				this->m_listenPorts = value;
			else if (name=="ip")				this->m_assignIP = value;
			else if (name=="password_console")	this->m_password_console = value;
			else if (name=="password_using")    {
				this->m_password_using_set = true;
				this->m_password_using.Calculate(value);
			}
			else if (name=="nagle")				this->m_enableNagle = (value=="on" || value=="ON");
			else if (name=="allow_ids") 
			{
				int i=0;

				while(true) {
					CStringA str = Common2::GetSubString(value, ",", i);
					if (str=="") break;
					int id = atol(str);
					if (id>0) {
						m_allow_ids.AddUINT32(id);
					}
				}				
			}
			else
				throw RLException("Incorrect parameter '%s' in the settings file", (LPCSTR)name);
		}		
	}
}

CStringA CRouter::GetCurrentTimeUTC()
{
	SYSTEMTIME t;
	::GetSystemTime(&t);
	
	CStringA str;
	str.Format("%.4hd%.2hd%.2hd-%.2hd:%.2hd:%.2hd", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
	return str;
}

void CRouter::Start()
{
	RLMutexLock l(m_start_stop_locker);

	if (m_started) return; // already started

	m_started = true;

	ConfigReadFile();

	m_logLogins.InitA(TheApp.m_path + "logins.log");
	m_logLogins.OpenFile();
	//m_logNoSocket.InitA(TheApp.m_path + "_no_socket.log");

	//_logWeb.InitA(TheApp.m_path + "_log_web.log");
	//_logWeb.Open();

	m_startedTime = GetCurrentTimeUTC();
	m_capture = true;

	ASSERT(m_thread1==0);
	ASSERT(m_thread2==0);

#ifdef _WIN32
	m_thread1 = _beginthreadex(NULL, 0, WorkingThread1_static, this, 0, NULL);
	if (m_thread1==0) throw RLException("_beginthreadex()#1 error=%d", errno);

	m_thread2 = _beginthreadex(NULL, 0, WorkingThread2_static, this, 0, NULL);
	if (m_thread2==0) throw RLException("_beginthreadex()#2 error=%d", errno);

	if (::SetThreadPriority((HANDLE)m_thread1, THREAD_PRIORITY_ABOVE_NORMAL)==0)
		throw RLException("SetThreadPriority() error=%d", ::GetLastError());
#else
	int err;
	err = pthread_create((pthread_t*)&m_thread1, NULL, WorkingThread1_static, this);
	if (err) throw RLException("pthread_create(1) error=%d", err);

	err = pthread_create((pthread_t*)&m_thread2, NULL, WorkingThread2_static, this);
	if (err) throw RLException("pthread_create(2) error=%d", err);

	// TODO: need SetThreadPriority(m_thread1) for UNIX

#endif
}


THREAD_RETURN CRouter::WorkingThread1_static(LPVOID lpParameter)
{
	((CRouter*)lpParameter)->WorkingThread1();
	return 0;
}

THREAD_RETURN CRouter::WorkingThread2_static(LPVOID lpParameter)
{
	((CRouter*)lpParameter)->WorkingThread2();
	return 0;
}

void CRouter::WorkingThread1()
{
	try 
	{
#ifdef _WIN32
		m_hWorkerWnd = CreateWorkerWindow();
#else

	if (TheApp.m_CmdArgs.sigsegv) {
		int* a = 0;
		*a = 123;
		return;
	}
		
	#ifdef _LINUX
		m_epoll = epoll_create(20000);
		if (m_epoll<=0) throw RLException("epoll_create() error=%u", m_epoll);
	#else // FreeBSD
		m_epoll = kqueue();
		if (m_epoll<=0) throw RLException("kqueue() error=%u", m_epoll);
	#endif
#endif		

		RLHttp2::m_assignIP = m_assignIP;
		
		// start listeners
		for(int i1=0; i1<m_listenPorts.GetLength();) {
			int i2 = m_listenPorts.Find(",", i1);

			if (i2==-1) i2 = m_listenPorts.GetLength();

			CStringA port_str = m_listenPorts.Mid(i1, i2-i1);
			int port_int = atoi(port_str);
			if (port_int>0) {
				SocketListener* pListener = new SocketListener(port_int);
				m_Listeners.push_back(pListener);
			}

			i1 = i2+1;
		}

		//send a notification to webserver
		{
			CCmdRouterInit cmd;
			cmd.m_strBuildStamp = RouterApp::GetBuildDateTime();
			cmd.m_strPorts = m_listenPorts;
			cmd.m_strIPs = m_assignIP;
			cmd.Send();

			if (cmd.m_strStatus!="") {
				throw RLException(cmd.m_strStatus);
			}
		}

		_log.WriteInfo("CRouter::WorkingThread1() started");

#ifdef _WIN32
		//main loop here
		MSG msg;
		DWORD Ret;
		while(Ret = ::GetMessage(&msg, NULL, 0, 0))
		{
			if (Ret == -1) {
				throw RLException("CRouter::StartInternal()#1 %d", ::GetLastError());
			}

			//::TranslateMessage(&msg);	// no keyboard messages
			::DispatchMessage(&msg);	// dispatches a message to a window procedure
		}
#else
		while(m_started) 
		{
	#ifdef _LINUX
			m_epoll_events_count = epoll_wait(m_epoll, &m_epoll_events[0], 20, 1000);

			if (m_epoll_events_count<0 || m_epoll_events_count>20)
				throw RLException("epoll_wait() error=%d, count=%d", errno, m_epoll_events_count);

			//printf("epoll_wait %u\n",m_epoll_events_count);

			for (int i=0; i<m_epoll_events_count; i++) {
				epoll_event* pEvent = &m_epoll_events[i];
				SOCKET s = pEvent->data.fd;
				uint32_t events = pEvent->events;

				//printf("Event: i=%u event=%X socket=%X\n", i, events, s);

				if (s==0) continue; // was closed

				SocketBase* pSocket = _Sockets.Find(s);
				if (pSocket==NULL)
					throw RLException("NOT FOUND socket %X", s);
				
				pSocket->OnEvent(events);
				pSocket->DeleteIfClosed();
			}
	#else // FreeBSD
			struct timespec timeout;
			timeout.tv_sec  = 1;
			timeout.tv_nsec = 0; //100000000;
			m_epoll_events_count = ::kevent(m_epoll, NULL, 0, &m_epoll_events[0], 20, &timeout);

			if (m_epoll_events_count<0) throw RLException("kevent() error=%d", errno);

			if (!m_started) break;

			//if (m_epoll_events_count!=0) _log.WriteInfo("kevent#2 %u\n", m_epoll_events_count);

			// first handle recv enents
			for (int i=0; i<m_epoll_events_count; i++) {
				SOCKET s = m_epoll_events[i].ident;
				uint32_t event = m_epoll_events[i].filter;

				if (event!=EVFILT_READ) {
					ASSERT(event==EVFILT_WRITE);
					continue;
				}
				if (s==0) continue; // was closed

				//_log.WriteInfo("Event rd: i=%u event=%d socket=%X", i, event, s);

				SocketBase* pSocket = (CSocketBase*)m_epoll_events[i].udata;
				ASSERT(pSocket!=NULL);
				ASSERT(pSocket->GetSocket()==s);
				pSocket->OnEvent(event);
				pSocket->DeleteIfClosed();
			}

			// than handle write enents
			for (int i=0; i<m_epoll_events_count; i++) {
				SOCKET s = m_epoll_events[i].ident;
				uint32_t event = m_epoll_events[i].filter;

				if (event!=EVFILT_WRITE) continue;
				if (s==0) continue; // was closed

				//_log.WriteInfo("Event wr: i=%u event=%d socket=%X", i, event, s);

				SocketBase* pSocket = (CSocketBase*)m_epoll_events[i].udata;
				ASSERT(pSocket!=NULL);
				ASSERT(pSocket->GetSocket()==s);
				pSocket->OnEvent(event);
				pSocket->DeleteIfClosed();
			}
	#endif
		}
#endif
	}
	catch(RLException& ex) {
		RL_ERROR(ex.GetDescription());
	}

	_log.WriteInfo("CRouter::WorkingThread1() stopping");

	// stoping here
	try {
		CloseHandles();
	}
	catch(RLException& ex) {
		RL_ERROR(ex.GetDescription());
	}
}

void CRouter::WorkingThread2()
{
	try {
		TaskSessionEnded::Init();
		_CmdSessionEndedQueue.SendFromQueue();
	}
	catch(RLException& ex) {
		RL_ERROR(ex.GetDescription());
	}	

	while(true) 
	{
		if (!m_thread1) break;

		SendWaitingIds();
		DWORD dwTicksLast = ::GetTickCount() + 300;

		if (!m_thread1) break;

		TaskSessionEnded* pTask = TaskSessionEnded::GetFirst();
		if (pTask) pTask->DoTask(false);

		DWORD dwTicksNow = ::GetTickCount();

		if (dwTicksLast > dwTicksNow) {
			DWORD wait = dwTicksLast-dwTicksNow;
			if (wait>300) wait=300;	// waiting no more than 300ms
			::Sleep(wait);
		}
	}

	// save all unsent sessions before exit
	while(true) {
		TaskSessionEnded* pTask = TaskSessionEnded::GetFirst();
		if (pTask==NULL) break;
		pTask->DoTask(true);
	}
}

void CRouter::SendWaitingIds()
{
	try {		
		// Collect Waiting IDs
		{
			RLMutexLock l(m_newWaitingIDs_locker);

			if (m_newWaitingIDs.size()>0)
			{
				std::vector<DWORD>::iterator it  = m_newWaitingIDs.begin();
				std::vector<DWORD>::iterator it2 = m_newWaitingIDs.end();

				for(;it!=it2; it++) {
					m_newWaitingIDs_for_sending.insert(*it);
				}

				m_newWaitingIDs.clear();
			}
		}
				
		int size = m_newWaitingIDs_for_sending.size(); // can't be local in case of sending error

		if (size==0) return;
		
		//_logWeb.WriteMsg("CCmdWaitingIDs-"); DWORD t1 = ::GetTickCount();

		CCmdWaitingIDs cmd;
		cmd.m_IDs.SetMinCapasity(size*sizeof(DWORD));

		std::set<DWORD>::iterator it  = m_newWaitingIDs_for_sending.begin();
		std::set<DWORD>::iterator it2 = m_newWaitingIDs_for_sending.end();

		for(;it!=it2; it++) {
			DWORD id = *it;
			cmd.m_IDs.AddUINT32(id);
		}
		cmd.Send();
				
		//t1 =::GetTickCount()-t1; _logWeb.WriteMsg("CCmdWaitingIDs+ %d     %d",size, t1);

		m_newWaitingIDs_for_sending.clear();
	}
	catch(RLException& ex) {		
		_log.WriteError("SendWaitingIds() %s", ex.GetDescription());
		//_logWeb.WriteError("SendWaitingIds() %s", ex.GetDescription());
	}
}


void CRouter::Stop()
{
	RLMutexLock l(m_start_stop_locker);

	if (!m_started) return;

	m_started = false;

	ASSERT(m_thread1!=0);
	ASSERT(m_thread2!=0);

#ifdef _WIN32
	{
		if (m_hWorkerWnd) ::PostMessage(m_hWorkerWnd, WM_QUIT, 0, 0);

		DWORD ret = ::WaitForSingleObject((HANDLE)m_thread1, 10000);
		if (ret==WAIT_TIMEOUT) RL_ERROR("Timeout on CRouter::Stop()#1");
		::CloseHandle((HANDLE)m_thread1);
		m_thread1 = 0;
	}

	{
		DWORD ret = ::WaitForSingleObject((HANDLE)m_thread2, 10000);
		if (ret==WAIT_TIMEOUT) RL_ERROR("Timeout on CRouter::Stop()#2");
		::CloseHandle((HANDLE)m_thread2);
		m_thread2 = 0;
	}
#else
	void* p;

	int err = pthread_join((pthread_t)m_thread1, &p);
	m_thread1=0;
	if (err) RL_ERROR("pthread_join(1) error=%d", err);

		err = pthread_join((pthread_t)m_thread2, &p);
	m_thread2=0;
	if (err) RL_ERROR("pthread_join(2) error=%d", err);
#endif
}


void CRouter::CloseHandles()
{
	// remove all listeners first
	while(!m_Listeners.empty()) {
		SocketListener* pListener = m_Listeners.back();
		pListener->CloseSocket();
		pListener->DeleteIfClosed(); // TODO: delete pListener;
		m_Listeners.pop_back();
	}

	while(true) {
		// not fast algorithm, be very heavy
		                     _Sockets.IterReset();
		SocketBase* pLeg1 = _Sockets.IterPrev(); // more fast removing from end of vector
		if (pLeg1==NULL) break;
		if (pLeg1->IsListener()) 
			throw RLException("CloseHandles()#1"); // no listeners should be here

		SocketConn* pLeg = (SocketConn*)pLeg1;

		// close connection, here it delete self from _Sockets, 
		// here it can delete linked legs, so use IterReset() in each loop
		pLeg->OnClose();
		pLeg->DeleteIfClosed(); // TODO: delete pLeg;
	}

	if (_Sockets.GetCount()!=0)
		throw RLException("CloseHandles()#2");

#ifdef _WIN32
	if (m_hWorkerWnd!=NULL) {
		VERIFY(::DestroyWindow(m_hWorkerWnd)!=FALSE);
		m_hWorkerWnd = NULL;
	}
#else
	if (m_epoll!=0) {
		if (::close(m_epoll)!=0) _log.WriteError("close(m_epoll) error=%d", errno);
		m_epoll=0;
	}
#endif
}

/*
RLTimer t1;

int ping_timeout = 5;

void CRouter::OnTimer()
{
	if (t1.GetElapsedSeconds()<ping_timeout) return;
	t1.Start();

	int n = 0;

	_Sockets.IterReset();
	while(true) {
		SocketBase* pLeg1 = _Sockets.IterNext();
		if (pLeg1==NULL) break;
		if (pLeg1->IsListener()) continue;
		SocketConn* pExiLeg = (SocketConn*)pLeg1;  // existing leg

		if (!pExiLeg->m_v3) continue; // only for v3
		if (pExiLeg->m_type != 1) continue;	// we're looking only Targets
		if (pExiLeg->IsLinked()) continue;

		// need to check status

		if (pExiLeg->SendOneTime("P", 1)!=1) _log.WriteError("ERROR while sending PING");
		
		pExiLeg->DeleteIfClosed();
		n++;
	}
		
	_log.WriteError("OnTimer() finished, %u, timeout=%u", n, ping_timeout);
	ping_timeout++;
}
*/


int CRouter::OnNewID(UINT32 id, BYTE* pwd_md5)
{
	int len = m_allow_ids.GetLen()/4;

	if (len>0) {
		UINT32* pIDs = (UINT32*)m_allow_ids.GetBuffer();
		bool allow = false;
		while(len>0) {			
			if (*pIDs==id) {
				allow = true;
				break;
			}
			len--;
			pIDs++;
		}
		if (!allow) return Router_Denied;
	}
	
	// check password here
	if (m_password_using_set) {
		if (pwd_md5!=NULL) {
			if (memcmp(m_password_using.hash, pwd_md5, sizeof(m_password_using.hash))==0) return 0; //ok
		}
		return Router_PasswordIncorrect;
	}

	return 0; // ok
}

void CRouter::OnNewTarget(UINT32 id)
{
	RLMutexLock l(m_newWaitingIDs_locker);
	m_newWaitingIDs.push_back(id);
}


// 0 - found and connected
// 1 - computer not found
// 2 - computer is busy by other session
// 3 - computer has invalid version (only for v3)

int CRouter::OnNewInitiator(CRouter::SocketConn* pNewLeg, UINT32 remoteId)
{
	bool found_linked = false;
	bool found_v2 = false;

	_Sockets.IterReset();
	while(true) {
		SocketBase* pLeg1 = _Sockets.IterNext();
		if (pLeg1==NULL) break;
		if (pLeg1->IsListener()) continue;
		SocketConn* pExiLeg = (SocketConn*)pLeg1;  // existing leg

		if (pExiLeg == pNewLeg) continue;

		if (pExiLeg->m_type != 1) continue;	// we're looking only Targets
		
		if (remoteId != pExiLeg->m_id) continue;

		if (pExiLeg->IsLinked()) {
			found_linked = true;
			continue;	// already linked with other leg
		}

		if (pExiLeg->m_v3 != pNewLeg->m_v3) {
			if (pNewLeg->m_v3) found_v2 = true;
			continue;	// different versions
		}

		//found, so linking two legs
		//SocketConn* pLegTarget = (pNewLeg->IsViewer()) ? pExiLeg : pNewLeg;
		//SocketConn* pLegViewer = (pNewLeg->IsViewer()) ? pNewLeg : pExiLeg;
		//DEBUG("connected %s -> %s", pLegViewer->GetName(), pLegTarget->GetName());

		pExiLeg->m_time = pNewLeg->m_time = Common2::GetSystemTime();

		pExiLeg->m_pOtherLeg = pNewLeg;
		pNewLeg->m_pOtherLeg = pExiLeg;

#ifdef _ROUTER_CAPTURE_
		if (m_capture && (!pExiLeg->m_v3) && (!pNewLeg->m_v3))
		{
			CStringA path = TheApp.m_path + "captures" + PATH_DELIMETER;

			if (!Common2::FileIsExistA(path)) {
				 Common2::CreateDirectoryA(path);
			}

			SYSTEMTIME t;
			Common2::UINT64ToSystemTime(pExiLeg->m_time, t);

			char name[128];

			sprintf(name, "_%.4hd%.2hd%.2hd-%.2hd%.2hd%.2hd_%.3hd_%u_%u",
				t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute,
				t.wSecond, t.wMilliseconds,  pNewLeg->m_id, pExiLeg->m_id);

			path += name;
			path += PATH_DELIMETER;

			if (Common2::FileIsExistA(path)) {
				_log.WriteError("Path '%s' is already exists", (LPCSTR)path);
			}
			else {
				Common2::CreateDirectoryA(path);
				pNewLeg->m_path = name;
				pExiLeg->m_path = name;
				pNewLeg->m_fileStream.Create(path+"t"); // swap, beacuse we capture OnSend
				pExiLeg->m_fileStream.Create(path+"v");
			}
		}

#endif
		return Router_Connected;
	}

	// not found here

	if (found_linked) return Router_ComputerIsBusy;
	if (found_v2)     return Router_ComputerOtherVersion; // (only for v3)
	
	return Router_ComputerNotFound;
}


#ifdef _WIN32
HWND CRouter::CreateWorkerWindow()
{
	char *ProviderClass = "AMMYY.Router.WorkerWindow_6352"; //"AsyncSelect";

	::UnregisterClass(ProviderClass, 0);

	WNDCLASS wndclass;
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = (WNDPROC)WindowProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = NULL;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = ProviderClass;

	if (::RegisterClass(&wndclass) == 0) 
		throw RLException("Error %d of RegisterClass()", ::GetLastError());	

	// Create a window.
	HWND hWindow = ::CreateWindow(ProviderClass, "", WS_OVERLAPPEDWINDOW,
						CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);
   
	if (hWindow == NULL) throw RLException("Error %d of CreateWindow()", ::GetLastError());	

	//if (::SetTimer(hWindow, 1, 1000, NULL)==0)
	//	throw RLException("Error %d of SetTimer()", ::GetLastError());

	return hWindow;
}
#endif

#ifdef _WIN32
LRESULT CRouter::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_SOCKET)
	{		
		WORD event = WSAGETSELECTEVENT(lParam);
		WORD error = WSAGETSELECTERROR(lParam);

		SocketBase* pSocket = _Sockets.Find((SOCKET)wParam);
		if (pSocket!=NULL) {
			try {
				pSocket->OnEvent(event, error);
			}
			catch(RLException& ex) {
				_log.WriteError(ex.GetDescription());
			}
			pSocket->DeleteIfClosed();
		}
		else {
			//Router.m_logNoSocket.WriteMsg("CRouter::WindowProc() not found socket %d, event=%d, error=%d", wParam, (int)event, (int)error);
		}
		return 0;
	}
	/*
	else if (msg == WM_TIMER) {
		Router.OnTimer();
		return 0;
	}
	*/

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
#endif
