#if !defined(RL_CMD_MSG_BOX_H_INCLUDED_)
#define RL_CMD_MSG_BOX_H_INCLUDED_

#include "CmdBase.h"

class CmdMsgBox : public CmdBase 
{
public:
	CmdMsgBox();

protected:
	virtual void ParseReply(RLStream* pStream);
	virtual void Execute();

private:
	UINT     m_dwType;
	CStringA m_strMsg;
};

#endif // !defined(RL_CMD_MSG_BOX_H_INCLUDED_)
