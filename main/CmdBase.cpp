#include "stdafx.h"
#include "CmdBase.h"

#ifdef _WIN32
#include <IPHlpApi.h>
#include <NtDDNdis.h>
#include <SetupAPI.h>
#include <RegStr.h>
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "Iphlpapi.lib")
//#include "../RL/RLHttp.h"
#include "CmdMsgBox.h"
#endif

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif



RLBase64Coder CmdBase::m_base64coder("EFGHIJKL123456MNOPQRSTUVWXYZabcdefghABCDijklmnopqrstuvwxyz0789/-");


CStringA CmdBase::m_strURL = "http://rl.ammyy.com";
CStringA CmdBase::m_strAppName;
CStringA CmdBase::m_strAppVersion;
CStringA CmdBase::m_strAppModule;
CStringA CmdBase::m_strProductKey;


CmdBase::CmdBase()
{
}

CmdBase::~CmdBase()
{
}

void CmdBase::ToStream(RLStream* pStream) const
{
	throw RLException("ToStream() not implemented");
}


void CmdBase::Send()
{
	RLStream streamSend(1024);
	RLStream streamEncoded;
	RLStream streamPostData(128);

	_log.WriteInfo("CCmdBase::Send()#1");

	this->ToStream(&streamSend);

	streamPostData.AddRaw("v=", 2);
	streamPostData.AddString0A(m_strAppVersion);
	streamPostData.AddRaw("&d=", 3);

	m_base64coder.Encode(&streamSend, &streamPostData);

	#ifdef _RLHTTP_2_H__INCLUDED_
		RLHttp2 http;
		http.Post(m_strURL, &streamPostData);

		if (http.m_replyStatus != "200")
			throw RLException("Error, http reply status='%s' is incorrect", (LPCSTR)http.m_replyStatus);

		RLStream& streamRecv = http.m_reply;
	#else
		RLHttp http;
		if (!http.Post(m_strURL, &streamPostData)) {
			_log.WriteInfo("CCmdBase::Send()#5_ERROR");
			throw RLException("Error {%d} occured while connecting to server \"%s\" ", http.GetError(), m_strURL);
		}

		DWORD http_code = http.GetStatusCode();

		if (http_code==407) throw RLException("Proxy Authentication Required");
		if (http_code!=200) throw RLException("HTTP status code=%u", http_code);

		RLStream streamRecv;
		if (!http.ReadData(streamRecv))				
			throw RLException("Error {%d} occured while reading from server \"%s\" ", http.GetError(), m_strURL);
	#endif


	_log.WriteInfo("CCmdBase::Send()#7");

	DWORD streamRecvLen = streamRecv.GetLen();

	if (streamRecvLen<8)
		throw RLException("Bad reply from server#1 {%d}", streamRecvLen);

	_log.WriteInfo("CCmdBase::Send()#8");

	DWORD dwInitialLabel = streamRecv.GetUINT32();

	if (0x2d2d4c52 != dwInitialLabel) // "RL--"
		throw RLException("Bad reply from server#2 %s {%x}", (LPCSTR)m_strURL, dwInitialLabel);

	_log.WriteInfo("CCmdBase::Send()#9");

	RLStream streamRecvDec;
	m_base64coder.Decode(&streamRecv, &streamRecvDec);

	_log.WriteInfo("CCmdBase::Send()#10");

	ExecuteReplyFromServer(&streamRecvDec);

	_log.WriteInfo("CCmdBase::Send()#11");
}



void CmdBase::ExecuteReplyFromServer(RLStream* pStream)
{
	while(pStream->GetAvailForReading()>=4) 
	{
		DWORD cmdId = pStream->GetUINT32();

		CmdBase* pCmd = NULL;

		if (m_cmdID == cmdId) {
			pCmd=this;
		}
		else {
			#ifdef AMMYY_ADMIN
			switch(cmdId)
			{			
				case CMD_MSG_BOX: pCmd=new CmdMsgBox(); break;			
				default: throw RLException("Invalid cmdID=%d from server", cmdId);
			}
			#else
				throw RLException("Invalid cmdID=%d from server", cmdId);
			#endif
		}

		if (pCmd!=NULL) {
			pCmd->ParseReply(pStream);
			pCmd->Execute();
		}
	}
}



#ifdef _WIN32
// get country telephone code of "standards and formats" settings
//
DWORD CmdBase::GetUserFormatCountryCode()
{
	DWORD countryCode = 0;
	char buffer[64];
		
	int  h = ::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IDEFAULTCOUNTRY, buffer, 64);
	if (h>0) {
		countryCode = atol(buffer);
	}

	return countryCode;
}


// get "location" of "Regional and Language Options" of Windows Control Panel
//
GEOID CmdBase::GetUserLocation()
{
	FARPROC address = ::GetProcAddress(::GetModuleHandle("kernel32.dll"), "GetUserGeoID");
	if (address==NULL) return GEOID_NOT_AVAILABLE;

	return ((GEOID (WINAPI*)(GEOCLASS))address)(GEOCLASS_NATION);
}

INT32 CmdBase::GetProcessorArchitecture()
{
	FARPROC address = ::GetProcAddress(::GetModuleHandleA("kernel32.dll"), "GetNativeSystemInfo");
	if (address==NULL) return -1;

	SYSTEM_INFO si;
	ZeroMemory(&si, sizeof(SYSTEM_INFO));

	((void (WINAPI*)(LPSYSTEM_INFO))address)(&si);

	return (INT32)si.wProcessorArchitecture;
}
#endif


void CmdBase::AddOSInfo(RLStream* pStream)
{
	// adding string {xx.xx.xx yy [SPzz.zz]}, xx-OS version, yy-Processor Architecture, zz - Service Pack
	CStringA str;
#ifdef _WIN32
	OSVERSIONINFOEX info;
	info.dwOSVersionInfoSize = sizeof(info);
	VERIFY(FALSE != ::GetVersionEx((OSVERSIONINFO*)&info));
	
	INT32 iProcessorArch = GetProcessorArchitecture();

	str.Format( (info.wServicePackMajor!=0 || info.wServicePackMinor!=0) ? "%u.%u.%u %d SP%u.%u" : "%u.%u.%u %d",
			info.dwMajorVersion, info.dwMinorVersion, info.dwBuildNumber,
			iProcessorArch, 
			info.wServicePackMajor, info.wServicePackMinor);
#else
	str = "0.0.0 0"; // Not implemented for Unix
#endif
	pStream->AddString1A(str);

	AddOSLanguage(pStream);
}


// adding string {xx.yy.zz}, xx-Default UI Language, yy-user country code, zz - user location
void CmdBase::AddOSLanguage(RLStream* pStream)
{
	CStringA str;

#ifdef _WIN32
	DWORD landID		= ::GetUserDefaultUILanguage(); // get language of Windows for current user
	DWORD countryCode	= GetUserFormatCountryCode();
	GEOID geoID			= GetUserLocation();

	str.Format("%u.%u.%d", landID, countryCode, geoID);
#else
	str = "0.0.0"; // Not implemented for Unix
#endif
	pStream->AddString1A(str);
}
