#include "stdafx.h"
#include "InteropCommon.h"
#include "../main/aaProtocol.h"
#include "CmdGetRouterForID.h"
#include "Common.h"
#include "AmmyyApp.h"
#include "TransportT.h"
#include <MSTcpIP.h>
#include <Ws2tcpip.h>
#include "../RL/RLHttp.h"
#include "../RL/RLBase64Coder.h"
#include "proxy/HttpClient.h"
#include "RLLanguages.h"
#include "rsa/rsa.h"
#include "TCP.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


HWND   InteropCommon::m_hErrorWnd = NULL;


InteropCommon::InteropCommon() 
{
	m_remoteId = 0;
	m_bStopping = NULL;
	m_counterAaPointerEvent = 0;
}

InteropCommon::~InteropCommon() {}



void InteropCommon::ParseRouter(LPCSTR pRouter, CStringA& ip, CStringA& ports)
{
	LPCSTR p1 = strchr(pRouter, ':');

	if (p1==NULL) 
		throw RLException("Invalid router is provided '%s'", pRouter);

	int len = p1 - pRouter;
	strncpy(ip.GetBuffer(len+1), pRouter, len);
	ip.ReleaseBuffer(len);

	ports = p1+1;

	if (ip.IsEmpty()) {
		throw RLException("Invalid router's IP is provided '%s'", pRouter);
	}
}

DWORD   InteropCommon::GetRemoteId()	{ return m_remoteId; }


// get routers ip and ports
CStringA InteropCommon::GetRouters()
{
	if (m_remoteId!=0)
	{
		SendInfo("Get Router IP");

		CmdGetRouterForID cmd;
		cmd.m_localID  = TheApp.m_ID;
		cmd.m_remoteID = this->m_remoteId;
		cmd.Send();

		if (!cmd.m_status.IsEmpty()) {
			if (cmd.m_status=="NOT_FOUND") {
				LPCSTR msg = rlLanguages.GetValueA(D_PCNotFound);
				throw RLException((LPCSTR)msg, this->m_remoteId);
			}
			else
				throw RLException("Couldn't get router ip, status='%s'", cmd.m_status);
		}

		if (!cmd.m_msgText.IsEmpty()) {
			::MessageBox(TheApp.m_hMainWnd, cmd.m_msgText, "Ammyy Admin", cmd.m_msgStyle);
		}

		if (!cmd.m_url.IsEmpty()) {
			::ShellExecute(NULL, "open", cmd.m_url, NULL, NULL, SW_SHOWNORMAL);
		}

		if (!cmd.m_allow)
			throw RLException("Access denied by server.");

		if (settings.m_requestRouter) 
			return cmd.m_router;
	}

	return settings.GetRouters();
}

void InteropCommon::SendInitMsg(SOCKET s)
{
	RLStream buffer(512);

	{
		RLMD5 md5;
		md5.SetEmpty();
		//md5.Calculate("");

		DWORD ticks = ::GetTickCount();
		UINT16 random1 = ticks;
		UINT16 random2 = ticks>>7;

		buffer.AddUINT8('=');								// preInitMsg
		
		// no need for LAN but I leave it for anti-debugging
		buffer.AddUINT16(random1);							// just 2 random bytes for crypto
		buffer.AddRaw(md5.hash, 16);						// router password

		bool target = this->IsTarget();
		UINT8  type = (target) ? aaTypeTarget : aaTypeViewer;
		UINT32 v1   = (target) ? PingTime : m_remoteId;
		
		buffer.AddUINT16(random2);							// just 2 random bytes for crypto
		buffer.AddUINT16(0);								// reversed, need to checking
		buffer.AddUINT32(Settings::GetVersionINT());		// version
		buffer.AddUINT8(type);
		buffer.AddUINT32(TheApp.m_ID);
		buffer.AddUINT32(v1);
	}

	this->AddToInitMsg(buffer);

	// encrypting
	m_transport->m_encryptor.SetKey((BYTE*)&aaKey02, true);
	RLEncryptor02 encryptor;
	encryptor.Copy(m_transport->m_encryptor);
	               encryptor.Encrypt((BYTE*)buffer.GetBuffer()+1,    18);
	m_transport->m_encryptor.Encrypt((BYTE*)buffer.GetBuffer()+1+18, buffer.GetLen()-(1+18));

	int ret = ::send(s, (LPCSTR)buffer.GetBuffer(), buffer.GetLen(), 0);
	if (ret!=buffer.GetLen())
		throw CWarningException("error=%d, while sending Initial msg", ::WSAGetLastError());
}



