#include "stdafx.h"
#include "CmdMsgBox.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

CmdMsgBox::CmdMsgBox()
{
	m_cmdID  = CMD_MSG_BOX;
	m_dwType = 0;
}


void CmdMsgBox::ParseReply(RLStream* pStream) 
{
	m_dwType = pStream->GetUINT32();
	m_strMsg = pStream->GetString1A();
}


void CmdMsgBox::Execute()
{
	::MessageBox(NULL, m_strMsg, m_strAppName, m_dwType);
}
