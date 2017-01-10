#if !defined(RL_CMD_INIT_H_INCLUDED_)
#define RL_CMD_INIT_H_INCLUDED_

#include "CmdBase.h"

class CmdInit : public CmdBase
{
public:
	CmdInit();

	virtual void ToStream(RLStream* pStream) const;
	virtual void ParseReply(RLStream* pStream);

	static void AddAppLang(RLStream* pStream);
	static void AddMacAddresses(RLStream* pStream);
	static void AddHDDInfo(RLStream* pStream);
	
	// input
	bool	 m_bService;			// we run under service
	bool	 m_bNeedComputerId;	// if true - always get computer id, false - can be for service

	// output
	CStringA m_strStatus;
	CStringA m_strRouters;
	CStringA m_strFiles;
	CStringA m_strUpdateURL;
	CStringA m_strLicenseType;
	DWORD	 m_timeLastUpdateTrying;

private:
};

#endif // !defined(RL_CMD_INIT_H_INCLUDED_)
