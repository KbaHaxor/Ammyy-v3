#include "stdafx.h"
#include "CmdPortTest.h"
#include "Common.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

CmdPortTest::CmdPortTest()
{
	m_cmdID = CMD_PORT_TEST;
}

void CmdPortTest::ToStream(RLStream* pStream) const
{
	pStream->AddUINT32(m_cmdID);
	pStream->AddUINT16(m_port);
	pStream->AddUINT32(5000);  // timeous  in milliseconds
	pStream->AddRaw(m_key, sizeof(m_key));
}

void CmdPortTest::ParseReply(RLStream* pStream) 
{
	m_external_ip   = pStream->GetUINT32();
	m_external_port = pStream->GetUINT16();
	m_result        = pStream->GetUINT8();
}



