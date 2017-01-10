#include "stdafx.h"
#include "TaskSessionEnded.h"
#include "RouterApp.h"
#include "CRouter.h"
#include "../common/Common2.h"


RLLog TaskSessionEnded::m_logSessionsAll;
std::list<TaskSessionEnded*> TaskSessionEnded::m_queueMemory;
RLMutex TaskSessionEnded::m_queueLocker;


void TaskSessionEnded::Init()
{
	// init special logs
	m_logSessionsAll.InitA  (TheApp.m_path + "sessions.log");
	_CmdSessionEndedQueue.m_fileName = TheApp.m_path + "sessions_queue.bin";
}


TaskSessionEnded::TaskSessionEnded()
{
	//::GetSystemTime(&m_time);
}


void TaskSessionEnded::Post()
{
	RLMutexLock l(m_queueLocker);
	m_queueMemory.push_back(this);
}

TaskSessionEnded* TaskSessionEnded::GetFirst()
{
	RLMutexLock l(m_queueLocker);

	if (m_queueMemory.size()==0) {
		return NULL;
	}
	else {
		TaskSessionEnded* p = m_queueMemory.front();
		m_queueMemory.pop_front();
		return p;
	}
}

CStringA TaskSessionEnded::ConvertToString(const SYSTEMTIME& t)
{
	CStringA str;
	str.Format("%.4hd%.2hd%.2hd-%.2hd:%.2hd:%.2hd.%.3hd",
			t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);

	return str; // example "20100827-09:47:11.482"
}



void TaskSessionEnded::DoTask(bool onlySave)
{
	// save as human readable string
	{
		LPCSTR mark = (m_terminator[0] == 'v') ? " *-> " : " ->* ";
		CStringA ViewerTargetNames = m_viewerIp + mark + m_targetIp;

		SYSTEMTIME time1;

		Common2::UINT64ToSystemTime(m_cmd.m_time, time1);
		
		CStringA date1 = ConvertToString(time1);

		SYSTEMTIME t;
		::GetSystemTime(&t);

		CStringA date2 = ConvertToString(t);

		CStringA line2;
		line2.Format("%s %s %5d.%03ds %7d+%9d    %s\r\n",
			(LPCSTR)date2,
			(LPCSTR)date1,
			(UINT32)(m_cmd.m_span/1000), // seconds
			(UINT32)(m_cmd.m_span%1000),
			(UINT32)m_cmd.m_sent_t,
			(UINT32)m_cmd.m_sent_v,
			(LPCSTR)ViewerTargetNames);

		m_logSessionsAll.WriteRawData(line2, line2.GetLength());
	}

	if (onlySave) {
		_CmdSessionEndedQueue.AddToQueue(m_cmd);
	}
	else {
		try {
			// first try to send old entries
			_CmdSessionEndedQueue.SendFromQueue();
			_CmdSessionEndedQueue.SendOneCmd(m_cmd, true);
		}
		catch(RLException& ex) {
			_log.WriteError(ex.GetDescription());

			// if not sent, we will try to send next time
			_CmdSessionEndedQueue.AddToQueue(m_cmd);
		}
	}

	delete this;
}
