#if (!defined(_TRANSPORT_T_H__ACF324C3_A745_492E_B7EE_61E5B75F2C41__INCLUDED_))
#define _TRANSPORT_T_H__ACF324C3_A745_492E_B7EE_61E5B75F2C41__INCLUDED_

#include "../RL/RLLock.h"
#include "FastQueue.h"
#include "Transport.h"

class TransportTCP1 : public Transport
{
public:
	virtual void SendExact(void* buf, UINT32 len, BOOL block=FALSE);
	virtual void ReadExact(void* buf, UINT32 len);

	void SendLock();
	void SendUnlock();	

private:
	void SendFromQueue();
	void SendQueued(const void *buff, const UINT bufflen);	// puts in a queue, to be sent later.

	// Output queue
	CFastQueue m_send_queue;
	RLStream   m_buffer;
	RLMutex	   m_send_lock;
};


class TransportTCP2: public TransportTCP1
{
public:
	virtual void ReadExact(void* buf, UINT32 len);
};

#endif // _TRANSPORT_T_H__ACF324C3_A745_492E_B7EE_61E5B75F2C41__INCLUDED_
