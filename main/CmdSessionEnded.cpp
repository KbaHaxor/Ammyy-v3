#include "stdafx.h"
#include "CmdSessionEnded.h"
#include "../RL/RLFile.h"
#include "../RL/RLEncryptor01.h"
#include "aaProtocol.h"
#include "../common/Common2.h"

CmdSessionEnded::CmdSessionEnded()
{
	m_cmdID = CMD_SESSION_ENDED_v3;
	m_strStatus = "-"; //to prevent unreading m_strStatus and it'll look like 'ok'
}

/*
void addUINTasString(RLStream* pStream, UINT32 v)
{
	char str[64];
	int h = sprintf(str, "%u", v);
	pStream->AddRaw(str, h+1);
}
*/

void CmdSessionEnded::ToStream(RLStream* pStream) const
{
	UINT32 now    = Common2::UINT64ToUnixTime(Common2::GetSystemTime());
	UINT32 time1  = Common2::UINT64ToUnixTime(m_time);
	UINT16 timems = (UINT16)(m_time % 1000);

	pStream->AddUINT32(m_cmdID);
	pStream->AddUINT8(m_source);
	pStream->AddString1A(m_buildStamp);
	pStream->AddUINT32(time1);
	pStream->AddUINT16(timems);
	pStream->AddUINT32(now);

	pStream->AddUINT32(m_id_t);
	pStream->AddUINT32(m_id_v);
	pStream->AddString1A(m_ip_t);
	pStream->AddString1A(m_ip_v);
	pStream->AddUINT64(m_sent_t);
	pStream->AddUINT64(m_sent_v);
	pStream->AddUINT64(m_span);
}

void CmdSessionEnded::FromStream(RLStream* pStream)
{
	m_source       = pStream->GetUINT8();
	m_buildStamp   = pStream->GetString1A();
	UINT32 time1   = pStream->GetUINT32();
	UINT16 timems  = pStream->GetUINT16();
	m_time         = Common2::UnixTimeToUINT64(time1, timems);
					  pStream->GetUINT32(); // now
	
	m_id_t         = pStream->GetUINT32();
	m_id_v         = pStream->GetUINT32();
	m_ip_t         = pStream->GetString1A();
	m_ip_v         = pStream->GetString1A();
	m_sent_t       = (UINT32)pStream->GetUINT64();
	m_sent_v       = (UINT32)pStream->GetUINT64();
	m_span	       = (UINT32)pStream->GetUINT64();
}

void CmdSessionEnded::ParseReply(RLStream* pStream) 
{
	m_strStatus = pStream->GetString1A();
	m_needClose = pStream->GetBool();
}

/*
CStringA CmdSessionEnded::ConvertToString(const SYSTEMTIME& t)
{
	CStringA str;
	str.Format("%.4hd%.2hd%.2hd%.2hd%.2hd%.2hd%.3hd",
			t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);

	return str; // example "20100827094711482"
}

void CmdSessionEnded::ConvertToTime(const CStringA& str, SYSTEMTIME& t)
{
	t.wYear         = atoi(str.Mid(0, 4));
	t.wMonth        = atoi(str.Mid(4, 2));
	t.wDay          = atoi(str.Mid(6, 2));
	t.wHour         = atoi(str.Mid(8, 2));
	t.wMinute       = atoi(str.Mid(10, 2));
	t.wSecond       = atoi(str.Mid(12, 2));
	t.wMilliseconds = atoi(str.Mid(14, 3));
}
*/

//______________________________________________________________________________________________________

#ifdef AMMYY_ROUTER
#include "../Router/CRouter.h"
#endif

CmdSessionEndedQueue _CmdSessionEndedQueue;


void CmdSessionEndedQueue::SendOneCmd(CmdSessionEnded& cmd, bool last)
{	
	cmd.Send();

	if (!cmd.m_strStatus.IsEmpty())
		throw RLException("Incorrect reply from server status='%s'", (LPCSTR)cmd.m_strStatus);

#ifdef AMMYY_ROUTER
	if (last && cmd.m_needClose) {
		_log.WriteError("Closing router because version is expired");
		Router.Stop();
	}
#endif
}


void CmdSessionEndedQueue::AddToQueue(const CmdSessionEnded& cmd)
{
	RLStream buffer(1024);
	cmd.ToStream(&buffer);

	UINT len = buffer.GetLen();
	char* pBuffer = (char*)buffer.GetBuffer();

	// we need fast setup encoding, so we use old encoder	
	RLEncryptor01::Encrypt((UINT8*)(pBuffer+4), len-4, 0, (char*)&aaKey02[0]);

	*((UINT32*)pBuffer) = len - 4;

	RLFile file;
	file.OpenOrCreate(m_fileName);
	file.Write(pBuffer, len);
	file.Close();
}


void CmdSessionEndedQueue::SendFromQueue()
{
	if (!Common2::FileIsExistA(m_fileName)) return;

	RLStream file;
	file.ReadFromFile(m_fileName);

	while (true)
	{
		if (file.GetAvailForReading()==0) {
			try {
				Common2::DeleteFileA(m_fileName);
			}
			catch(RLException& ex) {
				_log.WriteError(ex.GetDescription());
			}
			break;
		}

		UINT32 len = file.GetUINT32();

		RLStream buffer(len);
		file.GetRaw(buffer.GetBuffer(), len);
		buffer.SetLen(len);

		RLEncryptor01::Encrypt(buffer.GetBuffer(), len, 0, (char*)&aaKey02[0]); // decrypt

		CmdSessionEnded cmd;
		cmd.FromStream(&buffer);

		SendOneCmd(cmd, false);

		file.CutTillRead();
		if (file.GetAvailForReading()!=0) {
			file.WriteToFile(m_fileName);
		}
	}
}

