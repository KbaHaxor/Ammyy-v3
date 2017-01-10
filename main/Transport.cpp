#include "stdafx.h"
#include "Transport.h"
#include "../main/aaProtocol.h"
#include "TCP.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


Transport::Transport()
{
	m_socket = INVALID_SOCKET;

	//m_encryptor will be set later in SendInitMsg()
	m_decryptor.SetKey((BYTE*)&aaKey02, false);

	m_encryptor_on = m_decryptor_on = false;

	m_bytesRead = m_bytesSent = 0;
}

Transport::~Transport()
{
}


void Transport::Close(bool linger)
{
	if (m_socket != INVALID_SOCKET) 
	{
		try {
			if (linger) TCP::SetLinger(m_socket, true, 10);

			// to avoid getting 10035 error on closesocket
			TCP::SetBlockingMode(m_socket, true);
		}
		catch(RLException& ex) 
		{
			_log.WriteError(ex.GetDescription());
		}

		//_log.Print(LL_INF, VTCLOG("closing socket"));
		//::shutdown(m_socket, SD_BOTH);
		if (::closesocket(m_socket)!=0)
			_log.WriteError("closesocket() %u", ::GetLastError());

		m_socket = INVALID_SOCKET;
	}
}

bool Transport::IsOpened()
{
	return (m_socket!=INVALID_SOCKET);
}

void Transport::TurnOnEncryptor()
{ 
	m_encryptor_on = m_decryptor_on = true;
}

void Transport::SetTCPSocket(SOCKET s, bool direct)
{
	this->Close(true);
	m_socket = s;

	TCP::SetSocketOptions(s);		// TODO: may be it called before for this socket, but it's ok
	TCP::SetBlockingMode(s, false);
	TCP::SetBufferSizes(s);

	m_direct = direct;
}

/*
void Transport::SetTimeout(UINT32 millisecs)
{
	int timeout=millisecs;
	if (setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout))==SOCKET_ERROR)
		_log2.Print(LL_ERR, VTCLOG("SetTimeout(%d)#1 %X ERROR=%d"), millisecs, m_socket, ::GetLastError());
	
	if (setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout))==SOCKET_ERROR)
		_log2.Print(LL_ERR, VTCLOG("SetTimeout(%d)#2 %X ERROR=%d"), millisecs, m_socket, ::GetLastError());
}
*/

/*
BOOL Transport::SetRecvBufSize(int recvBufSize)
{	
    int err = ::setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (char *)&recvBufSize, sizeof(int));
	return (err==0) ? TRUE : FALSE;
}
*/

void Transport::SetKey(BYTE* key)
{
	if (key==NULL) {
		m_decryptor_on = m_encryptor_on = false;
	}
	else {
		m_encryptor.SetKey(key, true);
		m_decryptor.SetKey(key, false);
	}
}

bool Transport::IsReadyForRead(int timeout_ms)
{
	SOCKET socket = m_socket; // copy it, in case this object was closed and deleted while we're waiting

	struct timeval tm;
	tm.tv_sec  =  timeout_ms/1000;
	tm.tv_usec = (timeout_ms%1000)*1000;

	struct fd_set read_fds;

	FD_ZERO(&read_fds);
	FD_SET(socket, &read_fds);

	int count = select(socket + 1, &read_fds, NULL, NULL, &tm);
	if (count < 0 || count > 2)
		throw TransportException("ERROR in TransportTCP1::IsReadyForRead()#1 %d", count);	// "socket error in select()"

	return (FD_ISSET(socket, &read_fds)!=0);
}

CStringA Transport::GetDescription()
{
	CStringA ipv4 = TCP::GetPeerIPv4(m_socket);
	LPCSTR p = (m_direct) ? "Direct TCP with " : "TCP by router ";
	return p + ipv4;
}


TransportException::TransportException(LPCSTR templ, ...)
{
	va_list ap;
	va_start(ap, templ);
	m_description.FormatV(templ, ap);	
	va_end(ap);
}


// TODO: because problem on router: sometimes terminate message not come
// Windows Router didn't send last message sometimes, Linux works fine
//
bool TransportException::IsNormalClose()
{
	if (m_description.GetLength()>13) {	
		if (m_description.Mid(0, 13) == "ERROR 0 0 in ") return true;
	}
	return false;
}
