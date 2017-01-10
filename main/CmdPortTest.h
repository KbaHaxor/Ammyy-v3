#if !defined(RL_CMD_PORT_TEST_H_INCLUDED_)
#define RL_CMD_PORT_TEST_H_INCLUDED_

#include "CmdBase.h"

class CmdPortTest : public CmdBase
{
public:
	CmdPortTest();

	virtual void ToStream(RLStream* pStream) const;
	virtual void ParseReply(RLStream* pStream);

	// input
	UINT16   m_port;
	char     m_key[16];

	// output
	UINT32   m_external_ip;
	UINT16   m_external_port;
	UINT8    m_result;
};

#endif // !defined(RL_CMD_PORT_TEST_H_INCLUDED_)