SOCKET InteropCommon::ConnectToRouterOneAttempt(LPCSTR routerIP, int routerPort, const CStringA& proxyAddress, UINT16 proxyPort)
{
	TCP tcp_socket; // shoule be closed automatically in destructor
			
	try 
	{
		SetState(STATE::STARTED);

		tcp_socket.Create();

		if (proxyPort==0) {
			SendInfo("Connecting to %s:%d", routerIP, routerPort);

			DWORD error = tcp_socket.Connect(routerIP, routerPort, false);			
			
			//it's NOT critical error if we can't connect to this router
			if (error>0)
				throw CWarningException("Couldn't connect to router %s:%d", routerIP, routerPort);

			SendInfo("Connected to %s:%d", routerIP, routerPort);
		}
		else { // if (routerPort==0) connect by HTTPs proxy			
			SendInfo("Connecting to %s by proxy %s:%d", routerIP, (LPCSTR)proxyAddress, (int)proxyPort);

			CHttpClient	conn;
			conn.m_proxyAddress = proxyAddress;
			conn.m_proxyPort    = proxyPort;
			conn.m_userName = RLHttp::m_proxyUsername;
			conn.m_password = RLHttp::m_proxyPassword;

			if (!conn.ConnectByHttpsProxy(routerIP, routerPort))
				throw RLException("ERROR %d: failed to establish connection", conn.m_dwLastError);

			tcp_socket.Attach(conn.Detach());

			SendInfo("Connected to %s by HTTPS proxy", routerIP);
		}

		TCP::SetSocketOptions(tcp_socket);

		SetState(STATE::TCP_CONNECTED);
				
		if (IsStopping()) {
			throw CStoppingException();
		}
				
		this->SendInitMsg(tcp_socket);

		this->SendInfo("Sent initial message to router %s", routerIP);

		if (IsStopping()) {
			throw CStoppingException();
		}

		char buffer[10];
		ReadNonBlock(tcp_socket, 3000, buffer, 2);

		if (buffer[0] != aaPreInitReply[0])
			throw CWarningException("Invalid router's reply %u", 1);

		INT8 status = buffer[1];

		if (status!=0)
		{
			if (status==Router_Denied)
				throw RLException("Access denied by router %s", routerIP);

			if (status==Router_PasswordIncorrect)
				throw RLException("Incorrect password for router %s", routerIP);

			if (m_type==aaTypeViewer)
			{
				if (status==Router_ComputerIsBusy) {
					CStringA string = rlLanguages.GetValueA(D_COMPUTER_BUSY) + CStringA(".\n") + rlLanguages.GetValueA(D_BUY_LICENSE);
					throw RLException((LPCSTR)string);
				}
				else if (status == Router_ComputerNotFound)
				{
					LPCSTR string = rlLanguages.GetValueA(D_PCNotFound);
					throw RLException((LPCSTR)string, this->GetRemoteId());
				}
				else if (status==Router_ComputerOtherVersion) {
					throw RLException("Client's computer has other version");
				}
			}

			throw RLException("Invalid router's reply %u", 2);
		}

		this->SendInfo("Connected to router '%s:%d' successfully", routerIP, routerPort);

		SetState(STATE::ROUTER_CONNECTED);

		return tcp_socket.Detach();
	}
	catch(CWarningException& ex) {
		SetState(STATE::STARTED);
		this->SendInfo("ERROR: %s", ex.GetDescription());		
		return INVALID_SOCKET;
	}
	catch(RLException& ex) {
		SetState(STATE::STARTED);
		this->SendInfo("ERROR: %s", ex.GetDescription());
		throw;
	}
}


