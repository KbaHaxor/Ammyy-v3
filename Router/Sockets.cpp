#include "stdafx.h"
#include "Sockets.h"

Sockets _Sockets;

Sockets::Sockets()
{
	for (int i=0; i<SocketsCountBaskets; i++)
		m_baskets[i].reserve(12);
}

Sockets::~Sockets()
{
}

// TODO: may be it's better to hold SORTED items in baskets
//
void Sockets::AddNew(CRouter::SocketBase* pSocket)
{
	SOCKET s = pSocket->m_socket;
	ASSERT (s>0);
	SocketsBasket& basket = m_baskets[GetHash(s)];

	//check if exist
	{
		SocketsBasket::iterator it     = basket.begin();
		SocketsBasket::iterator it_end = basket.end();

		for(;it<it_end; it++ ) {
			if ((*it)->m_socket == s) throw RLException("Socket %X already exist", s);
		}
	}

	basket.push_back(pSocket);
}

void Sockets::Remove(CRouter::SocketBase* pSocket)
{
	SOCKET s = pSocket->m_socket;
	ASSERT (s>0);
	SocketsBasket& basket = m_baskets[GetHash(s)];

	SocketsBasket::iterator it     = basket.begin();
	SocketsBasket::iterator it_end = basket.end();

	for(;it<it_end; it++ ) {
		if (*it != pSocket) continue;
		basket.erase(it);
		return;
	}

	throw RLException("Socket %X %X not found on Remove()", s, pSocket);
}


CRouter::SocketBase* Sockets::Find(SOCKET s)
{
	SocketsBasket& basket = m_baskets[GetHash(s)];

	SocketsBasket::iterator it     = basket.begin();
	SocketsBasket::iterator it_end = basket.end();

	for(;it<it_end; it++ ) {
		CRouter::SocketBase* pSocket = *it;
		if (pSocket->m_socket == s) 
			return pSocket;
	}	
	return NULL; // not found
}



// NOTE: on Windows, added 16 or 12 for each socket, on Linux only 1
//
int Sockets::GetHash(SOCKET s)
{
#ifdef _WIN32
	s = s/16;
#endif

	return s % SocketsCountBaskets;
}


int Sockets::GetCount()
{
	int count = 0;
	for (int i=0; i<SocketsCountBaskets; i++) count += m_baskets[i].size();
	return count;
}


//_______________________________________________________________________________


void Sockets::IterReset()
{
	m_iterBasket = -1;
}

CRouter::SocketBase* Sockets::IterNext()
{
	if (m_iterBasket<0) {
		m_iterBasket = 0;
		m_iterIndex = 0;
	}
	else
		m_iterIndex++;

	while(true) {
		if (m_iterIndex<m_baskets[m_iterBasket].size())
			return m_baskets[m_iterBasket][m_iterIndex];

		m_iterBasket++;
		if (m_iterBasket==SocketsCountBaskets) {
			m_iterBasket = -1;
			return NULL; // no more items
		}
		m_iterIndex = 0;
	}
}


CRouter::SocketBase* Sockets::IterPrev()
{
	if (m_iterBasket<0) {
		m_iterBasket = SocketsCountBaskets-1;
		m_iterIndex = m_baskets[m_iterBasket].size() - 1;
	}
	else
		m_iterIndex--;

	while(true) {
		if (m_iterIndex>=0)
			return m_baskets[m_iterBasket][m_iterIndex];

		m_iterBasket--;
		if (m_iterBasket<0) {
			return NULL; // no more items
		}
		m_iterIndex = m_baskets[m_iterBasket].size()-1;
	}
}
