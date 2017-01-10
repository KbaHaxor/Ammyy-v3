#if !defined(RL_CMD_SESSION_ENED_H_INCLUDED_)
#define RL_CMD_SESSION_ENED_H_INCLUDED_

#pragma once

#include "../main/CmdBase.h"

class CmdSessionEnded : public CmdBase
{
public:
	CmdSessionEnded();

	virtual void ToStream(RLStream* pStream) const;
	virtual void ParseReply(RLStream* pStream);

	void FromStream(RLStream* pStream);

public:
	// input
	UINT8		m_source;			// 1-router v2, 2-router v3, 3 - operator
	CStringA	m_buildStamp;
	UINT64		m_time;				// time when session is started (for v2 it was ended)
	UINT32		m_id_t;
	UINT32		m_id_v;
	CStringA	m_ip_t;
	CStringA	m_ip_v;
	UINT64		m_sent_v;
	UINT64		m_sent_t;
	UINT64		m_span; // in ms

	// output
	CStringA m_strStatus;	// should be empty
	bool	 m_needClose;
};

class CmdSessionEndedQueue
{
public:
	void AddToQueue(const CmdSessionEnded& cmd);
	void SendOneCmd(CmdSessionEnded& cmd, bool last);
	void SendFromQueue();

	CStringA m_fileName;
};

extern CmdSessionEndedQueue _CmdSessionEndedQueue;

#endif // !defined(RL_CMD_SESSION_ENED_H_INCLUDED_)
