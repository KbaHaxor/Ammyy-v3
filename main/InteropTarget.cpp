#include "stdafx.h"
#include "AmmyyApp.h"
#include "InteropTarget.h"
#include "../main/aaProtocol.h"
#include "TransportT.h"
#include "ReTranslator.h"
#include "../target/TrMain.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


InteropTarget::InteropTarget()
{
	m_type = aaTypeTarget;
	m_state  = 0;
	m_socketRDP = NULL;
	this->SetStoppingEvent(&_TrMain.m_bStopping);
}

InteropTarget::~InteropTarget()
{
}

void InteropTarget::SetState(DWORD state)
{
	char name[64];

	if (m_state==state) return;

	m_state = state;

	m_eventState.Close();

	sprintf(name, "Global\\Ammyy.Target.StateEvent_%d_", m_state);

	m_eventState.Create(name, &_TrMain.m_saPublic);
}

void InteropTarget::ConnectToRDPserver()
{
	TCP socket;

	socket.Create();
	DWORD error = socket.Connect("127.0.0.1", 3389, false); // connect to RDP...
	if (error>0)
		throw RLException("Couldn't connect to local RDP server");	

	TCP::SetSocketOptions(socket);
	
	m_socketRDP = socket.Detach(); // will be closed by CReTranslator
}

void InteropTarget::ReTranslate()
{
	for (int i=0; i<2; i++) // allow 2 attempts
	{
		_log.WriteInfo("InteropTarget::ReTranslater#1 %d", i);

		if (m_socketRDP==NULL) ConnectToRDPserver();
		
		CReTranslator tr;
		tr.s1 = m_transport->GetSocket();
		tr.s2 = m_socketRDP;

		m_socketRDP = NULL;

		TCP::SetBlockingMode(tr.s2, false);
			
		int ret = tr.DoSmart();

		_log.WriteInfo("InteropTarget::ReTranslater#2 %d", ret);

		if (ret==2) continue;
		if (ret==3) {
			_log.WriteInfo("ReTranslate#1 Direct Transfer STARTED------------------------------");
			tr.DoDirect();
			_log.WriteInfo("ReTranslate#2 Direct Transfer FINISHED------------------------------");
			break;
		}
		else {
			_log.WriteError("CReTranslator::DoSmart() return error=%d", ret);
			return;
		}
	}
}

