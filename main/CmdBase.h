#if !defined(RL_CMDBASE_H_INCLUDED_)
#define RL_CMDBASE_H_INCLUDED_

#include "../RL/RLStream.h"
#include "../RL/RLBase64Coder.h"

const DWORD CMD_QUERY_TRIAL = 1;
const DWORD CMD_BUG_REPORT = 2;
const DWORD CMD_ACTIVATE   = 3;
const DWORD CMD_MSG_BOX    = 4;
const DWORD CMD_INIT = 6;	// inluded in v2.1
const DWORD CMD_SESSION_ENDED = 7;	// for router
const DWORD CMD_WAITING_IDS   = 8;	// for router
const DWORD CMD_GET_ROUTER_FOR_ID = 12;
const DWORD CMD_PORT_TEST = 13;
const DWORD CMD_SESSION_ENDED_v3 = 14;	// for router & for operator

class CmdBase
{
protected:
	CmdBase();
	virtual ~CmdBase();	
	virtual void ToStream(RLStream* pStream) const;
	virtual void ParseReply(RLStream* pStream) {}
	virtual void Execute() {}

public:
	void Send();

private:
	       void ExecuteReplyFromServer(RLStream* pStream);

#ifdef _WIN32
	static GEOID GetUserLocation();
	static DWORD GetUserFormatCountryCode();
	static INT32 GetProcessorArchitecture();
#endif

	static RLBase64Coder m_base64coder;

protected:
	static void	 AddOSInfo(RLStream* pStream);	
	static void  AddOSLanguage(RLStream* pStream);

public:
	static CStringA m_strAppName;
	static CStringA m_strAppVersion;
	static CStringA m_strAppModule;
	static CStringA m_strProductKey;
	static CStringA m_strURL;

protected:
	DWORD	m_cmdID;
public:
	CStringA m_strComputerId;	// previous value
};

#endif // !defined(RL_CMDBASE_H_INCLUDED_)
