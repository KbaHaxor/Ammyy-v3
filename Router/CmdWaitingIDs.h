#if !defined(RL_CMD_WAITING_IDS_H_INCLUDED_)
#define RL_CMD_WAITING_IDS_H_INCLUDED_

#include "../main/CmdBase.h"

// router->webserver, sends new waiting computer ids

class CCmdWaitingIDs : public CmdBase
{
public:
	CCmdWaitingIDs()
	{
		m_cmdID = CMD_WAITING_IDS;
	}

	virtual void ToStream(RLStream* pStream) const
	{
		RLStream ips;
		//GetLocalIPs(ips);

		pStream->AddUINT32(m_cmdID);
		pStream->AddStream(&ips, true);
		pStream->AddStream(&m_IDs, true);
	}
	
	virtual void ParseReply(RLStream* pStream) {}

public:
	// input
	RLStream m_IDs;

private:
	/* no need now, but may be we'll need it in the future
	void GetLocalIPs(RLStream& ips)
	{		
		char hostname[256];

		if (::gethostname(hostname, 256) != 0)
			throw RLException("error=%d while calling gethostname()", ::WSAGetLastError());

		ips.Reset();

		hostent* addresses = ::gethostbyname(hostname);

		if (addresses->h_addrtype != AF_INET || addresses->h_addr_list==NULL)
			return;

		in_addr** p_addr_list = (in_addr**)addresses->h_addr_list;

		while (in_addr* p_addr = *p_addr_list++) {
			in_addr addr = *p_addr;
			ips.AddUINT32((DWORD)addr.s_addr);
		}		
	}
	*/
};

#endif // !defined(RL_CMD_WAITING_IDS_H_INCLUDED_)
