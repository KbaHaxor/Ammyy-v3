#include "stdafx.h"
#include "RLHttp2.h"

#ifdef _WIN32
#include <MSTcpIP.h>
#include <Ws2tcpip.h>
#else

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

inline int closesocket(SOCKET s) { return close(s); }
inline int    GetLastError()   	 { return errno; }


#endif


CStringA RLHttp2::m_assignIP;

RLHttp2::RLHttp2()
{
	m_s =0;
	m_szHeader =  "Content-Type: application/x-www-form-urlencoded"
					//"\r\nAccept-Language:ru"
					//"\r\nAccept-Encoding:gzip, deflate"
					;

}

RLHttp2::~RLHttp2()
{
	CloseConnection();
}

UINT32 RLHttp2::ResolveHostname_IPv4(LPCSTR hostname)
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

		if (ip == INADDR_NONE) throw RLException("getaddrinfo('%s') couldn't be resolved", hostname);
	}
 
    return ip;
}

void RLHttp2::CloseConnection()
{
	if (m_s) {
		::closesocket(m_s);
		m_s = 0;
	}
}


// TODO: for FreeBSD need to set global keepalive parameters by sysctl net.inet.tcp.keepidle, net.inet.tcp.keepintvl
void RLHttp2::SetKeepAlive(SOCKET s, int keepidle, int keepintvl)
{	
#ifdef _WIN32
	tcp_keepalive alive;
	DWORD dwSize;

	alive.onoff = 1;
	alive.keepalivetime = keepidle*1000;
	alive.keepaliveinterval = keepintvl*1000;

	if (::WSAIoctl(s, SIO_KEEPALIVE_VALS, &alive, sizeof(alive), NULL, 0, &dwSize, NULL, NULL)!=0)
		throw RLException("ERROR %d while WSAIoctl(SIO_KEEPALIVE_VALS)", ::GetLastError());		
#else
	int optval = 1; // switch-on
	if(::setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0)
		throw RLException("ERROR %d while setsockopt(SO_KEEPALIVE)", errno);

#ifdef _LINUX
	optval = keepidle; // seconds keep-alive
	if(::setsockopt(s, SOL_TCP, TCP_KEEPIDLE, &optval, sizeof(optval)) < 0)
		throw RLException("ERROR %d while setsockopt(TCP_KEEPIDLE)", errno);

	optval = keepintvl; // seconds for each attempt
	if(::setsockopt(s, SOL_TCP, TCP_KEEPINTVL, &optval, sizeof(optval)) < 0)
		throw RLException("ERROR %d while setsockopt(TCP_KEEPINTVL)", errno);

	optval = 3; // 3 times to try
	if(::setsockopt(s, SOL_TCP, TCP_KEEPCNT, &optval, sizeof(optval)) < 0)
		throw RLException("ERROR %d while setsockopt(TCP_KEEPCNT)", errno);
#endif
#endif
}

void RLHttp2::SetTimeOut(SOCKET s, int seconds)
{
#ifdef _WIN32
	int timeout=seconds*1000; // milliseconds
#else
	timeval timeout;
	timeout.tv_sec = seconds;
	timeout.tv_usec = 0;
#endif
	if (::setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout))==SOCKET_ERROR) 
		throw RLException("ERROR %d while setsockopt(SO_RCVTIMEO) in RLHttp2", ::GetLastError());
		
	if (::setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout))==SOCKET_ERROR) 
		throw RLException("ERROR %d while setsockopt(SO_SNDTIMEO) in RLHttp2", ::GetLastError());
}


