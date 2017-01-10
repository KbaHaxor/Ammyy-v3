#if !defined(RL_CMD_GET_ROUTER_FOR_ID_H_INCLUDED_)
#define RL_CMD_GET_ROUTER_FOR_ID_H_INCLUDED_

#include "CmdBase.h"

// installer->webserver

class CmdGetRouterForID : public CmdBase
{
public:
	CmdGetRouterForID();

	virtual void ToStream(RLStream* pStream) const;
	virtual void ParseReply(RLStream* pStream);

	// input
	DWORD m_localID;
	DWORD m_remoteID;

	//output
	CStringA m_status;
	CStringA m_router;

	CStringA m_msgText;
	UINT32   m_msgStyle;
	CStringA m_url;
	bool	 m_allow;

};

#endif // !defined(RL_CMD_GET_ROUTER_FOR_ID_H_INCLUDED_)
