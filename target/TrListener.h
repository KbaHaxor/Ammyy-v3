#if !defined(_TR_LISTENER_H__0C696C5F__INCLUDED_)
#define _TR_LISTENER_H__0C696C5F__INCLUDED_

#include "../main/TCP.h"
#include <list>

class TrListener  
{
public:
	TrListener();
	~TrListener();

	void OpenPort(UINT16 port);
	void FreePort(UINT16 port);

	void OnTimer();
	SOCKET FindSocket(const void* key, int keylen);

	static UINT16 GetIntranetPort();

private:
	int  Find(UINT16 port);
	void CloseAllSockets();

	RLMutex			  m_mutex;

	struct ListenerItem
	{
		TCP		m_socket;
		UINT	m_count;
		UINT16	m_port;

		ListenerItem() 
		{
			m_count = m_port = 0;
		}

		ListenerItem(const ListenerItem& src)
		{
			m_port  = src.m_port;
			m_count = src.m_count;
			ListenerItem* pSrc = (ListenerItem*)&src;
			m_socket.Attach(pSrc->m_socket.Detach());
		}
	};	
	std::vector<ListenerItem> m_listeners;

	struct SocketItem
	{
	public:
		TCP			m_socket;
		UINT16		m_port;
		RLStream	m_recvBuffer;
		DWORD		m_expiredTicks;

		SocketItem()
		{
			m_port = m_expiredTicks = 0;
		}

		SocketItem(const SocketItem& src)
		{
			m_recvBuffer = src.m_recvBuffer;
			m_port  = src.m_port;
			m_expiredTicks = src.m_expiredTicks;

			SocketItem* pSrc = (SocketItem*)&src;
			m_socket.Attach(pSrc->m_socket.Detach());
		}

		bool		OnTimer();
	};
	std::list<SocketItem> m_sockets;
};

class TrListenerWrapper
{
public:
	TrListenerWrapper(UINT16 port);
	~TrListenerWrapper();
	inline bool InOpened() { return m_port>0; }

private:
	UINT16 m_port;
};

#endif // !defined(_TR_LISTENER_H__0C696C5F__INCLUDED_)