void RLHttp2::ConnectByTCP(LPCSTR host, int port)
{ 
	CloseConnection();

	SOCKET s = ::socket(PF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET) 
		throw RLException("ERROR %d of creating socket", ::GetLastError());

	m_s = s;

	if ( (!m_assignIP.IsEmpty()) && m_assignIP!="0.0.0.0") {
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = ::inet_addr(m_assignIP);
		addr.sin_port = 0;

		if (::bind(m_s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
			throw RLException("ERROR %u while bind() in RLHttp2::ConnectByTCP()", ::GetLastError());		
	}

	SetTimeOut(m_s, 60);
	
	sockaddr_in thataddr;
	thataddr.sin_addr.s_addr = ResolveHostname_IPv4(host);
	thataddr.sin_family = AF_INET;
	thataddr.sin_port = htons(port);

	if (::connect(m_s, (sockaddr*) &thataddr, sizeof(thataddr)) != 0)
		throw RLException("ERROR %d while coonect('%s')", ::GetLastError(), host);
			
	SetKeepAlive(m_s); // set keepalive, to detect connection breaks
}


void RLHttp2::Get(LPCSTR url)
{
	SendRequest("GET", url, NULL);
}


void RLHttp2::Post(LPCSTR url, RLStream* pOptionalData)
{
	SendRequest("POST", url, pOptionalData);
}


void RLHttp2::SendRequest(LPCSTR queryType, LPCSTR url, RLStream* pOptionalData)
{
	char	szHost[128];
	char	szProtocol[32];
	WORD	wPort;
	char	szURI[128];
	
	ParseURL(url, szProtocol, szHost, wPort, szURI);
	
	UINT len = (pOptionalData) ? pOptionalData->GetLen() : 0;

	CStringA queryStr;
	queryStr.Format(
		"%s /%s HTTP/1.1\r\n"
		"%s\r\n"
		"Host: %s\r\n"
		"Content-Length: %u\r\n"
		"Cache-Control: no-cache\r\n\r\n",
		queryType,
		szURI,
		(LPCSTR)m_szHeader,
		szHost,
		len	);

	RLStream query(queryStr.GetLength()+len);
	query.AddString0A(queryStr);
	if (len>0) query.AddRaw(pOptionalData->GetBuffer(), len);

	ConnectByTCP(szHost, wPort);	

	int needToSend = (int)query.GetLen();
	int sent = ::send(m_s, (char*)query.GetBuffer(), needToSend, 0);

	if (sent!=needToSend)
		throw RLException("ERROR %d while send('%s') in RLHttp2 %d %d", ::GetLastError(), url, sent, needToSend);

	// receive reply
	RLTimer t1;
	CStringA bufferIn;
	int count_recv = 0; // count recvieved	
	while(true)
	{
		int c = ::recv(m_s, bufferIn.GetBuffer(count_recv+2048)+count_recv, 2048, 0);
		if (c < 1)
			throw RLException("ERROR %d while recv('%s') in RLHttp2", ::GetLastError(), url);

		count_recv += c;

		bufferIn.ReleaseBuffer(count_recv);

		if (ParseReply(bufferIn)) break;

		if (t1.GetElapsedSeconds()>60)
			throw RLException("ERROR timeout in RLHttp2::SendRequest()");
	}	
}


int RLHttp2::GetDigit(LPCSTR p, int count, UINT& digit)
{
	if (count==0) return 0;

	LPCSTR p1 = p;

	UINT val = 0;

	while(true)
	{
		char c = *p1++;
		
		if (c>='0' && c<='9') {
			c = c - '0';
			val = val*16 + c;
		}
		else if (c>='A' && c<='F') {
			c = c - 'A' + 10;
			val = val*16 + c;
		}
		else if (c>='a' && c<='f') {
			c = c - 'a' + 10;
			val = val*16 + c;
		}
		else if (c=='\r') { // just skip it
		}
		else if (c=='\n') {
			int count = p1-p;
			if (count<2) 
				throw RLException("RLHttp2::GetDigit()#1");
			digit = val;
			return count;
		}
		else
			throw RLException("RLHttp2::GetDigit()#2");

		count--;
		if (count==0) return 0;
	}
}



bool RLHttp2::ParseReply(const CStringA& bufferIn)
{
	int i1 = bufferIn.Find("\r\n\r\n");
	if (i1<0) return false;

	{
		int i1 = bufferIn.Find(" ");
		if (i1<0) throw RLException("RLHttp2::ParseReply()#1");
		i1++;

		int i2 = bufferIn.Find(" ", i1);
		if (i2<0) throw RLException("RLHttp2::ParseReply()#2");

		CStringA status((LPCSTR)bufferIn + i1, i2-i1);

		m_replyStatus = status;
	}

	i1 += 4;
	LPCSTR p = (LPCSTR)bufferIn + i1;
	int count = bufferIn.GetLength() - i1; // leave bytes

	int k1 = bufferIn.Find("Content-Length: ");
	if (k1>0) {
		k1 += 16; // length of "Content-Length: "
		int contentLengh = atol((LPCSTR)bufferIn + k1);

		if (count<contentLengh) return false;

		m_reply.Reset();
		m_reply.AddRaw(p, contentLengh);

		return true;
	}
	else {
		// chunk
		k1 = bufferIn.Find("Transfer-Encoding: chunked\r\n");
		if (k1<0 || k1>=i1)
			throw RLException("RLHttp2::ParseReply()#3");

		int j2 = bufferIn.Find("\r\n\r\n", i1+4);
		if (j2<0) return false;


		m_reply.Reset();

		if (count<=0) return false;

		while(true) 
		{
			UINT chunk_size = 0;

			int i3 = GetDigit(p, count, chunk_size);

			if (i3==0) return false;

			p+=i3;
			count-=i3;

			if (chunk_size==0) return true; //finished!

			if (count<i3) return false;

			m_reply.AddRaw(p, chunk_size);

			p    +=chunk_size;
			count-=chunk_size;

			if (count<2) return false;

			if (p[0] != '\r' && p[1] != '\n') 
				throw RLException("RLHttp2::ParseReply()#4");

			p += 2;
			count -= 2;
		}
	}	
}


void RLHttp2::ParseURL(LPCSTR szURL, LPSTR szProtocol, LPSTR szHost, WORD &nPort, LPSTR szURI)
{
	if (strlen(szURL)==0) return;

	const char* pPosition = strstr(szURL, "://");

	if (pPosition != NULL) {
		if(szProtocol){
			DWORD len = pPosition-szURL;
			strncpy(szProtocol, szURL, len);
			szProtocol[len]=0;
		}
		pPosition+=3;
	}else {
		if(szProtocol) {strcpy(szProtocol, "http");}
		pPosition=szURL;
	}

	nPort = (strcmp(szProtocol, "https")==0) ? 443 : 80;
	
	LPCSTR pUriBegin = strstr(pPosition, "/");
	if (pUriBegin==NULL) pUriBegin = szURL + strlen(szURL);

	const char* pHostEnd = pUriBegin;
	
	for(const char* p=pPosition; p<pHostEnd; p++){
		if(*p == ':') { // find PORT

			char szPort[32]= "";
			strncpy(szPort, p+1, pHostEnd-p-1);
			nPort=(WORD)atol(szPort);			

			pHostEnd = p;
			break;
		}
	}

	int count = pHostEnd-pPosition;
	strncpy(szHost, pPosition, count);
	szHost[count]=0;

	if (*pUriBegin=='/') pUriBegin++;
	strcpy(szURI, pUriBegin);
}
