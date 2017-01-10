#include "stdafx.h"
#include "TrListener.h"
#include "../main/common.h"
#include "../main/aaProtocol.h"
#include "TrClient.h"


TrListener::TrListener()
{
}

TrListener::~TrListener()
{
	// to avoid unhandled exception if will error in m_socket.Close()
	//
	for (int i=0; i<m_listeners.size(); i++)
	{
		m_listeners[i].m_socket.CloseSafe();
	}
}

void TrListener::OpenPort(UINT16 port)
{
	RLMutexLock l(m_mutex);

	int index = Find(port);

	if (index>=0) {
		m_listeners[index].m_count++;
		return; // ok
	}

	TCP tcp;
	tcp.Listen(port); // it can throw exception

	int count = m_listeners.size();
	m_listeners.resize(count+1);
	m_listeners[count].m_count = 1;
	m_listeners[count].m_port  = port;
	m_listeners[count].m_socket.Attach(tcp.Detach());
}


void TrListener::FreePort(UINT16 port)
{
	RLMutexLock l(m_mutex);

	int i = Find(port);

	if (i<0)
		throw RLException("TrListener::Free(%u)", (int)port);

	m_listeners[i].m_count--;
	if (m_listeners[i].m_count==0) {
		m_listeners[i].m_socket.Close();
		m_listeners.erase(m_listeners.begin()+i);

		if (m_listeners.size()==0) {
			CloseAllSockets();
		}
	}
}

void TrListener::OnTimer()
{
	RLMutexLock l(m_mutex);

	int count = m_listeners.size();

	for (int i=0; i<count; i++) {
		SOCKET s = m_listeners[i].m_socket.Accept(0);

		if (s!=INVALID_SOCKET)
		{
			m_sockets.resize(m_sockets.size()+1);

			SocketItem si;
			si.m_socket.Attach(s);
			si.m_port = m_listeners[i].m_port;
			si.m_recvBuffer.SetMinCapasity(32);
			si.m_expiredTicks = ::GetTickCount() + 10000; // expired in 10 seconds

			m_sockets.push_back(si);
		}
	}

	for (std::list<SocketItem>::iterator it=m_sockets.begin() ; it != m_sockets.end();)
	{
		if (!(*it).OnTimer()) {
			it = m_sockets.erase(it); // it closes socket in destructor
		}
		else {
			it++;
		}
	}
}

int TrListener::Find(UINT16 port)
{
	int count = m_listeners.size();
	for (int i=0; i<count; i++) {
		if (m_listeners[i].m_port == port) return i;
	}
	return -1;
}


bool TrListener::SocketItem::OnTimer()
{
	if (CCommon::TimeIsReady(m_expiredTicks)) return false;

	if (m_recvBuffer.GetLen()==0) {
		int read = m_socket.TryRead(m_recvBuffer.GetBufferWr(), 1, 0);

		if (read==0) return true;  // no data, try next time
		if (read==1) {
			m_recvBuffer.AddRaw(NULL, read);
			UINT8 b = *(char*)m_recvBuffer.GetBuffer();

			if (b==aaPreInitMsg_v3[0]) 
			{
				if (settings.m_allowIncomingByIP && m_port==DEFAULT_INCOME_PORT) {
					TrClient* client = _TrMain.AddClient();
					if (client) {
						client->m_LAN = true;
						client->m_transport->SetTCPSocket(m_socket.Detach(), true);
						client->Thread01_start();
					}
				}
				return false; // remove it
			}
				
			if(b!='%') return false;
		}
		else {
			return false; // error of reading, close it
		}
	}

	int len = m_recvBuffer.GetLen();

	if (len>=1 && len<17) {
		int read = m_socket.TryRead(m_recvBuffer.GetBufferWr(), 17-len, 0);

		if (read<0)  return false; // error of reading, close it
		if (read==0) return true;  // no data, try next time

		m_recvBuffer.AddRaw(NULL, read);
	}

	return true;
}

SOCKET TrListener::FindSocket(const void* key, int keylen)
{
	RLMutexLock l(m_mutex);

	for (std::list<SocketItem>::iterator it=m_sockets.begin() ; it != m_sockets.end(); it++)
	{
		SocketItem& si = *it;

		if (si.m_recvBuffer.GetLen()==keylen+1) {
			if (memcmp((char*)si.m_recvBuffer.GetBuffer()+1, key, keylen)==0) {
				SOCKET s = si.m_socket.Detach();
				m_sockets.erase(it);
				return s;
			}
		}
	}

	return 0; // not found
}

void TrListener::CloseAllSockets()
{
	m_sockets.clear();

	/*
	for (std::list<SocketItem>::iterator it=m_sockets.begin() ; it != m_sockets.end();)
	{
		SocketItem& si = *it;
		it = m_sockets.erase(it);
	}
	*/
}

UINT16 TrListener::GetIntranetPort()
{
	// if DEFAULT_INCOME_PORT is opened by this process, let's use it to avoid firewalls alert for open new port,
	// But if we need to open port we'll open DEFAULT_INTRANET_PORT, because DEFAULT_INCOME_PORT can be busy by other process

	return (_TrMain.m_listener.Find(DEFAULT_INCOME_PORT)>=0) ? DEFAULT_INCOME_PORT : DEFAULT_INTRANET_PORT;
}

//____________________________________________________________________________________________


TrListenerWrapper::TrListenerWrapper(UINT16 port)
{
	m_port = 0;
	try {
		_TrMain.m_listener.OpenPort(port);
		m_port = port; // ok
	}
	catch(RLException&) {}
}

TrListenerWrapper::~TrListenerWrapper()
{
	if (m_port>0)
		_TrMain.m_listener.FreePort(m_port);
}
