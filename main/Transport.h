#if !defined(_TRANSPORT_H__993EF4F5__INCLUDED_)
#define _TRANSPORT_H__993EF4F5__INCLUDED_

#include "../RL/RLEncryptor02.h"

class Transport
{
	friend class InteropCommon; // for m_encryptor

public:
	void Close(bool linger);		// Close the currently attached socket
	bool IsOpened();
	void SetTCPSocket(SOCKET s, bool direct);
	bool IsReadyForRead(int timeout_ms);
	CStringA GetDescription();
	
	// SendExact and ReadExact attempt to send and recieve exactly the specified number of bytes.
	virtual void SendExact(void* buf, UINT32 len, BOOL block=FALSE) = 0;
	virtual void ReadExact(void* buf, UINT32 len) = 0;
	
	//virtual void SetTimeout(UINT32 millisecs);

	virtual ~Transport();

	virtual void SendLock() = 0;
	virtual void SendUnlock() = 0;

	void TurnOnEncryptor();

	void SetKey(BYTE* key);

	//operator SOCKET () { return m_socket; }
	SOCKET GetSocket() { return m_socket; }


	inline void  SendByte(BYTE b, BOOL block=FALSE) {this->SendExact(&b, 1, block);}
	inline UINT8 ReadByte() { UINT8 b; this->ReadExact(&b, 1); return b; }
	inline void  ReadStream16(RLStream& buffer)
	{
		UINT16 count;
		this->ReadExact(&count, sizeof(count));		
		buffer.SetMinCapasity(count);
		this->ReadExact(buffer.GetBuffer(), count);
		buffer.SetLen(count);
	}


protected:
	Transport();
	
	SOCKET m_socket;
	RLEncryptor02 m_encryptor;
	RLEncryptor02 m_decryptor;
public:
	bool m_decryptor_on;
	bool m_encryptor_on;

public:
	UINT64 m_bytesRead;
	UINT64 m_bytesSent;

public:
	bool m_direct;
};

class TransportException: public RLException
{
public:
	TransportException(LPCSTR templ, ...);

	bool IsNormalClose();
};

#endif // !defined(_TRANSPORT_H__993EF4F5__INCLUDED_)