BYTE InteropCommon::ReadByteWithPinging(bool& waitingPingReply)
{
begin:
	if (!waitingPingReply) {
		if (m_transport->IsReadyForRead(PingTime*1000)) goto read_ready;
		m_transport->SendByte(aaPingRequest, TRUE);
		waitingPingReply = true;
	}

	if (waitingPingReply) {
		if (m_transport->IsReadyForRead(PingTimeOut*1000)) goto read_ready;
		throw RLException("Ping timeout was expired");
	}

read_ready:
	BYTE b = m_transport->ReadByte();

	if (b==aaPingReply) {
		if (!waitingPingReply) throw RLException("Invalid aaPingReply");
		waitingPingReply = false;
		goto begin;
	}
	return b;
}


void InteropCommon::ReadInitMsg(bool lan, UINT32& myId, UINT32& remoteId, UINT32& version)
{
	bool target = this->IsTarget();

	BYTE b = aaPreInitMsg_v3[0];

	if (!lan && target) 
	{
		bool waitingPingReply = false;
		
		b = this->ReadByteWithPinging(waitingPingReply); // send ping request

		// for testing very rare case, when sent aaPingRequest and viewer connected at the same time
		//m_transport->SendByte(aaPingRequest, TRUE);
	}
	else {
		// this byte was read before in case of ( lan && target) in TrListener::SocketItem::OnTimer()
		// this byte was read before in case of (!lan && target) in this function
		//
		if (!target) {
			b = m_transport->ReadByte();
		}
	}

	if (b!=aaPreInitMsg_v3[0]) 
		throw RLException("Invalid initmsg %u %u", __LINE__, (int)b);

	if (lan) {
		// no need for LAN but I leave it for anti-debugging
		char buffer1[18];
		m_transport->ReadExact(buffer1, sizeof(buffer1)); // 2 bytes random + md5
	}
	else {
		this->ReadRouterInfo(m_transport);
	}

	BYTE buffer[17];

	m_transport->TurnOnEncryptor();
	m_transport->ReadExact(buffer, sizeof(buffer));

	UINT16 reversed = *((UINT16*)&buffer[ 2]);
	       version  = *((UINT32*)&buffer[ 4]);
	UINT8  type     = *((UINT8*) &buffer[ 8]);
	       myId     = *((UINT32*)&buffer[ 9]);
	       remoteId = *((UINT32*)&buffer[13]);

	if (reversed!=0) throw RLException("Invalid initmsg %u", __LINE__);
	if ( target && type!=aaTypeViewer) throw RLException("Invalid initmsg %u", __LINE__);
	if (!target && type!=aaTypeTarget) throw RLException("Invalid initmsg %u", __LINE__);

	if (target) {
		if (!lan && remoteId!=TheApp.m_ID) throw RLException("Invalid initmsg %u", __LINE__);
		if ( lan && remoteId!=0)           throw RLException("Invalid initmsg %u", __LINE__);
	}
	else {
		if (m_remoteId!=0 && myId!=m_remoteId) throw RLException("Invalid initmsg %u", __LINE__);
	}
}


void InteropCommon::ReadRouterInfo(Transport* transport)
{
	aaRouterInfoMsg msg;

	transport->ReadExact(&msg, sizeof(msg));

	RLEncryptor02 decryptor;
	decryptor.Copy(transport->m_decryptor);
	decryptor.Decrypt((BYTE*)&msg, sizeof(msg));

	m_router_time   = msg.time;
	m_external_port = msg.tcp_port;
	m_external_ip   = msg.ip;
}

