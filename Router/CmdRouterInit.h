#if !defined(RL_CMD_ROUTER_INIT_H_INCLUDED_)
#define RL_CMD_ROUTER_INIT_H_INCLUDED_

#include "../main/CmdBase.h"

class CCmdRouterInit : public CmdBase
{
public:
	CCmdRouterInit();

	virtual void ToStream(RLStream* pStream) const;
	virtual void ParseReply(RLStream* pStream);

	// input
	CStringA m_strBuildStamp;
	CStringA m_strPorts;
	CStringA m_strIPs;

	// output
	CStringA m_strStatus;
};

const DWORD CMD_ROUTER_INIT = 10;

CCmdRouterInit::CCmdRouterInit()
{
	m_cmdID = CMD_ROUTER_INIT;
}

void CCmdRouterInit::ToStream(RLStream* pStream) const
{
	pStream->AddUINT32(m_cmdID);
	pStream->AddString1A(m_strBuildStamp);
	pStream->AddString1A(m_strPorts);
	pStream->AddString1A(m_strIPs);
}

void CCmdRouterInit::ParseReply(RLStream* pStream) 
{
	m_strStatus = pStream->GetString1A();
}

#endif // !defined(RL_CMD_ROUTER_INIT_H_INCLUDED_)
