#if !defined (_SOCKETS_H__3B37A832__INCLUDED_)
#define _SOCKETS_H__3B37A832__INCLUDED_

#include "CRouter.h"

const int SocketsCountBaskets = 1000;

class Sockets
{
public:
	Sockets();
	~Sockets();
	
	void AddNew(CRouter::SocketBase* pSocket);
	void Remove(CRouter::SocketBase* pSocket);
	CRouter::SocketBase* Find(SOCKET s);
	int GetCount();

	void IterReset();
	CRouter::SocketBase* IterNext();
	CRouter::SocketBase* IterPrev();

private:
	static inline int GetHash(SOCKET s);

	typedef std::vector<CRouter::SocketBase*> SocketsBasket;

	SocketsBasket m_baskets[SocketsCountBaskets];

	int m_iterBasket; // -1 - not set
	int m_iterIndex;
};

extern Sockets _Sockets;

#endif // !defined(_SOCKETS_H__3B37A832__INCLUDED_)
