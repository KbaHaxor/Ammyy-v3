#include "stdafx.h"
#include "CmdInit.h"
#include "Common.h"
#include "../RL/RLHardwareMac.h"
#include "../RL/RLHardwareHDD.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

CmdInit::CmdInit()
{
	m_cmdID = CMD_INIT;
}

void CmdInit::AddMacAddresses(RLStream* pStream)
{
	static RLStream macs_cashe;

	if (macs_cashe.GetLen()==0) {
		RLHardwareMac::AddMacAddressesReal(&macs_cashe);
	}
	pStream->AddRaw(macs_cashe.GetBuffer(), macs_cashe.GetLen());
}

void CmdInit::AddHDDInfo(RLStream* pStream)
{
	static RLStream hdd_cashe;

	if (hdd_cashe.GetLen()==0) 
	{
		bool primary;
		RLHardwareHDD::HDD hdd = RLHardwareHDD::GetHDD(primary);
		if (hdd.IsValid())
		{
			// can be problem if we meet char=0, so we replcae it to space
			hdd.firmwareRev.Replace('\0', ' ');
			hdd.model.Replace('\0', ' ');
			hdd.serial.Replace('\0', ' ');

			hdd_cashe.SetMinCapasity(64); // optional to speed up
			hdd_cashe.AddUINT8((primary)? 1 : 2);
			hdd_cashe.AddUINT64(hdd.size);
			hdd_cashe.AddString1A(hdd.firmwareRev);
			hdd_cashe.AddString1A(hdd.model);
			hdd_cashe.AddString1A(hdd.serial);

			//_log.WriteInfo("hdd %u %I64d '%s' '%s' '%s'", primary, hdd.size, hdd.firmwareRev, hdd.model, hdd.serial);
		}
		else {
			hdd_cashe.AddUINT8(0); // no valid hdd
		}		
	}
	pStream->AddRaw(hdd_cashe.GetBuffer(), hdd_cashe.GetLen());
}


void CmdInit::ToStream(RLStream* pStream) const
{
	_log.WriteInfo("CCmdInit::ToStream()#1");

	pStream->AddUINT32(m_cmdID);
	pStream->AddString1A(m_strAppName);
	pStream->AddString1A(CStringA(settings.GetVersionSTR()));	
	pStream->AddString1A(CStringA(CCommon::GetBuildDateTime()));
	pStream->AddString1A(m_strProductKey);
	AddAppLang(pStream);
	pStream->AddString1A(m_strComputerId);	// previous computer id

	AddOSInfo(pStream);
	CmdInit::AddMacAddresses(pStream);
	CmdInit::AddHDDInfo(pStream);
	pStream->AddBool(m_bService);
	pStream->AddBool(m_bNeedComputerId);
	pStream->AddUINT32(m_timeLastUpdateTrying);
}

void CmdInit::ParseReply(RLStream* pStream) 
{
	m_strStatus = pStream->GetString1A();

	if (m_strStatus.IsEmpty()) {
		m_strComputerId  = pStream->GetString1A();
		m_strRouters     = pStream->GetString1A();
		m_strFiles       = pStream->GetString1A();
		m_strLicenseType = pStream->GetString1A();
	}
	else if (m_strStatus=="UPDATE") {
		m_strUpdateURL  = pStream->GetString1A();
		m_timeLastUpdateTrying = pStream->GetUINT32();
	}
	else {
		throw RLException("Incorrect reply from server status='%s'", m_strStatus);
	}
}

void CmdInit::AddAppLang(RLStream* pStream)
{
	CStringA str;
	str.Format("%u", (int)settings.m_langId);
	pStream->AddString1A(str);
}


