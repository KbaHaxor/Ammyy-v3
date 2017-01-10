#if !defined(AFX_TASKSESSIONENDED_H__07337E45_1C2B_4EBB_9BFB_9562D2CAE1DE__INCLUDED_)
#define AFX_TASKSESSIONENDED_H__07337E45_1C2B_4EBB_9BFB_9562D2CAE1DE__INCLUDED_

#pragma warning(disable:4786)
#include <list>

#include "../RL/RLLock.h"
#include "../main/aaProtocol.h"
#include "../main/CmdSessionEnded.h"

class TaskSessionEnded  
{
public:
	TaskSessionEnded();
	virtual ~TaskSessionEnded() {}

	virtual void Post();
	virtual void DoTask(bool onlySave);

	CStringA m_viewerIp;	// ip + port + router_port
	CStringA m_targetIp;    // ip + port + router_port
	LPCSTR   m_terminator;	// 'v' or 't'

	CmdSessionEnded m_cmd;

	static void	Init();

	static TaskSessionEnded* GetFirst();

	static std::list<TaskSessionEnded*>  m_queueMemory;
	static RLMutex						 m_queueLocker;

private:
	static CStringA ConvertToString(const SYSTEMTIME& t);

	static RLLog	m_logSessionsAll;
};


#endif // !defined(AFX_TASKSESSIONENDED_H__07337E45_1C2B_4EBB_9BFB_9562D2CAE1DE__INCLUDED_)
