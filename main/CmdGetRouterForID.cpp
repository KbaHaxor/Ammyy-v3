#include "stdafx.h"
#include "CmdGetRouterForID.h"
#include "CmdInit.h"
#include "../main/Common.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

CmdGetRouterForID::CmdGetRouterForID()
{
	m_cmdID = CMD_GET_ROUTER_FOR_ID;
	m_status = "-"; //to prevent unreading m_status and it'll look like 'ok'
}


void CmdGetRouterForID::ToStream(RLStream* pStream) const
{
	pStream->AddUINT32(m_cmdID);
	pStream->AddString1A(CStringA(settings.GetVersionSTR()));
	pStream->AddString1A(CStringA(CCommon::GetBuildDateTime()));
	CmdBase::AddOSLanguage(pStream);
	CmdInit::AddAppLang(pStream);
	pStream->AddUINT32(m_localID);
	pStream->AddUINT32(m_remoteID);
}


void CmdGetRouterForID::ParseReply(RLStream* pStream)
{
	m_status = pStream->GetString1A();
	m_router = pStream->GetString1A();

	m_msgText  = pStream->GetString1A();
	m_msgStyle = pStream->GetUINT32();
	m_url      = pStream->GetString1A();
	m_allow	   = pStream->GetBool();

	CStringA license = pStream->GetString1A();

	settings.SetHardware("", license);
}