void InteropCommon::ConnectToRouter()
{
	if (TheApp.m_ID==0)
		throw RLException("Your ID is empty");

	SOCKET s = INVALID_SOCKET;
	CStringA routers = GetRouters();

	for (int pos1=0;;)
	{
		CStringA router = CCommon::GetSubString(routers, "|", pos1);

		if (router.IsEmpty()) {
			throw RLException("Couldn't connect to routers %s", routers);
		}

		CStringA routerIP, routerPorts;

		ParseRouter(router, routerIP, routerPorts);

		CStringA proxyHost = RLHttp::m_proxyHost;
		UINT16   proxyPort = RLHttp::m_proxyPort;
		
		for (int pos2=0;;)
		{
			CStringA port = CCommon::GetSubString(routerPorts, ",", pos2);
			if (port.IsEmpty()) break;

			if (IsStopping()) throw CStoppingException();

			if (proxyPort) {
				s = ConnectToRouterOneAttempt(routerIP, 443, proxyHost, proxyPort);
				if (s!=INVALID_SOCKET) goto exit;
				break; // if usuing proxy just only 1 try for router
			}
			else {
				s = ConnectToRouterOneAttempt(routerIP, atol(port), "", 0);
				if (s!=INVALID_SOCKET) goto exit;
			}
		}
	}

exit:
	m_transport->SetTCPSocket(s, false);
}


// check first 1 bytes of reply
//
void InteropCommon::ReadNonBlock(SOCKET s, UINT timeout, char* buffer, int len)
{
	DWORD dwTickStart = ::GetTickCount();

	while(true) {
		u_long bytesRecv = 0;
		int ret = ::ioctlsocket(s, FIONREAD, &bytesRecv);

		if (bytesRecv<len) 
		{
			if (::GetTickCount()-dwTickStart>timeout)
				throw CWarningException("Router's 'Initial reply' timeout is expired");
			
			::Sleep(50);
			continue;
		}

		if (recv(s, buffer, len, 0) != len) {	// MSG_PEEK
			throw CWarningException("ReadNonBlock() error=%d", ::WSAGetLastError());
		}

		return; // ok
	}
}



void InteropCommon::SendInfo(LPCSTR templ, ...)
{	
	CStringA msg;

	va_list arg;
	va_start(arg, templ);
	msg.FormatV(templ, arg);
	va_end(arg);

	//LPCSTR prefix = ((m_type == aaTypeTarget)  ? "Target: ": "Viewer: ");
	//msg = prefix + msg;

	_log.WriteInfo((LPCSTR)msg);

	if (m_hErrorWnd!=NULL) {
		VERIFY(::SendMessage((HWND)m_hErrorWnd, WM_SETTEXT, 0, (LPARAM)msg.GetBuffer(0)) != 0);
	}
}

void InteropCommon::ShowError(LPCSTR error)
{
	SendInfo("ERROR: %s", error);
}



void InteropCommon::SetStoppingEvent(BOOL* pEvent)
{
	m_bStopping = pEvent;
}


void InteropCommon::Thread01_start()
{
	 // start new thread
	DWORD dwThreaId;
	HANDLE hThread = ::CreateThread(NULL, 0, InteropCommon::Thread01_static, (LPVOID)this, 0, &dwThreaId);

	if (hThread==NULL)
		throw RLException("CreateThread() Error %d", ::GetLastError());
	
	if (::CloseHandle(hThread)==0)
		throw RLException("CloseHandle() Error %d", ::GetLastError());
}

DWORD InteropCommon::Thread01_static(LPVOID lpParameter)
{
	InteropCommon* _this = (InteropCommon*)lpParameter;
	_this->Thread01();
	return 0;
}



CStoppingException::CStoppingException(){}

CWarningException::CWarningException(LPCSTR templ, ...)
{
	va_list ap;
	va_start(ap, templ);
	m_description.FormatV(templ, ap);	
	va_end(ap);
}
